/*
 * draw.c
 *
 *  Created on: Aug 8, 2017
 *      Author: alxhoff
 */

#include <string.h>

#include "screen.h"
#include "draw.h"
#include "RTC_dev.h"
#include "stm32f1xx_hal.h"
#include "states.h"
#include "SN54HC595.h"
#include "nixie.h"
#include "config.h"
#include "buttons.h"

uint8_t blink_flag = 0;

#ifdef SCREEN_ON
static char weekday[12] = {0};
static char date[12] = {0};
static char time[12] = {0};
#endif

void get_weekday_string(WEEKDAYS_e weekday, char *buf) {
	switch (weekday) {
	case MONDAY:
		strcpy(buf, "MON");
		break;
	case TUESDAY:
		strcpy(buf, "TUE");
		break;
	case WEDNESDAY:
		strcpy(buf, "WED");
		break;
	case THURSDAY:
		strcpy(buf, "THU");
		break;
	case FRIDAY:
		strcpy(buf, "FRI");
		break;
	case SATURDAY:
		strcpy(buf, "SAT");
		break;
	case SUNDAY:
		strcpy(buf, "SUN");
		break;
	default:
		strcpy(buf, "XXX");
		break;
	}
}

void get_month_string(MONTHS_e month, char *buf) {
	switch (month) {
	case JANUARY:
		strcpy(buf, "JAN");
		break;
	case FEBUARY:
		strcpy(buf, "FEB");
		break;
	case MARCH:
		strcpy(buf, "MAR");
		break;
	case APRIL:
		strcpy(buf, "APR");
		break;
	case MAY:
		strcpy(buf, "MAY");
		break;
	case JUNE:
		strcpy(buf, "JUN");
		break;
	case JULY:
		strcpy(buf, "JUL");
		break;
	case AUGUST:
		strcpy(buf, "AUG");
		break;
	case SEPTERMBER:
		strcpy(buf, "SEP");
		break;
	case OCTOBER:
		strcpy(buf, "OCT");
		break;
	case NOVEMBER:
		strcpy(buf, "NOV");
		break;
	case DECEMBER:
		strcpy(buf, "DEC");
		break;
	default:
		break;
	}
}

void get_alarm_date_string(char *buf, TIME_TYPE_e type) {
	unsigned char date;
	WEEKDAYS_e weekday;
	DAY_OR_DATE_e date_or_day;

	switch (type) {
	case ALARM_ONE:
		date_or_day = RTC_dev_alarm1_get_day_or_date();
		if (date_or_day == DAY_OF_WEEK) {
			weekday = RTC_dev_alarm1_get_weekday();
			get_weekday_string(weekday, buf);
		} else {
			date = RTC_dev_alarm1_get_date();
			sprintf(buf, "%d", date);
		}

		break;
	case ALARM_TWO:
		date_or_day = RTC_dev_alarm2_get_day_or_date();
		if (date_or_day == DAY_OF_WEEK) {
			weekday = RTC_dev_alarm2_get_weekday();
			get_weekday_string(weekday, buf);
		} else {
			date = RTC_dev_alarm2_get_date();
			sprintf(buf, "%d", date);
		}
		break;
	default:
		break;
	}
}

void get_alarm_time_string(char *buf, TIME_TYPE_e type) {
	unsigned char hour;
	unsigned char min;

	switch (type) {
	case ALARM_ONE:
		hour = RTC_dev_alarm1_get_hour();
		min = RTC_dev_alarm1_get_min();

		if (RTC_dev_alarm1_get_format() == HOUR_24)
			sprintf(buf, "%02d:%02d", hour, min);
		else if (RTC_dev_alarm1_get_am_pm() == AM)
			sprintf(buf, "%02d:%02d%s", hour, min, "AM");
		else
			sprintf(buf, "%02d:%02d%s", hour, min, "PM");
		break;
	case ALARM_TWO:
		hour = RTC_dev_alarm2_get_hour();
		min = RTC_dev_alarm2_get_min();

		if (RTC_dev_alarm2_get_format() == HOUR_24)
			sprintf(buf, "%02d:%02d", hour, min);
		else if (RTC_dev_alarm2_get_am_pm() == AM)
			sprintf(buf, "%02d:%02d%s", hour, min, "AM");
		else
			sprintf(buf, "%02d:%02d%s", hour, min, "PM");
		break;
	default:
		break;
	}
}

