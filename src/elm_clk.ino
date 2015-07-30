#include <Wire.h>

#include <OneWire.h>
#include <TimerOne.h>
#include <DallasTemperature.h>

#include <OneButton.h>
#include <Time.h>
#include <MCP79412RTC.h>
#include "SevenSegController.h"

// ---------------------- //
//  display control pins
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

// ---------------------- //
//  button pins
// ---------------------- //
#define BUTTON_A_PIN A0
#define BUTTON_B_PIN A1

// ---------------------- //
//  FSM definitions
// ---------------------- //
#define EDIT_TIME_MODE 0
#define SHOW_TIME_MODE 1
#define SHOW_TEMP_MODE 2
#define ERROR		   3

#define SHOW_TIME_DURATION 6500
#define SHOW_TEMP_DURATION 3500

// ---------------------- //
//  RTC definitions
// ---------------------- //
#define NO_OF_DIGITS 4

// ---------------------- //
//  Thermometer
// ---------------------- //
#define ONE_WIRE_BUS 10

// ---------------------- //
//  Globals
// ---------------------- //
byte oldFsmState    = 255;
byte fsmState       = 0;
byte activeDigit    = 0;
byte digitValues[4] = {0,0,0,0};
int  tempInCelsius  = 0;
long updateInterval = 100;
long lastTempRead   = 0;
long lastClockRead  = 0;
long lastShowTimeStart = 0;
long lastShowTempStart = 0;

SevenSegController display(DIGIT0_PIN, DIGIT1_PIN, DIGIT2_PIN, DIGIT3_PIN, COLON_PIN, DEGREE_PIN, LATCH_PIN, DATA_PIN, CLOCK_PIN);
OneButton buttonA(BUTTON_A_PIN, false);
OneButton buttonB(BUTTON_B_PIN, false);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);
DeviceAddress devAddr;

void setup()
{
	fsmState = EDIT_TIME_MODE;

	// initialize thermometer
	sensor.begin();
	sensor.setWaitForConversion(true);
	sensor.getAddress(devAddr, 0);
	sensor.requestTemperatures();
	tempInCelsius = (int) (sensor.getTempC(devAddr)*10);

	// initialize buttons
	buttonA.setClickTicks(250);
	buttonA.setPressTicks(600);
	buttonA.attachLongPressStart(longPressA);
	buttonA.attachClick(singleClickA);
	buttonA.attachDoubleClick(doubleClickA);

	buttonB.setClickTicks(250);
	buttonB.setPressTicks(600);
	buttonB.attachClick(singleClickB);
	buttonB.attachDoubleClick(doubleClickB);
	buttonB.attachLongPressStart(longPressB);

	// initialize rtc
	// setTime(11, 25, 00, 29, 7, 2015);
  	// RTC.set(now());
	setSyncProvider(RTC.get);

	// initialize serial
	Serial.begin(9600);
}

void loop()
{
	buttonA.tick();
	buttonB.tick();

	switch (fsmState)
	{
		case EDIT_TIME_MODE:

			if (oldFsmState != fsmState)
			{
				display.enableClockDisplay();
				display.enableBlink(0);
			}

			oldFsmState = fsmState;
			break;

		case SHOW_TIME_MODE:

			if (oldFsmState != fsmState)
			{
				updateTime();
				lastClockRead = millis();
				lastShowTimeStart = lastClockRead;
				display.enableClockDisplay();
			}

			oldFsmState = fsmState;

			if ((millis() - lastClockRead) > 100)
			{
				updateTime();
				lastClockRead = millis();
			}

			if ((millis() - lastShowTimeStart) > SHOW_TIME_DURATION)
			{
				fsmState = SHOW_TEMP_MODE;
			}

			break;

		case SHOW_TEMP_MODE:
			
			if (oldFsmState != fsmState)
			{
				updateTemperature();
				lastTempRead = millis();
				lastShowTempStart = lastTempRead;
				display.enableTempDisplay();
			}

			oldFsmState = fsmState;

			if ((millis() - lastTempRead) > 100)
			{
				updateTemperature();
				lastTempRead = millis();
			}

			if ((millis() - lastShowTempStart) > SHOW_TEMP_DURATION)
			{
				fsmState = SHOW_TIME_MODE;
			}

			break;

		case ERROR:
		default:
			break;
	}

	delay(10);
}

