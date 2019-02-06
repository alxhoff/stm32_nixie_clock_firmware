/*
 * RTC_dev.h
 *
 *  Created on: 2 Feb 2019
 *      Author: alxhoff
 */

#ifndef RTC_DEV_H_
#define RTC_DEV_H_

#include "ds3231.h"

signed char RTC_dev_init(unsigned char def_vals);
signed char RTC_dev_set_time(unsigned char hour, unsigned char min,
		unsigned char sec, TIME_FORMAT_e format, AM_OR_PM_e am_pm);
signed char RTC_dev_get_time(void);
signed char RTC_dev_set_alarm(TIME_TYPE_e alarm_number, unsigned char hour,
		unsigned char min, TIME_FORMAT_e format, AM_OR_PM_e am_pm,
		WEEKDAYS_e weekday, unsigned char date, DAY_OR_DATE_e date_or_day,
		ALARM_TYPE_e type);
signed char RTC_dev_get_alarm(TIME_TYPE_e alarm_number);
signed char RTC_dev_set_date(WEEKDAYS_e, unsigned char date, MONTHS_e month,
		unsigned short year);
signed char RTC_dev_read_date(void);

//GETS
TIME_FORMAT_e RTC_dev_time_get_format(void);
unsigned char RTC_dev_time_get_hour(void);
unsigned char RTC_dev_time_get_min(void);
unsigned char RTC_dev_time_get_sec(void);
AM_OR_PM_e RTC_dev_time_get_AM_or_PM(void);
WEEKDAYS_e RTC_dev_time_get_weekday(void);
unsigned char RTC_dev_time_get_date(void);
MONTHS_e RTC_dev_time_get_month(void);
unsigned short RTC_dev_time_get_year(void);
TIME_FORMAT_e RTC_dev_alarm1_get_format(void);
unsigned char RTC_dev_alarm1_get_min(void);
unsigned char RTC_dev_alarm1_get_hour(void);
AM_OR_PM_e RTC_dev_alarm1_get_am_pm(void);
WEEKDAYS_e RTC_dev_alarm1_get_weekday(void);
unsigned char RTC_dev_alarm1_get_date(void);
DAY_OR_DATE_e RTC_dev_alarm1_get_day_or_date(void);
ALARM_TYPE_e RTC_dev_alarm1_get_type(void);
unsigned char RTC_dev_alarm1_get_sec(void);
TIME_FORMAT_e RTC_dev_alarm2_get_twelve_hour(void);
unsigned char RTC_dev_alarm2_get_min(void);
unsigned char RTC_dev_alarm2_get_hour(void);
AM_OR_PM_e RTC_dev_alarm2_get_am_pm(void);
WEEKDAYS_e RTC_dev_alarm2_get_weekday(void);
unsigned char RTC_dev_alarm2_get_date(void);
DAY_OR_DATE_e RTC_dev_alarm2_get_day_or_date(void);
ALARM_TYPE_e RTC_dev_alarm2_get_type(void);

#endif /* RTC_DEV_H_ */