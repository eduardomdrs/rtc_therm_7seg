#include <DallasTemperature.h>
#include <MCP79412RTC.h>
#include <OneButton.h>
#include <OneWire.h>
#include <Time.h>
#include <TimerOne.h>
#include <Wire.h>

#include "SevenSegController.h"

// ---------------------- //
//  display control pins
// ---------------------- //
#define DIGIT0_PIN 3
#define DIGIT1_PIN 9
#define DIGIT2_PIN 10
#define DIGIT3_PIN 11
#define COLON_PIN  5
#define DEGREE_PIN 6
#define LATCH_PIN  8
#define CLOCK_PIN  2
#define DATA_PIN   7

// ---------------------- //
//  Thermometer
// ---------------------- //
#define ONE_WIRE_BUS 4

// ---------------------- //
//  button pins
// ---------------------- //
#define BUTTON_A_PIN A0
#define BUTTON_B_PIN A1

// ---------------------- //
//  alarm pins / addr
// ---------------------- //
#define ALARM_PIN A3
#define ALARM_H_ADDR  0x20
#define ALARM_M_ADDR  0x21
#define ALARM_ON_ADDR 0x22

// ---------------------- //
//  FSM definitions
// ---------------------- //
#define EDIT_TIME_MODE  0
#define EDIT_ALARM_MODE 1
#define SHOW_TIME_MODE  2
#define SHOW_TEMP_MODE  3
#define ERROR_MODE      4


#define SHOW_TIME_DURATION 7000
#define SHOW_TEMP_DURATION 3000

// ---------------------- //
//  Common definitions
// ---------------------- //
#define N 4 // number of LCD digits

// ---------------------- //
//  Globals
// ---------------------- //
byte oldFsmState    = 255;
byte fsmState       = 0;
byte activeDigit    = 0;
byte digitValues[N] = {0,0,0,0};
int  tempInCelsius  = 0;
unsigned long updateInterval    = 100;
unsigned long lastTempRead      = 0;
unsigned long lastClockRead     = 0;
unsigned long lastShowTimeStart = 0;
unsigned long lastShowTempStart = 0;

SevenSegController display(DIGIT0_PIN, DIGIT1_PIN, DIGIT2_PIN, DIGIT3_PIN, COLON_PIN, DEGREE_PIN, LATCH_PIN, DATA_PIN, CLOCK_PIN);
OneButton buttonA(BUTTON_A_PIN, true);
OneButton buttonB(BUTTON_B_PIN, true);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);
DeviceAddress devAddr;

// ----------------------------- //
//  RTC control/test functions
// ----------------------------- //
void rtcStatus()
{
  timeStatus_t rtcSta = timeStatus();
  
  if(rtcSta == timeSet)
    Serial.println("Time's clock has been set.");
  else if (rtcSta == timeNotSet)
    Serial.println("Time's clock has not been set.");
  else if (rtcSta == timeNeedsSync)
    Serial.println("Time's clock is set, but the sync has failed.");
  else
    Serial.println("error");
}

void enableRtcAlarm()
{
	RTC.enableAlarm(ALARM_0, ALM_MATCH_HOURS);
	RTC.enableAlarm(ALARM_1, ALM_MATCH_MINUTES);
	RTC.alarmPolarity(LOW);

	// save status to SRAM, easier retrieval
	RTC.sramWrite(ALARM_ON_ADDR, 1);
}

void disableRtcAlarm()
{
	RTC.enableAlarm(ALARM_0, ALM_DISABLE);
	RTC.enableAlarm(ALARM_1, ALM_DISABLE);
	
	// save status to SRAM, easier retrieval
	RTC.sramWrite(ALARM_ON_ADDR, 0);
}

byte isRtcAlarmOn()
{
	byte alarmOn = RTC.sramRead(ALARM_ON_ADDR);
	return alarmOn;
}

void setRtcAlarm(byte hour, byte minute)
{	
	tmElements_t hourSetting;
	hourSetting.Hour = hour;
	RTC.setAlarm(ALARM_0, makeTime(hourSetting));

	tmElements_t minuteSetting;
	minuteSetting.Minute = minute;
	RTC.setAlarm(ALARM_1, makeTime(minuteSetting));

	// save alarm setting to parts of SRAM, easier retrieval
	RTC.sramWrite(ALARM_H_ADDR, hour);
	RTC.sramWrite(ALARM_M_ADDR, minute);
}

// Debug statements, print new alarm / status to serial port.
void printAlarmStatus()
{
	byte h, m, rtcOn;
	Serial.print("New alarm time: ");
	h = RTC.sramRead(ALARM_H_ADDR);
	Serial.print(h);
	Serial.print(":");
	m = RTC.sramRead(ALARM_M_ADDR);
	Serial.print(m);
	rtcOn = isRtcAlarmOn();
	if (rtcOn)
		Serial.println(" -- ON");
	else
		Serial.println(" -- OFF");
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
}

