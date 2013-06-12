#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include <string.h>
#include "dev/leds.h"
#include "dev/uart0.h"
#include "dev/button-sensor.h"
#include "net/uip-debug.h"
#include <math.h>
#include <stdlib.h>

PROCESS(gps_process, "GPS process");
AUTOSTART_PROCESSES(&gps_process);
int32_t curTime = 0; // 120 anos
#define millis() (curTime*1000)

#define radians(x) ((x)*PI/180.0)
#define degrees(x) ((x)*180.0/PI)
#define sq(x) ((x)*(x))

typedef unsigned char byte;
typedef bool;
#define true 1
#define false 0

#define _GPRMC_TERM   "GPRMC"
#define _GPGGA_TERM   "GPGGA"

#define _GPS_VERSION 12 // software version of this library
#define _GPS_MPH_PER_KNOT 1.15077945
#define _GPS_MPS_PER_KNOT 0.51444444
#define _GPS_KMPH_PER_KNOT 1.852
#define _GPS_MILES_PER_METER 0.00062137112
#define _GPS_KM_PER_METER 0.001

enum {
	GPS_INVALID_AGE = 0xFFFFFFFF,      GPS_INVALID_ANGLE = 999999999,
	GPS_INVALID_ALTITUDE = 999999999,  GPS_INVALID_DATE = 0,
	GPS_INVALID_TIME = 0xFFFFFFFF,		 GPS_INVALID_SPEED = 999999999,
	GPS_INVALID_FIX_TIME = 0xFFFFFFFF, GPS_INVALID_SATELLITES = 0xFF,
	GPS_INVALID_HDOP = 0xFFFFFFFF
};
const float GPS_INVALID_F_ANGLE = 1000.0;
const float GPS_INVALID_F_ALTITUDE = 1000000.0;
const float GPS_INVALID_F_SPEED = -1.0;
enum {_GPS_SENTENCE_GPGGA, _GPS_SENTENCE_GPRMC, _GPS_SENTENCE_OTHER};

// properties
unsigned long _time, _new_time;
unsigned long _date, _new_date;
long _latitude, _new_latitude;
long _longitude, _new_longitude;
long _altitude, _new_altitude;
unsigned long  _speed, _new_speed;
unsigned long  _course, _new_course;
unsigned long  _hdop, _new_hdop;
unsigned short _numsats, _new_numsats;
unsigned long _last_time_fix, _new_time_fix;
unsigned long _last_position_fix, _new_position_fix;
byte _parity;
bool _is_checksum_term;
char _term[15];
byte _sentence_type;
byte _term_number;
byte _term_offset;
bool _gps_data_good;

#ifndef _GPS_NO_STATS
// statistics
unsigned long _encoded_characters;
unsigned short _good_sentences;
unsigned short _failed_checksum;
unsigned short _passed_checksum;
#endif

// internal utilities
int from_hex(char a);
unsigned long parse_decimal();
unsigned long parse_degrees();
bool term_complete();
bool gpsisdigit(char c) { return c >= '0' && c <= '9'; }
long gpsatol(const char *str);
int gpsstrcmp(const char *str1, const char *str2);

// lat/long in hundred thousandths of a degree and age of fix in milliseconds
void get_position(long *latitude, long *longitude, unsigned long *fix_age); // = 0

// date as ddmmyy, time as hhmmsscc, and age in milliseconds
void get_datetime(unsigned long *date, unsigned long *time, unsigned long *age); // = 0

// signed altitude in centimeters (from GPGGA sentence)
inline long altitude() { return _altitude; }

// course in last full GPRMC sentence in 100th of a degree
inline unsigned long course() { return _course; }

// speed in last full GPRMC sentence in 100ths of a knot
inline unsigned long speed() { return _speed; }

// satellites used in last full GPGGA sentence
inline unsigned short satellites() { return _numsats; }

// horizontal dilution of precision in 100ths
inline unsigned long hdop() { return _hdop; }

bool encode(char c); // process one character received from GPS

void f_get_position(float *latitude, float *longitude, unsigned long *fix_age); // = 0
void crack_datetime(int *year, byte *month, byte *day,
		byte *hour, byte *minute, byte *second, byte *hundredths, unsigned long *fix_age); // =0, = 0
float f_altitude();
float f_course();
float f_speed_knots();
float f_speed_mph();
float f_speed_mps();
float f_speed_kmph();

static int library_version() { return _GPS_VERSION; }

static float distance_between (float lat1, float long1, float lat2, float long2);
static float course_to (float lat1, float long1, float lat2, float long2);
static const char *cardinal(float course);

