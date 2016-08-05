#ifndef MEX_CLK_H
#define MEX_CLK_H

// Alarm functions
void enableRtcAlarm();
void disableRtcAlarm();
void setRtcAlarm(byte hour, byte minute);
void stopAlarmCallback();
byte isRtcAlarmOn();
void playAlarmSong();

// Display functions
void updateTime();
void updateAlarm();
void updateTemperature();
int maxValueForDigit(int digit);

// User IO functions
void implClickA(int value);
void doubleClickA();
void singleClickA();
void longPressA();
void implClickB(int value);
void doubleClickB();
void singleClickB();
void longPressB();

// debug functions
void printDigits(int digits, char separator);
void digitalClockDisplay();
void rtcStatus();
void printAlarmStatus();

#endif