#ifndef ALARM_H
#define ALARM_H
#include <Time.h>

class Alarm
{
	public:
		Alarm();
		void enableAlarm();
		void disableAlarm();
		void setAlarmTime(time_t newTime);
		time_t getAlarmTime();
		bool isEnabled();
		bool isTriggered(time_t currentTime);

	private:
		time_t _alarmTime;
		bool   _enabled;
};

#endif
