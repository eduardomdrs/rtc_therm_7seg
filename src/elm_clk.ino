
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
#define SHOW_ALARM_MODE 4
#define ERROR_MODE      5

#define SHOW_TIME_DURATION 7000
#define SHOW_TEMP_DURATION 3000
#define ONE_MINUTE         60000

// ---------------------- //
//  Alarm variables
// ---------------------- //
#define SONG_DURATION 64

int notePosition = 0;

int melody[] = {1319, 0, 1319, 0, 1319, 0, 1319, 0, 1976, 0, 1976, 0, 1976,
 0, 1976, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1976, 0, 1976, 0, 1976, 0, 
 1976, 0, 2349, 0, 2349, 0, 2349, 0, 2349, 0, 1760, 0, 1760, 0, 1760, 0,
  1760, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0, 1760, 0,
   1760, 0};

int noteDurations[] = {63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63,
 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42,
  63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42,
   63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42, 63, 42};

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
unsigned long lastAlarmTrigger  = 0;
unsigned long lastShowTimeStart = 0;
unsigned long lastShowTempStart = 0;

SevenSegController display(DIGIT0_PIN, DIGIT1_PIN, DIGIT2_PIN, DIGIT3_PIN, 
	COLON_PIN, DEGREE_PIN, LATCH_PIN, DATA_PIN, CLOCK_PIN);
OneButton buttonA(BUTTON_A_PIN, true);
OneButton buttonB(BUTTON_B_PIN, true);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);
DeviceAddress devAddr;

// ----------------------------- //
//  RTC alarm functions
// ----------------------------- //
void enableRtcAlarm()
{
	RTC.enableAlarm(ALARM_0, ALM_MATCH_HOURS);
	RTC.enableAlarm(ALARM_1, ALM_MATCH_MINUTES);
	//RTC.alarmPolarity(LOW);

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

void setRtcAlarm(byte hour, byte minute)
{	
	// copy time_t object, modify some fields, 
	// inherit month, day, dayOfWeek and Year.
	time_t justNow = now();
	tmElements_t alarmSetting;
	breakTime(justNow, alarmSetting);

	alarmSetting.Hour = hour;
	alarmSetting.Minute = minute;
	alarmSetting.Second = 0;
	RTC.setAlarm(ALARM_0, makeTime(alarmSetting));
	RTC.setAlarm(ALARM_1, makeTime(alarmSetting));

	// save alarm setting to parts of SRAM, easier retrieval
	RTC.sramWrite(ALARM_H_ADDR, hour);
	RTC.sramWrite(ALARM_M_ADDR, minute);
}

void stopAlarmCallback()
{
	oldFsmState = SHOW_ALARM_MODE;
	fsmState    = SHOW_TIME_MODE;
	disableRtcAlarm();
	notePosition = 0;
	lastAlarmTrigger = millis();
}

byte isRtcAlarmOn()
{
	byte alarmOn = RTC.sramRead(ALARM_ON_ADDR);
	return alarmOn;
}

void playAlarmSong()
{	
	int noteDuration = noteDurations[notePosition];	

	if (melody[notePosition])
	{
		tone(A2, melody[notePosition]);
		delay(noteDuration);
		noTone(A2);
	} else
	{
		delay(noteDuration);	
	}
	notePosition++;
	// roll over, once finished;
	notePosition %= SONG_DURATION; 
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
void implClickA(int value)
{
	switch (fsmState)
	{
		case EDIT_TIME_MODE:
		case EDIT_ALARM_MODE:
			display.disableBlink(activeDigit);
			activeDigit += value;
			activeDigit %= N;
			display.enableBlink(activeDigit);
			break;

		case SHOW_ALARM_MODE:
			stopAlarmCallback();
			break;
	}
}

void doubleClickA()
{
	implClickA(2);
}

void singleClickA()
{
	implClickA(1);
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

				// printAlarmStatus();
			} else 
			{
				enableRtcAlarm();
				for (int i = 0; i < N; i++)
					display.enableDecimalPoint(i);

				// printAlarmStatus();
			}
			break;

		case SHOW_TIME_MODE:
		case SHOW_TEMP_MODE:
			fsmState = EDIT_TIME_MODE;
			break;


		case SHOW_ALARM_MODE:
			stopAlarmCallback();
			break;
	}
}

