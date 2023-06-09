#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"
String command;
MAX30105 particleSensor;
#define MAX_BRIGHTNESS 255
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
uint16_t irBuffer[100]; //infrared LED sensor data
uint16_t redBuffer[100];  //red LED sensor data
#else
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
#endif
int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid
byte pulseLED = 11; //Must be on PWM pin
byte readLED = 13; //Blinks with each data read
#include <Wire.h>
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
#define DEBUG1 1
#define DEBUG2 1
void setup()
{
  Serial.begin(115200);
  pinMode(pulseLED, OUTPUT);
  pinMode(readLED, OUTPUT);
  #if DEBUG1
  // max30102/5
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  byte ledBrightness = 60; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
  #endif
  #if DEBUG2
  // max90614
  mlx.begin();
  #endif
}
void loop()
{
  if (Serial.available()) {
    command = Serial.readStringUntil('\n');
    command.trim();
    if (command.equals("MAX")) 
    {
        #if DEBUG1
        // max30102/5
        bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps
        maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
        //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
        while (1)
        {
          //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
          for (byte i = 25; i < 100; i++)
          {
            redBuffer[i - 25] = redBuffer[i];
            irBuffer[i - 25] = irBuffer[i];
          }
          //take 25 sets of samples before calculating the heart rate.
          for (byte i = 75; i < 100; i++)
          {
            while (particleSensor.available() == false) //do we have new data?
              particleSensor.check(); //Check the sensor for new data
            //digitalWrite(readLED, !digitalRead(readLED)); //Blink onboard LED with every data read
            redBuffer[i] = particleSensor.getRed();
            irBuffer[i] = particleSensor.getIR();
            particleSensor.nextSample(); //We're finished with this sample so move to next sample
            //send samples and calculation result to terminal program through UART
            Serial.print(F(", HR="));
            Serial.print(heartRate, DEC);
            Serial.print(F(", HRvalid="));
            Serial.print(validHeartRate, DEC);
            Serial.print(F(", SPO2="));
            Serial.print(spo2, DEC);
            Serial.print(F(", SPO2Valid="));
            Serial.println(validSPO2, DEC);            
          }
          //After gathering 25 new samples recalculate HR and SP02
          maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
        }
        #endif          
    }
  }
  if (Serial.available()) {
    command = Serial.readStringUntil('\n');
    command.trim();
    if (command.equals("MLX")) {
      #if DEBUG2
      // max90614
      Serial.print("Ambient = ");
      Serial.print(mlx.readAmbientTempF());
      Serial.print("Object = ");
      Serial.print(mlx.readObjectTempF());
      Serial.println();
      delay(500);
      #endif
    }
  }
}
