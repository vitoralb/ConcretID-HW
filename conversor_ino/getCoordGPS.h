#ifndef GETCOORDGPS_H_
#define GETCOORDGPS_H_
#include "Arduino.h"


typedef struct coord {
  int lat_degree;
  int lat_minutes;
  float lat_seconds;
  int lon_degree;
  int lon_minutes;
  float lon_seconds;

} Coordenadas_GPS;


void getCoordGPS (double latitude, double longitude,  Coordenadas_GPS * coord_GPS){
 
  int lat_sign = 1;
  int lon_sign = 1;
  
  if (latitude < 0)
  {
     lat_sign = -1; 
  }
   if (longitude < 0)
  {
     lon_sign = -1; 
  }
  
  latitude *= lat_sign;  
  
  Serial.println("lat_degree: ");
  Serial.println((int)(lat_sign * latitude));
  double lat_minutes = (latitude - (int) latitude) * 60.0;
  coord_GPS->lat_minutes = (int) lat_minutes;
  Serial.println("lat_minutes: ");
  Serial.println(coord_GPS->lat_minutes);
  coord_GPS->lat_seconds = (coord_GPS->lat_minutes - lat_minutes) * 60.0;
  Serial.println("lat_seconds: ");
  Serial.println(coord_GPS->lat_seconds);
  Serial.println("");
  
  
  longitude *= lon_sign;  
  
 Serial.println("lon_degree: ");
  Serial.println((int)(lon_sign * longitude));
  double lon_minutes = (longitude - (int) longitude) * 60.0;
  coord_GPS->lon_minutes = (int) lon_minutes;
  Serial.println("lon_minutes: ");
  Serial.println(coord_GPS->lon_minutes);
  coord_GPS->lon_seconds = (coord_GPS->lon_minutes - lon_minutes) * 60.0;
  Serial.println("lon_seconds: ");
  Serial.println(coord_GPS->lon_seconds);
  Serial.println("");
 
  
  
}

#endif 
