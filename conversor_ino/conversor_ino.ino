// the setup routine runs once when you press reset:

#include "getCoordGPS.h"

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}


// the loop routine runs over and over again forever:
void loop() {

  double latitude = -8.055621;
  double longitude = -34.951431;
  
  Coordenadas_GPS coord;
  getCoordGPS (latitude,longitude, &coord);
  
  delay (5000);
 
}