// ---------------------- //
//  Display Update fnc
// ---------------------- //
void updateTime()
{
	time_t t = now();
	int h = hour(t);
	int m = minute(t);

	digitValues[0] = h / 10;
	digitValues[1] = h % 10;
	digitValues[2] = m / 10;
	digitValues[3] = m % 10;

	display.writeDigit(0, digitValues[0]);
	display.writeDigit(1, digitValues[1]);
	display.writeDigit(2, digitValues[2]);
	display.writeDigit(3, digitValues[3]);

	digitalClockDisplay();
}


void updateTemperature()
{
	sensor.requestTemperatures();
	tempInCelsius  = (int) (sensor.getTempC(devAddr)*10);    
	digitValues[0] = tempInCelsius / 100;
	digitValues[1] = (tempInCelsius % 100) / 10;
	digitValues[2] = tempInCelsius % 10;
	display.writeDigit(0,digitValues[0]);
	display.writeDigit(1,digitValues[1]);
	display.writeDigit(2,digitValues[2]);
}

void digitalClockDisplay()
{
	time_t t = now();
	Serial.print(hour(t));
	printDigits(minute(t));
	printDigits(second(t));
	Serial.print(" ");
	Serial.print(day(t));
	Serial.print(" ");
	Serial.print(month(t));
	Serial.print(" ");
	Serial.print(year(t)); 
	Serial.println(); 
}

void printDigits(int digits)
{
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

int maxValueForDigit(int digit)
{
	switch (digit)
	{
		case 0:
			if (digitValues[1] >= 4)
				return 2;
			else
				return 3;

		case 1:
			if (digitValues[0] <= 1)
				return 10;
			else
				return 4;

		case 2:
			return 6;

		case 3:
			return 10;
	}
}

// ---------------------- //
//  Button callbacks
// ---------------------- //
void doubleClickA()
{
	if (fsmState == EDIT_TIME_MODE)
	{
		display.disableBlink(activeDigit);
		activeDigit+=2;
		activeDigit %= NO_OF_DIGITS;
		display.enableBlink(activeDigit);
	}	
}

void singleClickA()
{
	if (fsmState == EDIT_TIME_MODE)
	{
		display.disableBlink(activeDigit);
		activeDigit++;
		activeDigit %= NO_OF_DIGITS;
		display.enableBlink(activeDigit);
	}
}

void longPressA()
{
	switch(fsmState)
	{
		case EDIT_TIME_MODE:
			int h;
			int m;
			h = digitValues[0]*10 + digitValues[1];
			m = digitValues[2]*10 + digitValues[3];
			setTime(h, m, 0, 1, 1, 2015);
  			RTC.set(now());
  			fsmState = SHOW_TIME_MODE;
  			break;

		case SHOW_TIME_MODE:
			fsmState = EDIT_TIME_MODE;
			break;
	}
}

void doubleClickB()
{
	if (fsmState == EDIT_TIME_MODE)
	{
		digitValues[activeDigit] += 2;
		digitValues[activeDigit] %= maxValueForDigit(activeDigit);
		display.writeDigit(activeDigit, digitValues[activeDigit]);
	}
}

void singleClickB()
{
	if (fsmState == EDIT_TIME_MODE)
	{
		digitValues[activeDigit]++;
		digitValues[activeDigit] %= maxValueForDigit(activeDigit);
		display.writeDigit(activeDigit, digitValues[activeDigit]);
	}
}

void longPressB()
{
	if (fsmState == EDIT_TIME_MODE)
	{
		digitValues[0] = 0; digitValues[1] = 0;
		digitValues[2] = 0; digitValues[3] = 0;
		display.writeDigit(0, 0);
		display.writeDigit(1, 0);
		display.writeDigit(2, 0);
		display.writeDigit(3, 0);
	}
}
