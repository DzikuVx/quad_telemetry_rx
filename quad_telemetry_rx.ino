
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "Wire.h"
#include "Adafruit_BMP085.h"
#include <Filters.h>
#include <LiquidCrystal_I2C.h>

Adafruit_BMP085 bmp;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

#define ledPin 5

//
// Hardware configuration
//
RF24 radio(9,10);

const uint64_t pipes[2] = { 0xe7e7e7e7e7LL, 0xc2c2c2c2c2LL };

typedef struct{
  int32_t pressure;
  float temperature;
  
} typeData;

typeData data;

int calibration;
bool bCalibrated;

FilterTwoPole pressureFilter; 

void setup(void)
{
  Serial.begin(57600);

  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setAutoAck(true);
  radio.setRetries(15,15);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();
  
  bmp.begin(2);
  
  bCalibrated = false;
  
  pressureFilter.setAsFilter( LOWPASS_BUTTERWORTH, 0.5);
  
  lcd.begin(16,2);
}

uint32_t basePressure;

int iteration = 0;
char buffer[16];

float computeAltitude(float base, float current) {
  float altitude;

  altitude = 44330 * (1.0 - pow(current / base,0.1903));

  return altitude; 
} 

void loop(void)
{
  
  pressureFilter.input(bmp.readPressure());
  
  if (iteration < 200) {
    iteration++;
  } else {

    /*
    Filter had some time to settle, so we can begin to read
    */
    
    if (radio.available()) {
      
      basePressure = (uint32_t) pressureFilter.output();
      
      radio.read( &data, sizeof(data));
      
      if (bCalibrated == false) {
        bCalibrated = true;
        calibration = basePressure - data.pressure;  
      }
      
      int diff;
      diff = basePressure - data.pressure - calibration;
         
      float altitude;
      
      altitude = computeAltitude(basePressure, data.pressure + calibration);
            
      Serial.println(altitude); 
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("alt: " + (String) dtostrf(altitude,4,1,buffer) + "m");
       
    }
  }
}

 
