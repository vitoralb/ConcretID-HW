/* 
    DemoDay.ino
    Autor: Felipe Salgueiro Oliveira e Silva
    27/03/2013
    
*/

#include <TinyGPS.h>
#define MONITOR_BAUDRATE 9600
#define GPS_BAUDRATE 9600
#define BUTTON_PIN 36
#define LED_PIN 37
#define BUZZER_PIN 38
TinyGPS gps;
unsigned long fix_age; // returns +- latitude/longitude in degrees
long latitude, longitude;

// Variables will change:
int ledState = HIGH;         // the current state of the output pin
int buttonState = LOW;             // the current reading from the input pin
int buttonStateBuffer = LOW;    // Identificar se o usuário demorou apertando o botão...
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers


void setup() {
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("InicioSerial0 ");

  Serial1.begin(GPS_BAUDRATE);   // Serial do GPS
  while (!Serial1) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("InicioSerial1 ");
  
  // initialize the LED pin as an output:
  pinMode(LED_PIN, OUTPUT);      
  // initialize the pushbutton pin as an input:
  pinMode(BUTTON_PIN, INPUT);
  // initialize the BUZZER pin as an output:
  pinMode(BUZZER_PIN, OUTPUT);   
}

void loop(){
  // read the state of the switch into a local variable:
  int reading = digitalRead(BUTTON_PIN);

  // check to see if you just pressed the button ------- DEBOUNCE
  // (i.e. the input went from LOW to HIGH),  and you've waited 
  // long enough since the last press to ignore any noise:  

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;
      buttonStateBuffer = reading;
    }
  }
  
 

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;
  
  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH and IT IS A TRASITION!
  if (buttonState == HIGH && buttonStateBuffer == LOW) {     
    // turn LED on:    
    digitalWrite(BUZZER_PIN, HIGH);
    delay (500);
    digitalWrite(BUZZER_PIN, LOW); 
    
    // Ler do GPS
    while (Serial1.available())
    {
      int c = Serial1.read();
      if (gps.encode(c))
      {
        // process new gps info here
        gps.get_position(&latitude, &longitude, &fix_age);
        if (fix_age == TinyGPS::GPS_INVALID_AGE){
          Serial.println("No fix detected");
          digitalWrite(LED_PIN, LOW); 
        }
        else if (fix_age > 5000){
          Serial.println("Warning: possible stale data!");
          digitalWrite(LED_PIN, LOW); 
        }
        else
        {
          Serial.println("Data is current.");
          Serial.print("Latitude:  "); 
          Serial.println(latitude);
          Serial.print("Longitude:  "); 
          Serial.println(longitude);
          digitalWrite(LED_PIN, HIGH);
          
          // Enviar pela Serial0 para o CC2530
        }
      }
    }  
  }  
}