#ifndef _GPS_NO_STATS
void stats(unsigned long *chars, unsigned short *good_sentences, unsigned short *failed_cs);
#endif
void initGps(){
	_time = (GPS_INVALID_TIME);
	_date = (GPS_INVALID_DATE);
	_latitude = (GPS_INVALID_ANGLE);
	_longitude = (GPS_INVALID_ANGLE);
	_altitude = (GPS_INVALID_ALTITUDE);
	_speed = (GPS_INVALID_SPEED);
	_course = (GPS_INVALID_ANGLE);
	_hdop = (GPS_INVALID_HDOP);
	_numsats = (GPS_INVALID_SATELLITES);
	_last_time_fix = (GPS_INVALID_FIX_TIME);
	_last_position_fix = (GPS_INVALID_FIX_TIME);
	_parity = (0);
	_is_checksum_term = (false);
	_sentence_type = (_GPS_SENTENCE_OTHER);
	_term_number = (0);
	_term_offset = (0);
	_gps_data_good = (false);
#ifndef _GPS_NO_STATS
	_encoded_characters = (0);
	_good_sentences = (0);
	_failed_checksum = (0);
#endif
	_term[0] = '\0';
}

bool encode(char c)
{
	bool valid_sentence = false;

#ifndef _GPS_NO_STATS
	++_encoded_characters;
#endif
	switch(c)
	{
	case ',': // term terminators
		_parity ^= c;
	case '\r':
	case '\n':
	case '*':
		if (_term_offset < sizeof(_term))
		{
			_term[_term_offset] = 0;
			valid_sentence = term_complete();
		}
		++_term_number;
		_term_offset = 0;
		_is_checksum_term = c == '*';
		return valid_sentence;

	case '$': // sentence begin
		_term_number = _term_offset = 0;
		_parity = 0;
		_sentence_type = _GPS_SENTENCE_OTHER;
		_is_checksum_term = false;
		_gps_data_good = false;
		return valid_sentence;
	}

	// ordinary characters
	if (_term_offset < sizeof(_term) - 1)
		_term[_term_offset++] = c;
	if (!_is_checksum_term)
		_parity ^= c;

	return valid_sentence;
}


#ifndef _GPS_NO_STATS
void stats(unsigned long *chars, unsigned short *sentences, unsigned short *failed_cs)
{
	if (chars) *chars = _encoded_characters;
	if (sentences) *sentences = _good_sentences;
	if (failed_cs) *failed_cs = _failed_checksum;
}
#endif

int from_hex(char a)
{
	if (a >= 'A' && a <= 'F')
		return a - 'A' + 10;
	else if (a >= 'a' && a <= 'f')
		return a - 'a' + 10;
	else
		return a - '0';
}

unsigned long parse_decimal()
{
	static  unsigned long ret;
	static char *p;
	static bool isneg;
	p = _term;
	isneg = *p == '-';
	if (isneg) ++p;
	ret = 100UL * gpsatol(p);
	while (gpsisdigit(*p)) ++p;
	if (*p == '.')
	{
		if (gpsisdigit(p[1]))
		{
			ret += 10 * (p[1] - '0');
			if (gpsisdigit(p[2]))
				ret += p[2] - '0';
		}
	}
	return isneg ? -ret : ret;
}

unsigned long parse_degrees()
{
	static char *p;
	static unsigned long left;
	static unsigned long tenk_minutes;
	left = gpsatol(_term);
	tenk_minutes= (left % 100UL) * 10000UL;
	for (p=_term; gpsisdigit(*p); ++p);
	if (*p == '.')
	{
		unsigned long mult = 1000;
		while (gpsisdigit(*++p))
		{
			tenk_minutes += mult * (*p - '0');
			mult /= 10;
		}
	}
	return (left / 100) * 100000 + tenk_minutes / 6;
}

#define COMBINE(sentence_type, term_number) (((unsigned)(sentence_type) << 5) | term_number)

