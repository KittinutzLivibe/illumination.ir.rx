#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

const uint16_t kRecvPin = 14;
TaskHandle_t Task1;

IRrecv irrecv(kRecvPin);
decode_results results;

bool awaitingSecondPart = false;
uint64_t receivedLow = 0, receivedHigh = 0;
const uint32_t DELIMITER_MASK = 0x4000000; // Delimiter bit mask (example, adjust as needed)

// LED

#define LED_R 25
#define LED_G 26
#define LED_B 27
const uint8_t RED_CHANNEL = 0;   // Adjust channel based on LEDC configuration
const uint8_t GREEN_CHANNEL = 1; // Adjust channel based on LEDC configuration
const uint8_t BLUE_CHANNEL = 2;  // Adjust channel based on LEDC configuration

const uint16_t FADE_DURATION = 60000; // Adjust interval (500ms in this case)
const uint32_t FADE_STEPS = 255;      // Number of steps for smooth fading
uint64_t fullPayload = 0;

void handleIr(void *pvParameters)
{
  for (;;)
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
          fullPayload = (receivedHigh << 32) | receivedLow;
          // handlerLED(fullPayload);
          // Process fullPayload here
          Serial.println("Full payload received: 0x" + String(fullPayload, HEX));

          awaitingSecondPart = false;
        }
      }

      irrecv.resume(); // Ready for next message
    }
  }
}

void handlerLED(uint64_t payload)
{
  if (payload > 0)
  {
    Serial.println(" payload received: 0x" + String(payload, HEX));

    uint8_t id = (payload >> 40) & 0xFF;
    uint8_t mode = (payload >> 32) & 0xFF;
    uint32_t color = (payload >> 8) & 0xFFFFFF;
    uint8_t bpm = payload & 0xff;
    uint8_t isFade = 1;
    Serial.println(bpm);
    uint8_t R = (color >> 16) & 0xFF;
    uint8_t G = (color >> 8) & 0xFF;
    uint8_t B = color & 0xFF;
    for (int duty = 0; duty <= FADE_STEPS; duty++)
    {
      // Calculate duty cycle for each color based on current step
      int redDuty = map(duty, 0, FADE_STEPS, 0, R);
      int greenDuty = map(duty, 0, FADE_STEPS, 0, G);
      int blueDuty = map(duty, 0, FADE_STEPS, 0, B);

      // Set duty cycle for each channel
      ledcWrite(RED_CHANNEL, redDuty);
      ledcWrite(GREEN_CHANNEL, greenDuty);
      ledcWrite(BLUE_CHANNEL, blueDuty);
      unsigned long waitTime = ((FADE_DURATION / bpm) / FADE_STEPS) / 2;
      unsigned long startTime = millis();
      while (millis() - startTime < waitTime)
      {
        // Yield to other tasks while waiting
        yield();
      }
    }

    if (isFade)
    {
      // Fade down with specified steps and delay
      for (int duty = FADE_STEPS; duty >= 0; duty--)
      {
        // Calculate duty cycle for each color based on current step
        int redDuty = map(duty, 0, FADE_STEPS, 0, R);
        int greenDuty = map(duty, 0, FADE_STEPS, 0, G);
        int blueDuty = map(duty, 0, FADE_STEPS, 0, B);

        // Set duty cycle for each channel
        ledcWrite(RED_CHANNEL, redDuty);
        ledcWrite(GREEN_CHANNEL, greenDuty);
        ledcWrite(BLUE_CHANNEL, blueDuty);
        unsigned long waitTime = ((FADE_DURATION / bpm) / FADE_STEPS) / 2;
        unsigned long startTime = millis();
        while (millis() - startTime < waitTime)
        {
          // Yield to other tasks while waiting
          yield();
        }
      }
    }

    Serial.print("ID: ");
    Serial.print(id);
    Serial.print(", Mode: ");
    Serial.print(mode);
    Serial.print(", Color: #");
    Serial.println(color, HEX);
    Serial.print(", bpm: ");

    Serial.println(bpm, HEX);
  }
}

void setup()
{

  Serial.begin(115200);

  irrecv.enableIRIn();
  Serial.println(F("IR Receiver ready"));
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  ledcSetup(RED_CHANNEL, 5000, 8);
  ledcSetup(GREEN_CHANNEL, 5000, 8);
  ledcSetup(BLUE_CHANNEL, 5000, 8);

  // Attach LED pins to channels
  ledcAttachPin(LED_R, RED_CHANNEL);
  ledcAttachPin(LED_G, GREEN_CHANNEL);
  ledcAttachPin(LED_B, BLUE_CHANNEL);

  // Set initial duty cycle to 0 (off)
  ledcWrite(RED_CHANNEL, 0);
  ledcWrite(GREEN_CHANNEL, 0);
  ledcWrite(BLUE_CHANNEL, 0);
  xTaskCreatePinnedToCore(
      handleIr, /* Task function. */
      "Task1",  /* name of task. */
      10000,    /* Stack size of task */
      NULL,     /* parameter of the task */
      1,        /* priority of the task */
      &Task1,   /* Task handle to keep track of created task */
      1);
}

void loop()
{
  while (1)
  {
    handlerLED(fullPayload);
  }
}