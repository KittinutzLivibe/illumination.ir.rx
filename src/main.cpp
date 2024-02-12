#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

const uint16_t kRecvPin = 14;
IRrecv irrecv(kRecvPin);
decode_results results;

bool awaitingSecondPart = false;
uint64_t receivedLow = 0, receivedHigh = 0;
const uint32_t DELIMITER_MASK = 0x80000000; // Delimiter bit mask (example, adjust as needed)

void setup()
{
  Serial.begin(115200);
  irrecv.enableIRIn();
  Serial.println(F("IR Receiver ready"));
}
void handlerPayload(uint64_t payload)
{
  uint8_t id = (message >> 32) & 0xFF;
  uint8_t mode = (message >> 24) & 0xFF;
  uint32_t color = message & 0xFFFFFF;

  Serial.print("ID: ");
  Serial.print(id);
  Serial.print(", Mode: ");
  Serial.print(mode);
  Serial.print(", Color: #");
  Serial.println(color, HEX);
}

void loop()
{
  if (irrecv.decode(&results))
  {
    if (results.decode_type == decode_type_t::NEC_LIKE)
    {
      if (!awaitingSecondPart)
      {
        Serial.println(results.value);
        if (results.value & DELIMITER_MASK)
        {
          receivedLow = results.value & ~DELIMITER_MASK; // Remove delimiter from data
          awaitingSecondPart = true;
        }
        else
        {
          Serial.println("Invalid transmission: Missing delimiter");
        }
      }
      else
      {
        receivedHigh = results.value;

        // Combine receivedLow and receivedHigh for complete 64-bit payload
        uint64_t fullPayload = (receivedHigh << 32) | receivedLow;

        // Process fullPayload here
        Serial.println("Full payload received: 0x" + String(fullPayload, HEX));

        awaitingSecondPart = false;
        handlerLED(fullPayload);
      }
    }
    irrecv.resume(); // Ready for next message
  }
}