void get_date_string(char *buf) {
	unsigned char date = RTC_dev_time_get_date();
	MONTHS_e month = RTC_dev_time_get_month();
	unsigned short year = RTC_dev_time_get_year();

	char month_str[4] = { 0 };
	get_month_string(month, month_str);

	sprintf(buf, "%02d %s %04hu", date, month_str, year);

	return;
}

void get_time_weekday_string(char *buf) {
	WEEKDAYS_e weekday = RTC_dev_time_get_weekday();

	get_weekday_string(weekday, buf);
}

void get_time_string(char *buf) {
	unsigned char hour = RTC_dev_time_get_hour();
	unsigned char min = RTC_dev_time_get_min();
	unsigned char sec = RTC_dev_time_get_sec();

	if (RTC_dev_time_get_format() == HOUR_24)
		sprintf(buf, "%02d:%02d:%02d", hour, min, sec);
	else {
		if (RTC_dev_time_get_AM_or_PM() == AM)
			sprintf(buf, "%02d:%02d:%02d%s", hour, min, sec, "AM");
		else
			/* PM */
			sprintf(buf, "%02d:%02d:%02d%s", hour, min, sec, "PM");
	}
}

unsigned char if_cursor(void) {
	unsigned long ticks = HAL_GetTick() >> 9;
	if (ticks & 0x1)
		return 1;
	else
		return 0;
}

//STATES
void change_big_states(void) {
	unsigned char cur_state = states_get_state();

	switch (cur_state) {
	case state_time:
		states_set_state(state_time_set);
		break;
	case state_time_set:
#ifdef SCREEN_ON
		states_set_state(state_alarm_1_set);
		break;
	case state_alarm_1_set:
#endif
		states_set_state(state_time);
		break;
	default:
		break;
	}
	states_clear_input();
}
//TIME
void draw_time_run(void) {
#ifdef SCREEN_ON
	RTC_dev_get_time();
	get_time_weekday_string(weekday);
	get_date_string(date);
	get_time_string(time);

	screen_add_line_at_index(0, weekday);
	screen_add_line_at_index(1, date);
	screen_add_line_at_index(2, time);
#else

	nixie_split_set_digit(RTC_dev_time_get_min(), NIXIE_MIN_INDEX_LSB);
	nixie_split_set_digit(RTC_dev_time_get_hour(), NIXIE_HOUR_INDEX_LSB);
	//TODO reduce
	unsigned char *output = nixie_compile_output();
	SN54HC595_out_bytes((unsigned char *) output, CHECK_ODD(NIXIE_DEVICES));
#endif

	unsigned char input = states_get_input();
	if (input & 0x1)
		change_big_states();
	else if ((input >> 1) & 0x1)
		NULL;
	else if ((input >> 2) & 0x1)
		NULL;
	states_clear_input();
}

//SET TIME
#define HANDLE_BUTTONS(LEFT, CENTER, RIGHT)	\
		unsigned char input = states_get_input();\
		if((input >> 0) & 0x1)				\
			{LEFT;}							\
		else if((input >> 1) & 0x1)			\
			{CENTER;}							\
		else if((input >> 2) & 0x1)			\
			{RIGHT;}							\
		states_clear_input();