void implClickB(int value)
{
	switch (fsmState)
	{
		case EDIT_TIME_MODE:
		case EDIT_ALARM_MODE:
			digitValues[activeDigit] += value;
			digitValues[activeDigit] %= maxValueForDigit(activeDigit);
			display.writeDigit(activeDigit, digitValues[activeDigit]);
			break;

		case SHOW_ALARM_MODE:
			stopAlarmCallback();
			break;
	}
}

void doubleClickB()
{
	implClickB(2);
}

void singleClickB()
{
	implClickB(1);
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
			// printAlarmStatus();
  			fsmState = SHOW_TIME_MODE;
  			break;

		case SHOW_TIME_MODE:
		case SHOW_TEMP_MODE:
			fsmState = EDIT_ALARM_MODE;
			break;

		case SHOW_ALARM_MODE:
			stopAlarmCallback();
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
		fsmState = ERROR_MODE;
	} else
	{
		Serial.println("RTC has set the system time"); 
	}
	
	// default alarm settings, 08:30, disabled
	pinMode(ALARM_PIN, INPUT_PULLUP);
	setRtcAlarm(0,0);
	disableRtcAlarm();
}

void loop()
{
	// If error detected, disable buttons.
	if (fsmState != ERROR_MODE)
	{
		buttonA.tick();
		buttonB.tick();
	}

	// If alarm condition is detected, modify FSM state accordingly
	// When an alarm is triggered, the alarm pin is pulled low.
	// if (RTC.alarm(ALARM_0) && RTC.alarm(ALARM_1) && !alarmTriggeredToday)
	if (RTC.alarm(ALARM_0) && RTC.alarm(ALARM_1))
	{
		oldFsmState = fsmState;
		// update the time, so the display is not stuck in garbage.
		updateTime();
		display.enableClockDisplay();
		fsmState    = SHOW_ALARM_MODE;
	}

	// After an alarm is cleared by the user, wait for a minute
	// and re-enable the alarm to provide for repeatable alarms
	// everyday without user intervention.
	if ((millis() - lastAlarmTrigger) > ONE_MINUTE && lastAlarmTrigger)
	{
		enableRtcAlarm();
		lastAlarmTrigger = 0;
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

		case SHOW_ALARM_MODE:
			display.disableDisplay();
			playAlarmSong();
			display.enableDisplay();
			break;

		case ERROR_MODE:
			if (oldFsmState != fsmState)
			{	
				display.enableNumericDisplay();
				display.writeMessage("ERRO");
				Serial.println("Error detected, disabling buttons.");
				Serial.println("Error detected, disabling RTC alarm.");
				disableRtcAlarm();
			}
			break;

		default:
			break;
	}
}

// -------------------------------------- //
//  Debug helper functions
// -------------------------------------- //
void printDigits(int digits, char separator)
{
  // Utility function for digital clock display: prints preceding 
  // separator and leading 0
  Serial.print(separator);
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void digitalClockDisplay()
{
	time_t t = now();
	printDigits(hour(t)  ,' ');
	printDigits(minute(t),':');
	printDigits(second(t),':');
	Serial.print(" ");
	Serial.print(day(t));
	Serial.print(" ");
	Serial.print(month(t));
	Serial.print(" ");
	Serial.print(year(t)); 
	Serial.println(); 
}

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

// Debug statements, print new alarm / status to serial port.
void printAlarmStatus()
{
	byte h, m, rtcOn;
	Serial.print("New alarm time: ");
	h = RTC.sramRead(ALARM_H_ADDR);
	m = RTC.sramRead(ALARM_M_ADDR);
	printDigits(h,' ');
	printDigits(m,':');
	rtcOn = isRtcAlarmOn();
	if (rtcOn)
		Serial.println(" -- ON");
	else
		Serial.println(" -- OFF");
}