// Processes a just-completed term
// Returns true if new sentence has just passed checksum test and is validated
bool term_complete()
{
	if (_is_checksum_term)
	{
		static byte checksum;
		checksum = 16 * from_hex(_term[0]) + from_hex(_term[1]);
		if (checksum == _parity)
		{
			if (_gps_data_good)
			{
#ifndef _GPS_NO_STATS
				++_good_sentences;
#endif
				_last_time_fix = _new_time_fix;
				_last_position_fix = _new_position_fix;

				switch(_sentence_type)
				{
				case _GPS_SENTENCE_GPRMC:
					_time      = _new_time;
					_date      = _new_date;
					_latitude  = _new_latitude;
					_longitude = _new_longitude;
					_speed     = _new_speed;
					_course    = _new_course;
					break;
				case _GPS_SENTENCE_GPGGA:
					_altitude  = _new_altitude;
					_time      = _new_time;
					_latitude  = _new_latitude;
					_longitude = _new_longitude;
					_numsats   = _new_numsats;
					_hdop      = _new_hdop;
					break;
				}

				return true;
			}
		}

#ifndef _GPS_NO_STATS
		else
			++_failed_checksum;
#endif
		return false;
	}

	// the first term determines the sentence type
	if (_term_number == 0)
	{
		if (!gpsstrcmp(_term, _GPRMC_TERM))
			_sentence_type = _GPS_SENTENCE_GPRMC;
		else if (!gpsstrcmp(_term, _GPGGA_TERM))
			_sentence_type = _GPS_SENTENCE_GPGGA;
		else
			_sentence_type = _GPS_SENTENCE_OTHER;
		return false;
	}
	if (_sentence_type != _GPS_SENTENCE_OTHER && _term[0])
		switch(COMBINE(_sentence_type, _term_number))
		{
		case COMBINE(_GPS_SENTENCE_GPRMC, 1): // Time in both sentences
		case COMBINE(_GPS_SENTENCE_GPGGA, 1):
		_new_time = parse_decimal();
		_new_time_fix = millis();
		break;
		case COMBINE(_GPS_SENTENCE_GPRMC, 2): // GPRMC validity
    		  _gps_data_good = _term[0] == 'A';
		break;
		case COMBINE(_GPS_SENTENCE_GPRMC, 3): // Latitude
		case COMBINE(_GPS_SENTENCE_GPGGA, 2):
		_new_latitude = parse_degrees();
		_new_position_fix = millis();
		break;
		case COMBINE(_GPS_SENTENCE_GPRMC, 4): // N/S
		case COMBINE(_GPS_SENTENCE_GPGGA, 3):
		if (_term[0] == 'S')
			_new_latitude = -_new_latitude;
		break;
		case COMBINE(_GPS_SENTENCE_GPRMC, 5): // Longitude
		case COMBINE(_GPS_SENTENCE_GPGGA, 4):
		_new_longitude = parse_degrees();
		break;
		case COMBINE(_GPS_SENTENCE_GPRMC, 6): // E/W
		case COMBINE(_GPS_SENTENCE_GPGGA, 5):
		if (_term[0] == 'W')
			_new_longitude = -_new_longitude;
		break;
		case COMBINE(_GPS_SENTENCE_GPRMC, 7): // Speed (GPRMC)
    		  _new_speed = parse_decimal();
		break;
		case COMBINE(_GPS_SENTENCE_GPRMC, 8): // Course (GPRMC)
    		  _new_course = parse_decimal();
		break;
		case COMBINE(_GPS_SENTENCE_GPRMC, 9): // Date (GPRMC)
    		  _new_date = gpsatol(_term);
		break;
		case COMBINE(_GPS_SENTENCE_GPGGA, 6): // Fix data (GPGGA)
    		  _gps_data_good = _term[0] > '0';
		break;
		case COMBINE(_GPS_SENTENCE_GPGGA, 7): // Satellites used (GPGGA)
    		  _new_numsats = (unsigned char)atoi(_term);
		break;
		case COMBINE(_GPS_SENTENCE_GPGGA, 8): // HDOP
    		  _new_hdop = parse_decimal();
		break;
		case COMBINE(_GPS_SENTENCE_GPGGA, 9): // Altitude (GPGGA)
    		  _new_altitude = parse_decimal();
		break;
		}
	return false;
}

long gpsatol(const char *str)
{
	static long ret;
	ret= 0;
	while (gpsisdigit(*str))
		ret = 10 * ret + *str++ - '0';
	return ret;
}

int gpsstrcmp(const char *str1, const char *str2)
{
	while (*str1 && *str1 == *str2)
		++str1, ++str2;
	return *str1;
}

/* static */
float distance_between (float lat1, float long1, float lat2, float long2)
{
	// returns distance in meters between two positions, both specified
	// as signed decimal-degrees latitude and longitude. Uses great-circle
	// distance computation for hypothetical sphere of radius 6372795 meters.
	// Because Earth is no exact sphere, rounding errors may be up to 0.5%.
	// Courtesy of Maarten Lamers
	static float delta ,sdlong ,cdlong ,slat1 ,clat1 ,slat2 ,clat2 ,denom;
	delta = radians(long1-long2);
	sdlong = sinf(delta);
	cdlong = cosf(delta);
	lat1 = radians(lat1);
	lat2 = radians(lat2);
	slat1 = sinf(lat1);
	clat1 = cosf(lat1);
	slat2 = sinf(lat2);
	clat2 = cosf(lat2);
	delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
	delta = sq(delta);
	delta += sq(clat2 * sdlong);
	delta = sqrtf(delta);
	denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
	delta = atan2f(delta, denom);
	return delta * 6372795;
}

