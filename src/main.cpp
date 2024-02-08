#include "Arduino.h"
#include <EEPROM.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
const uint16_t kRecvPin = 14;
#define LED_RED 25
#define LED_GREEN 26
#define LED_BLUE 27
#define MEM_SIZE 24
int i = 0;
#define DELAY_AFTER_SEND 2000
#define DELAY_AFTER_LOOP 5000

IRrecv irrecv(kRecvPin);

decode_results results;
void handleIllumination()
{
  int R = EEPROM.readByte(0);
  int G = EEPROM.readByte(1);
  int B = EEPROM.readByte(2);
  // Serial.printf("RED-->%d\n", R);
  // Serial.printf("GREEN-->%d\n", G);
  // Serial.printf("BLUE-->%d\n", B);
  // Serial.print("\n");
  analogWrite(LED_RED, R);
  analogWrite(LED_GREEN, G);
  analogWrite(LED_BLUE, B);
}

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(MEM_SIZE);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  EEPROM.writeByte(0, 0xff);
  EEPROM.writeByte(1, 0x00);
  EEPROM.writeByte(2, 0x00);
  EEPROM.commit();
  Serial.print("Success setup \n");
  irrecv.enableIRIn(); // Start the receiver
  while (!Serial)      // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();
  Serial.print("IRrecvDemo is now running and waiting for IR message on Pin ");
  Serial.println(kRecvPin);
}

void loop()
{
  // if (i == 3)
  // {
  //   int R = EEPROM.readByte(0);
  //   int G = EEPROM.readByte(1);
  //   int B = EEPROM.readByte(2);
  //   analogWrite(LED_RED, R);
  //   analogWrite(LED_GREEN, G);
  //   analogWrite(LED_BLUE, B);
  //   delay(1000);
  //   i = 0;
  // }
  // i++;
  handleIllumination();

  if (irrecv.decode(&results))
  {
    Serial.print(results.decode_type);
    Serial.print("\n");
    Serial.print(results.value, HEX);

    Serial.print("Received NEC: ");
    if (results.bits == 24)
    {
      // Assume the received value is an RGB color
      uint32_t rgbColor = results.value;
      uint8_t R = (rgbColor >> 16) & 0xFF;
      uint8_t G = (rgbColor >> 8) & 0xFF;
      uint8_t B = rgbColor & 0xFF;

      // Output the RGB value
      Serial.print("R: ");
      Serial.print(R);
      Serial.print(" G: ");
      Serial.print(G);
      Serial.print(" B: ");
      Serial.println(B);
      EEPROM.write(0, R);
      EEPROM.write(1, G);
      EEPROM.write(2, B);
      EEPROM.commit();
      // Here you can add code to set the RGB LED color
      // setRGBColor(red, green, blue);
    }
    else
    {
      Serial.println("Received non-24-bit data");
    }
    // uint8_t R = results.value >> 8 * 0;
    // uint8_t G = results.value >> 8 * 1;
    // uint8_t B = results.value >> 8 * 2;

    irrecv.resume(); // Receive the next value
  }
  delay(100);
}