void updateAlarm()
{
	int h = RTC.sramRead(ALARM_H_ADDR);
	int m = RTC.sramRead(ALARM_M_ADDR);

	digitValues[0] = h / 10;
	digitValues[1] = h % 10;
	digitValues[2] = m / 10;
	digitValues[3] = m % 10;

	display.writeDigit(0, digitValues[0]);
	display.writeDigit(1, digitValues[1]);
	display.writeDigit(2, digitValues[2]);
	display.writeDigit(3, digitValues[3]);
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

void printDigits(int digits)
{
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
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

int maxValueForDigit(int digit)
{
	int maxDigit = 0;
	switch (digit)
	{
		case 0:
			if (digitValues[1] >= 4)
			{
				maxDigit = 2;
			} else
			{
				maxDigit = 3;
			}
			
			break;

		case 1:
			if (digitValues[0] <= 1)
			{
				maxDigit = 10;
			} else
			{
				maxDigit = 4;
			}
			break;

		case 2:
			maxDigit = 6;
			break;

		case 3:
			maxDigit = 10;
			break;
	}

	return maxDigit;
}

// ---------------------- //
//  Button callbacks
// ---------------------- //
void playSong()
{
	display.disableDisplay();
	int melody[] = {1319, 0, 1319, 0, 1319, 0, 1319, 0, 1976, 0, 1976, 0, 1976, 0, 1976, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1976, 0, 1976, 0, 1976, 0, 1976, 0, 2349, 0, 2349, 0, 2349, 0, 2349, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0};

	int noteDurations[] = {63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42};

	// iterate over the notes of the melody:
	for (int thisNote = 0; thisNote < 64; thisNote++)
	{
		int noteDuration = noteDurations[thisNote];

		if (melody[thisNote])
		{
			tone(A2, melody[thisNote]);
			delay(noteDuration);
			noTone(A2);
		} else
		{
			delay(noteDuration);	
		}		
	}
	display.enableDisplay();
}

void doubleClickA()
{
	if (fsmState == EDIT_TIME_MODE || fsmState == EDIT_ALARM_MODE)
	{
		display.disableBlink(activeDigit);
		activeDigit+=2;
		activeDigit %= N;
		display.enableBlink(activeDigit);
	}	
}

void singleClickA()
{
	if (fsmState == EDIT_TIME_MODE || fsmState == EDIT_ALARM_MODE)
	{
		display.disableBlink(activeDigit);
		activeDigit++;
		activeDigit %= N;
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

		case EDIT_ALARM_MODE:
			if (isRtcAlarmOn())
			{
				disableRtcAlarm();
				for (int i = 0; i < N; i++)
					display.disableDecimalPoint(i);

				printAlarmStatus();
			} else 
			{
				enableRtcAlarm();
				for (int i = 0; i < N; i++)
					display.enableDecimalPoint(i);

				printAlarmStatus();
			}
			break;

		case SHOW_TIME_MODE:
			fsmState = EDIT_TIME_MODE;
			break;
	}
}

void doubleClickB()
{
	if (fsmState == EDIT_TIME_MODE || fsmState == EDIT_ALARM_MODE)
	{
		digitValues[activeDigit] += 2;
		digitValues[activeDigit] %= maxValueForDigit(activeDigit);
		display.writeDigit(activeDigit, digitValues[activeDigit]);
	}
}

void singleClickB()
{
	if (fsmState == EDIT_TIME_MODE || fsmState == EDIT_ALARM_MODE)
	{
		digitValues[activeDigit]++;
		digitValues[activeDigit] %= maxValueForDigit(activeDigit);
		display.writeDigit(activeDigit, digitValues[activeDigit]);
	} else if (fsmState == SHOW_TIME_MODE)
	{
		playSong();
	}
}

void longPressB()
{
	switch(fsmState)
	{
		case EDIT_ALARM_MODE:
			byte h, m;
			h = digitValues[0]*10 + digitValues[1];
			m = digitValues[2]*10 + digitValues[3];
			setRtcAlarm(h,m);
			printAlarmStatus();
  			fsmState = SHOW_TIME_MODE;
  			break;

		case SHOW_TIME_MODE:
			fsmState = EDIT_ALARM_MODE;
			break;
	}
}

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

	// initialize serial
	Serial.begin(9600);

	// initialize rtc
	setSyncProvider(RTC.get);
	setSyncInterval(2);
	if(timeStatus()!= timeSet) 
	{
		Serial.println("Unable to sync with the RTC");
	} else
	{
		Serial.println("RTC has set the system time"); 
		fsmState = ERROR_MODE;
	}
	
	pinMode(ALARM_PIN, INPUT_PULLUP);
	setRtcAlarm(8,30);
	disableRtcAlarm();
}

void loop()
{
	if (fsmState != ERROR_MODE)
	{
		buttonA.tick();
		buttonB.tick();
	}

	switch (fsmState)
	{
		case EDIT_TIME_MODE:

			if (oldFsmState != fsmState)
			{
				display.enableNumericDisplay();
				display.writeMessage("hora");
				delay(600);
				updateTime();
				display.enableClockDisplay();
				activeDigit = 0;
				display.enableBlink(activeDigit);
			}

			oldFsmState = fsmState;
			break;

		case EDIT_ALARM_MODE:
			
			if (oldFsmState != fsmState)
			{
				display.enableNumericDisplay();
				display.writeMessage("alarme");
				delay(600);
				updateAlarm();
				display.enableClockDisplay();

				if (isRtcAlarmOn())
				{
					for (int i = 0; i < N; i++)
						display.enableDecimalPoint(i);
				} else {
					for (int i = 0; i < N; i++)
						display.disableDecimalPoint(i);
				}

				activeDigit = 0;
				display.enableBlink(activeDigit);
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

			if ((millis() - lastClockRead) > updateInterval)
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

			if ((millis() - lastTempRead) > updateInterval)
			{
				updateTemperature();
				lastTempRead = millis();
			}

			if ((millis() - lastShowTempStart) > SHOW_TEMP_DURATION)
			{
				fsmState = SHOW_TIME_MODE;
			}

			break;

		case ERROR_MODE:
			if (oldFsmState != fsmState)
			{	
				display.writeMessage("ERRO");
				Serial.println("Error detected, disabling buttons.");
				Serial.println("Error detected, disabling RTC alarm.");
				disableRtcAlarm();
			}
			break;

		default:
			break;
	}

	delay(10);
}