#ifdef SCREEN_ON
#define SET_TIME_DRAW_STATE(FROM, TO, LINE, LEFT_BUT, CENTER_BUT, RIGHT_BUT)	\
		get_time_weekday_string(weekday);	\
		get_date_string(date);		\
		get_time_string(time);		\
		strcpy( weekday + 3, " *");	\
		if(if_cursor())			\
			for(signed char i = FROM; i <= TO; i++)	\
				LINE[i] = '_';		\
		screen_add_line_at_index(0, weekday); 	\
		screen_add_line_at_index(1, date);		\
		screen_add_line_at_index(2, time);		\
		HANDLE_BUTTONS(LEFT_BUT,CENTER_BUT,RIGHT_BUT)
#else
#define	SET_TIME_DRAW_STATE_NIXIE(NIXIE_INDEX, LEFT_BUT, CENTER_BUT, RIGHT_BUT)	\
		nixie_split_set_digit(RTC_dev_time_get_min(), NIXIE_MIN_INDEX_LSB);		\
		nixie_split_set_digit(RTC_dev_time_get_hour(), NIXIE_HOUR_INDEX_LSB);	\
		unsigned char *output = nixie_compile_output();							\
		SN54HC595_out_bytes(output, CHECK_ODD(NIXIE_DEVICES));\
		if(if_cursor()){	\
			nixie_disable_tube(NIXIE_INDEX);		\
			nixie_disable_tube(NIXIE_INDEX + 1);	\
		}else{	\
			nixie_enable_tube(NIXIE_INDEX);			\
			nixie_enable_tube(NIXIE_INDEX + 1);		\
		}											\
		HANDLE_BUTTONS(LEFT_BUT,CENTER_BUT,RIGHT_BUT)
#endif


void change_time_state(void) {
	unsigned char cur_state = states_get_state();

	switch (cur_state) {
	case state_time_set:
#ifdef SCREEN_ON
		states_set_state(state_time_set_sec);
		break;
	case state_time_set_sec:
		states_set_state(state_time_set_min);
		break;
	case state_time_set_min:
		states_set_state(state_time_set_hour);
		break;
	case state_time_set_hour:
		states_set_state(state_time_set_date);
		break;
	case state_time_set_date:
		states_set_state(state_time_set_month);
		break;
	case state_time_set_month:
		states_set_state(state_time_set_year);
		break;
	case state_time_set_year:
		states_set_state(state_time_set_day);
		break;
	case state_time_set_day:
		states_set_state(state_time_set_sec);
		break;
#else
		states_set_state(state_time_set_min);
		break;
	case state_time_set_min:
		states_set_state(state_time_set_hour);
		break;
	case state_time_set_hour:
		states_set_state(state_time_set_min);
		break;
#endif
	default:
		break;
	}
	states_clear_input();
}

void draw_set_time_enter(void) {
	RTC_dev_get_time();
}

void draw_set_time_exit(void){
	RTC_dev_actualize();
#ifdef SCREEN_ON
	states_set_state(state_time_set);
#else
	states_set_state(state_time);
#endif
}

void draw_set_time_run(void) {
	RTC_dev_get_time();
#ifdef SCREEN_ON
	SET_TIME_DRAW_STATE(0, -1, time, change_big_states(), change_time_state(),
			NULL);
#else
	states_set_state(state_time_set_min);
#endif
}

void draw_set_time_min_run(void) {
#ifdef SCREEN_ON
	SET_TIME_DRAW_STATE(3, 4, time, draw_set_time_exit(),
			change_time_state(), RTC_dev_set_time_min_increment())
#else
//	SET_TIME_DRAW_STATE_NIXIE(NIXIE_MIN_INDEX_LSB, draw_set_time_exit(), change_time_state(),
//			RTC_dev_set_time_min_increment())
		nixie_split_set_digit(RTC_dev_time_get_min(), NIXIE_MIN_INDEX_LSB);
		nixie_split_set_digit(RTC_dev_time_get_hour(), NIXIE_HOUR_INDEX_LSB);
		unsigned char *output = nixie_compile_output();
		SN54HC595_out_bytes(output, CHECK_ODD(NIXIE_DEVICES));
		if(if_cursor()){
			nixie_disable_tube(NIXIE_MIN_INDEX_LSB);
			nixie_disable_tube(NIXIE_MIN_INDEX_LSB + 1);
		}else{	\
			nixie_enable_tube(NIXIE_MIN_INDEX_LSB);
			nixie_enable_tube(NIXIE_MIN_INDEX_LSB + 1);
		}
		unsigned char input = states_get_input();
		if((input >> 0) & 0x1){
			draw_set_time_exit();
			nixie_enable_all();
		}else if((input >> 1) & 0x1){
			change_time_state();
			nixie_enable_all();
		}else if((input >> 2) & 0x1)
			RTC_dev_set_time_min_increment();
		states_clear_input();
#endif
}

