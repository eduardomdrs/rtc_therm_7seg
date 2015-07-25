#include <TimerOne.h>
#include "SevenSegController.h"

// ---------------------- //
// display control pins
// ---------------------- //
#define DIGIT0_PIN 7
#define DIGIT1_PIN 6
#define DIGIT2_PIN 5
#define DIGIT3_PIN 4
#define COLON_PIN  3
#define DEGREE_PIN 2
#define LATCH_PIN  8
#define CLOCK_PIN 12
#define DATA_PIN  11 

SevenSegController controller(DIGIT0_PIN, DIGIT1_PIN, DIGIT2_PIN, DIGIT3_PIN, COLON_PIN, DEGREE_PIN, LATCH_PIN, DATA_PIN, CLOCK_PIN);

void setup()
{
}

void setRandomMode()
{
  int i;
  int j;

  for (i = 0; i < 4; i++)
  {
    j = random(3);
    switch(j)
    {
      case 0:
        controller.disableDigit(i);
        break;
      case 1:
        controller.enableDigit(i);
        break;
      case 2:
        controller.enableBlink(i);
        break;
    }
  }
}

void writeRandomNo()
{
  controller.writeDigit(0, random(10));
  controller.writeDigit(1, random(10));
  controller.writeDigit(2, random(10));
  controller.writeDigit(3, random(10));
}

void loop()
{
  controller.enableClockDisplay();
  controller.enableBlink(3);
  writeRandomNo();
  delay(3000);

  controller.enableTempDisplay();
  delay(3000);
}