float course_to (float lat1, float long1, float lat2, float long2)
{
	// returns course in degrees (North=0, West=270) from position 1 to position 2,
	// both specified as signed decimal-degrees latitude and longitude.
	// Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
	// Courtesy of Maarten Lamers
	static float dlon, a1, a2;
	dlon = radians(long2-long1);
	lat1 = radians(lat1);
	lat2 = radians(lat2);
	a1 = sinf(dlon) * cosf(lat2);
	a2 = sinf(lat1) * cosf(lat2) * cosf(dlon);
	a2 = cosf(lat1) * sinf(lat2) - a2;
	a2 = atan2f(a1, a2);
	if (a2 < 0.0)
	{
		a2 += TWO_PI;
	}
	return degrees(a2);
}

const char *cardinal (float course)
{
	static const char* directions[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};

	int direction = (int)((course + 11.25f) / 22.5f);
	return directions[direction % 16];
}

// lat/long in hundred thousandths of a degree and age of fix in milliseconds
void get_position(long *latitude, long *longitude, unsigned long *fix_age)
{
	if (latitude) *latitude = _latitude;
	if (longitude) *longitude = _longitude;
	if (fix_age) *fix_age = _last_position_fix == GPS_INVALID_FIX_TIME ?
			GPS_INVALID_AGE : millis() - _last_position_fix;
}

// date as ddmmyy, time as hhmmsscc, and age in milliseconds
void get_datetime(unsigned long *date, unsigned long *time, unsigned long *age)
{
	if (date) *date = _date;
	if (time) *time = _time;
	if (age) *age = _last_time_fix == GPS_INVALID_FIX_TIME ?
			GPS_INVALID_AGE : millis() - _last_time_fix;
}

void f_get_position(float *latitude, float *longitude, unsigned long *fix_age)
{
	static long lat, lon;
	get_position(&lat, &lon, fix_age);
	*latitude = lat == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : (lat / 100000.0);
	*longitude = lat == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : (lon / 100000.0);
}

void crack_datetime(int *year, byte *month, byte *day,
		byte *hour, byte *minute, byte *second, byte *hundredths, unsigned long *age)
{
	static  unsigned long date, time;
	get_datetime(&date, &time, age);
	if (year)
	{
		*year = date % 100;
		*year += *year > 80 ? 1900 : 2000;
	}
	if (month) *month = (date / 100) % 100;
	if (day) *day = date / 10000;
	if (hour) *hour = time / 1000000;
	if (minute) *minute = (time / 10000) % 100;
	if (second) *second = (time / 100) % 100;
	if (hundredths) *hundredths = time % 100;
}

float f_altitude()
{
	return _altitude == GPS_INVALID_ALTITUDE ? GPS_INVALID_F_ALTITUDE : _altitude / 100.0;
}

float f_course()
{
	return _course == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : _course / 100.0;
}

float f_speed_knots()
{
	return _speed == GPS_INVALID_SPEED ? GPS_INVALID_F_SPEED : _speed / 100.0;
}

float f_speed_mph()
{
	static float sk;
	sk= f_speed_knots();
	return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_MPH_PER_KNOT * f_speed_knots();
}

float f_speed_mps()
{
	static  float sk;
	sk= f_speed_knots();
	return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_MPS_PER_KNOT * f_speed_knots();
}

float f_speed_kmph()
{
	static float sk;
	sk = f_speed_knots();
	return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_KMPH_PER_KNOT * f_speed_knots();
}

// -------------------------------------------------------------------------------------


static void print_int(unsigned long val, unsigned long invalid, int len)
{
	static char sz[32];
	static int i;
	if (val == invalid)
		strcpy(sz, "*******");
	else
		sprintf(sz, "%ld", val);
	sz[len] = 0;
	for (i=strlen(sz); i<len; ++i)
		sz[i] = ' ';
	if (len > 0)
		sz[len-1] = ' ';
	printf(sz);
}
#define abs(x) ((x)<0?-(x):(x))