void draw_set_time_hour_run(void) {
#ifdef SCREEN_ON
	SET_TIME_DRAW_STATE(0, 1, time, draw_set_time_exit(),
			change_time_state(), RTC_dev_set_time_hour_increment())
#else
	SET_TIME_DRAW_STATE_NIXIE(NIXIE_HOUR_INDEX_LSB, draw_set_time_exit(); nixie_enable_all(),
				change_time_state(); nixie_enable_all(),
				RTC_dev_set_time_hour_increment())
#endif
}

#ifdef SCREEN_ON
void draw_set_time_sec_run(void) {
	SET_TIME_DRAW_STATE(6, 7, time, draw_set_time_exit(),
			change_time_state(), RTC_dev_set_time_sec_zero())
}

void draw_set_time_date_run(void) {
	SET_TIME_DRAW_STATE(0, 1, date, draw_set_time_exit(),
			change_time_state(), RTC_dev_set_time_date_increment())
}

void draw_set_time_month_run(void) {
	SET_TIME_DRAW_STATE(3, 5, date, draw_set_time_exit(),
			change_time_state(), RTC_dev_set_time_month_increment())
}

void draw_set_time_year_run(void) {
	SET_TIME_DRAW_STATE(7, 10, date, draw_set_time_exit(),
			change_time_state(), RTC_dev_set_time_year_increment())
}

void draw_set_time_day_run(void) {
	SET_TIME_DRAW_STATE(0, 2, weekday, draw_set_time_exit(),
			change_time_state(), RTC_dev_set_time_day_increment())
}
#endif

#ifdef SCREEN_ON
#define SET_ALARM_DRAW_STATE(FROM, TO, LINE, LEFT_BUT, CENTER_BUT, RIGHT_BUT)	\
		RTC_dev_get_alarm(ALARM_ONE);	\
		get_alarm_date_string(date, ALARM_ONE);		\
		get_alarm_time_string(time, ALARM_ONE);		\
		strcpy( date + 3, " *");					\
		if(if_cursor())		\
			for(signed char i = FROM; i <= TO; i++)	\
				LINE[i] = '_';		\
		screen_add_line_at_index(1, date);		\
		screen_add_line_at_index(2, time);		\

//SET ALARM 1
void draw_alarm1_run(void) {
	volatile unsigned char input = states_get_input();

	RTC_dev_get_alarm(ALARM_ONE);
	get_alarm_date_string(date, ALARM_ONE);
	get_alarm_time_string(time, ALARM_ONE);
	strcpy( date + 3, " *");

	screen_add_line_at_index(1, date);
	screen_add_line_at_index(2, time);

	if ((input >> 0) & 0x1)
		change_big_states();
	else if ((input >> 1) & 0x1)
		NULL;
	else if ((input >> 2) & 0x1)
		NULL;
	states_clear_input();
}

void draw_alarm1_min_run(void) {
	SET_ALARM_DRAW_STATE(3, 4, time, NULL, NULL, NULL)
}

void draw_alarm1_hour_run(void) {
	SET_ALARM_DRAW_STATE(0, 1, time, NULL, NULL, NULL)
}

void draw_alarm1_day_run(void) {
	SET_ALARM_DRAW_STATE(0, 2, date, NULL, NULL, NULL)
}
#endif
