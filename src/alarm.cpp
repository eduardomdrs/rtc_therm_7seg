#include "alarm.h"

Alarm::Alarm()
{
	// fill in structure with current time
	time_t n = now();
	tmElements_t alarmSetting;
	breakTime(n, alarmSetting);

	// set alarm value to 00:00:00 (HH:MM:SS).
	alarmSetting.Hour   = 0;
	alarmSetting.Minute = 0;
	alarmSetting.Second = 0;

	setAlarmTime(makeTime(alarmSetting));
	disableAlarm();
}

void Alarm::enableAlarm()
{
	_enabled = true;
}

void Alarm::disableAlarm()
{
	_enabled = false;
}

void Alarm::setAlarmTime(time_t newTime)
{
	_alarmTime = newTime;
}

time_t Alarm::getAlarmTime()
{
	return _alarmTime;
}

bool Alarm::isTriggered(time_t currentTime)
{
	if (isEnabled())
	{
		tmElements_t hms_alarm;
		tmElements_t hms_time;
		breakTime(_alarmTime,  hms_alarm);
		breakTime(currentTime, hms_time);

		if (hms_alarm.Minute == hms_time.Minute &&
			hms_alarm.Hour == hms_time.Hour)
		{
			return true;
		}
	}

	return false;
}

bool Alarm::isEnabled()
{
	return _enabled;
}