static void print_float(float val, float invalid, int len, int prec)
{
	static char sz[32];
	static   int i;
	if (val == invalid)
	{
		strcpy(sz, "*******");
		sz[len] = 0;
		if (len > 0)
			sz[len-1] = ' ';
		for (i=7; i<len; ++i)
			sz[i] = ' ';
		printf(sz);
	}
	else
	{
		int vi;
		int flen;
		vi = abs((int)val);
		flen = prec + (val < 0.0 ? 2 : 1);
		printf("%f",val, prec);
		flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
		for (i=flen; i<len; ++i)
			printf(" ");
	}
}

static void print_date()
{
	static int year;
	static byte month, day, hour, minute, second, hundredths;
	static unsigned long age;
	crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
	if (age == GPS_INVALID_AGE)
		printf("*******    *******    ");
	else
	{
		char sz[32];
		sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d   ",
				month, day, year, hour, minute, second);
		printf(sz);
	}
	print_int(age, GPS_INVALID_AGE, 5);
}

static void print_str(const char *str, int len)
{
	static int i;
	static int slen;
	slen = strlen(str);
	for ( i=0; i<len; ++i)
		printf("%c",i<slen ? str[i] : ' ');
}

static void gpsdump()
{
	static float flat, flon, tmp;
	static unsigned long age, date, time, chars;
	static unsigned short sentences, failed;
	static const float LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
	chars = 0;
	sentences = 0;
	failed = 0;


	print_int(satellites(), GPS_INVALID_SATELLITES, 5);
	print_int(hdop(), GPS_INVALID_HDOP, 5);
	f_get_position(&flat, &flon, &age);
	print_float(flat, GPS_INVALID_F_ANGLE, 9, 5);
	print_float(flon, GPS_INVALID_F_ANGLE, 10, 5);
	print_int(age, GPS_INVALID_AGE, 5);

	print_date();

	print_float(f_altitude(), GPS_INVALID_F_ALTITUDE, 8, 2);
	print_float(f_course(), GPS_INVALID_F_ANGLE, 7, 2);
	print_float(f_speed_kmph(), GPS_INVALID_F_SPEED, 6, 2);
	print_str(f_course() == GPS_INVALID_F_ANGLE ? "*** " : cardinal(f_course()), 6);
	print_int(flat == GPS_INVALID_F_ANGLE ? 0UL : (unsigned long)distance_between(flat, flon, LONDON_LAT, LONDON_LON) / 1000, 0xFFFFFFFF, 9);
	print_float(flat == GPS_INVALID_F_ANGLE ? 0.0 : course_to(flat, flon, 51.508131, -0.128002), GPS_INVALID_F_ANGLE, 7, 2);
	tmp = course_to(flat, flon, LONDON_LAT, LONDON_LON);
	print_str(flat == GPS_INVALID_F_ANGLE ? "*** " : cardinal(tmp), 6);

	stats(&chars, &sentences, &failed);
	print_int(chars, 0xFFFFFFFF, 6);
	print_int(sentences, 0xFFFFFFFF, 10);
	print_int(failed, 0xFFFFFFFF, 9);
	puts("");
}

void puts0(const char * d, uint16_t size) {
	while( size-- && *d ) uart0_writeb(*d++);
	uart0_writeb('\n');
}
void printbyte0(unsigned char * d, uint16_t len){
	while(len--) {
		if( (*d&15) < 10 ) uart0_writeb((*d&0xf)+'0');
		else uart0_writeb((*d&0xf)-10+'A');
		if( (*d>>4) < 10 ) uart0_writeb((*d>>4)+'0');
		else uart0_writeb((*d>>4)-10+'A');
		d++;
	}
	uart0_writeb('\n');
}

#define BUF_SIZE 2000
char buf[BUF_SIZE];
uint16_t bufi = 0, bufe = 0;

int getc0(unsigned char x) {
	buf[bufe] = x;
	bufe = (bufe+1)%BUF_SIZE;
}

void timeout_handler(void) {
	curTime++;
	//if( curTime % ANNOUNCE_TIME == 0 ) announceMe();
	printf("-> %u %u\n", bufi, bufe);
	while( bufi != bufe ) {
		watchdog_periodic();
		encode(buf[bufi]);
		bufi = (bufi+1)%BUF_SIZE;
	}
	printf("<- %u %u\n", bufi, bufe);

	if( curTime % 5 == 0 ) gpsdump();
}

struct etimer et;
PROCESS_THREAD(gps_process, ev, data) {
	static uint8_t i;
	PROCESS_BEGIN();

	uart0_set_input(getc0);
	etimer_set(&et, CLOCK_SECOND);

	while(1) {
		PROCESS_YIELD();
		if(etimer_expired(&et)) {
			timeout_handler();
			etimer_restart(&et);
		}
	}

	PROCESS_END();
}
