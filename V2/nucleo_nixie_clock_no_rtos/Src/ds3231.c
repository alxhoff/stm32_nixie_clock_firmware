/*
 * ds3231_stm32_alex.c
 *
 *  Created on: Aug 6, 2017
 *      Author: alxhoff
 */

#include <stdlib.h>
#include <string.h>

#include "ds3231.h"

typedef struct ds3231_registers {
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t week_day;
	uint8_t date;
	uint8_t month;
	uint16_t year;
	uint8_t alarm1_sec;
	uint8_t alarm1_min;
	uint8_t alarm1_hour;
	uint8_t alarm1_date;
	uint8_t alarm2_min;
	uint8_t alarm2_hour;
	uint8_t alarm2_date;
	uint8_t control;
	uint8_t status;
	uint8_t offset;
	uint8_t MSB_temp;
	uint8_t LSB_temp;
} ds3231_registers_t;

uint8_t dec2bcd(uint8_t d) {
	return ((d / 10 * 16) + (d % 10));
}

uint8_t bcd2dec(uint8_t b) {
	return ((b / 16 * 10) + (b % 16));
}

//OBJECT ORIENTATED END

void DS3231_write_time_short(I2C_HandleTypeDef *hi2c, uint8_t twelve_hour,
uint8_t hour, uint8_t min, uint8_t sec) {
	uint8_t write_buffer[3] = { 0 };
	write_buffer[0] = dec2bcd(sec);
	write_buffer[1] = dec2bcd(min);

	if (twelve_hour) {
		write_buffer[2] |= (1 << TWELVE_FLAG);
		if (hour >= 12) {
			write_buffer[2] |= (1 << PM_AM_FLAG);
			write_buffer[2] |= dec2bcd((hour & 0x1F) - 12);
		} else {
			write_buffer[2] &= ~(1 << PM_AM_FLAG);
			write_buffer[2] |= dec2bcd(hour & 0x1F);
		}
	} else {
		write_buffer[2] &= ~(1 << TWELVE_FLAG);
		write_buffer[2] |= dec2bcd(hour & 0x3F);
	}

	HAL_I2C_Mem_Write(hi2c, DS3231_ADDR8, 0x00, 1, write_buffer, 3, 10);
}

void DS3231_read_time_short(I2C_HandleTypeDef *hi2c, uint8_t* pm,
uint8_t* twelve_hour, uint8_t* hour, uint8_t*min, uint8_t* sec) {
	uint8_t read_buffer[3];
	uint8_t return_hour;

	HAL_I2C_Mem_Read(hi2c, DS3231_ADDR8, 0x00, 1, read_buffer, 3, 10);

	*sec = bcd2dec(read_buffer[0]);
	*min = bcd2dec(read_buffer[1]);
	return_hour = read_buffer[2];

	if (return_hour & (1 << TWELVE_FLAG)) {
		//12 hour
		*twelve_hour = TRUE;
		if (return_hour & (1 << PM_AM_FLAG)) {
			//PM
			*pm = TRUE;
			*hour = bcd2dec(return_hour & 0x1F);
		} else {
			//AM
			*pm = FALSE;
			*hour = bcd2dec(return_hour & 0x1F);
		}
	} else {
		//24 hour
		*twelve_hour = FALSE;
		*hour = bcd2dec(return_hour & 0x3F);
	}
}

signed char DS3231_write_date(I2C_HandleTypeDef *hi2c, uint16_t year,
uint8_t month, uint8_t date, uint8_t weekday) {
	uint8_t write_buffer[4];
	uint8_t century = 0;

	if (year > 2000) {
		century = 0x80;
		year -= 2000;
	} else {
		century = 0x00;
		year -= 1900;
	}

	write_buffer[0] = dec2bcd(weekday);
	write_buffer[1] = dec2bcd(date);
	write_buffer[2] = dec2bcd(month) | century;
	write_buffer[3] = dec2bcd(year);

	if (HAL_I2C_Mem_Write(hi2c, DS3231_ADDR8, 0x03, 1, write_buffer, 4, 10)
			!= HAL_OK)
		return -1;
	return 0;
}

signed char DS3231_read_date(I2C_HandleTypeDef *hi2c, uint16_t* year,
uint8_t* month, uint8_t* date, uint8_t* weekday) {
	uint8_t read_buffer[4];
	volatile uint8_t century = 0;

	if (HAL_I2C_Mem_Read(hi2c, DS3231_ADDR8, 0x03, 1, read_buffer, 4, 10)
			!= HAL_OK)
		return -1;

	century = ((read_buffer[2] & 0x80) >> 7);
	*year = (
			century == 1 ?
					(2000 + bcd2dec(read_buffer[3])) :
					(1900 + bcd2dec(read_buffer[3])));
	*month = bcd2dec(read_buffer[2] & 0x1F);
	*date = bcd2dec(read_buffer[1] & 0x3F);
	*weekday = bcd2dec(read_buffer[0]);

	return 0;
}

signed char DS3231_write_time(I2C_HandleTypeDef *hi2c, ds3231_time_t* time) {
	uint8_t write_buffer[7];
	uint8_t century = 0, year = time->year;
	uint8_t hour;

	if (time->year >= 2000) {
		century = 0x80;
		year -= 2000;
	} else {
		century = 0x00;
		year -= 1900;
	}

	if (time->twelve_hour) {
		//12 hour
		if (time->am_or_pm == PM) {
			//PM
			hour = dec2bcd(time->hour & 0x1F);
			//set flags
			hour |= (1 << PM_AM_FLAG) | (1 << TWELVE_FLAG);
		} else {
			//AM
			hour = dec2bcd(time->hour & 0x1F);
			//set flags
			hour |= (1 << TWELVE_FLAG);
			hour &= ~(1 << PM_AM_FLAG);
		}
	} else {
		//24 hour
		hour = dec2bcd(time->hour & 0x3F);
		//set flags
		hour &= ~(1 << TWELVE_FLAG);
	}

	write_buffer[0] = dec2bcd(time->sec);
	write_buffer[1] = dec2bcd(time->min);
	write_buffer[2] = hour;
	write_buffer[3] = dec2bcd(time->weekday);
	write_buffer[4] = dec2bcd(time->date);
	write_buffer[5] = (dec2bcd(time->month) | century);
	write_buffer[6] = dec2bcd(year);

	if (HAL_I2C_Mem_Write(hi2c, DS3231_ADDR8, 0x00, 1, write_buffer, 7, 10)
			!= HAL_OK)
		return -1;

	return 0;
}

signed char DS3231_read_time(I2C_HandleTypeDef *hi2c,
		ds3231_time_t* return_struct) {
	uint8_t read_buffer[7];
	uint8_t century = 0;
	uint8_t hour_byte;

	if (HAL_I2C_Mem_Read(hi2c, DS3231_ADDR8, 0x00, 1, read_buffer, 7, 10)
			!= HAL_OK)
		return -1;

	hour_byte = read_buffer[2];

	if (hour_byte & (1 << TWELVE_FLAG)) {
		//12 hour
		return_struct->twelve_hour = TRUE;
		if (hour_byte & (1 << PM_AM_FLAG)) {
			//PM
			return_struct->am_or_pm = PM;
		} else {
			//AM
			return_struct->am_or_pm = AM;
		}
		return_struct->hour = bcd2dec(hour_byte & 0x1F);
	} else {
		//24 hour
		return_struct->twelve_hour = FALSE;
		return_struct->am_or_pm = AM;
		return_struct->hour = bcd2dec(hour_byte & 0x3F);
	}

	return_struct->sec = bcd2dec(read_buffer[0]);
	return_struct->min = bcd2dec(read_buffer[1]);
	return_struct->weekday = bcd2dec(read_buffer[3]);
	return_struct->date = bcd2dec(read_buffer[4]);
	return_struct->month = bcd2dec(read_buffer[5] & 0x1F);
	century = (read_buffer[5] & 0x80) >> 7;
	return_struct->year =
			century == 1 ?
					(2000 + bcd2dec(read_buffer[6])) :
					(1900 + bcd2dec(read_buffer[6]));

	return 0;
}

//untested
signed char DS3231_read_temp(I2C_HandleTypeDef *hi2c, float *temp) {
	uint8_t read_buffer[2];

	if (HAL_I2C_Mem_Read(hi2c, DS3231_ADDR8, 0x11, 1, read_buffer, 2, 10)
			!= HAL_OK)
		return -1;

	read_buffer[1] = (read_buffer[1] >> 6) * 25;

	*temp = ((((short) read_buffer[0] << 8) | (short) read_buffer[1]) >> 6)
			/ 4.0f;

	return 0;
}

void DS3231_register_dump(I2C_HandleTypeDef *hi2c,
		ds3231_registers_t* return_struct) {
	uint8_t read_buffer[19];

	HAL_I2C_Mem_Read(hi2c, DS3231_ADDR8, 0x00, 1, read_buffer, 20, 10);

	return_struct->sec = read_buffer[0];
	return_struct->min = read_buffer[1];
	return_struct->hour = read_buffer[2];
	return_struct->week_day = read_buffer[3];
	return_struct->date = read_buffer[4];
	return_struct->month = read_buffer[5];
	return_struct->year = read_buffer[6];
	return_struct->alarm1_sec = read_buffer[7];
	return_struct->alarm1_min = read_buffer[8];
	return_struct->alarm1_hour = read_buffer[9];
	return_struct->alarm1_date = read_buffer[10];
	return_struct->alarm2_min = read_buffer[11];
	return_struct->alarm2_hour = read_buffer[12];
	return_struct->alarm2_date = read_buffer[13];
	return_struct->control = read_buffer[14];
	return_struct->status = read_buffer[15];
	return_struct->offset = read_buffer[16];
	return_struct->MSB_temp = read_buffer[17];
	return_struct->LSB_temp = read_buffer[18];
}

//untested
signed char DS3231_set_alarm_short(I2C_HandleTypeDef *hi2c, uint8_t twelve_hour,
uint8_t hour, uint8_t min, uint8_t sec, TYPE_TIME_t alarm_number) {
	uint8_t write_buffer[3] = { 0 };
	write_buffer[0] = dec2bcd(sec);
	write_buffer[1] = dec2bcd(min);

	if (twelve_hour) {
		write_buffer[2] |= (1 << TWELVE_FLAG);
		if (hour >= 12) {
			write_buffer[2] |= (1 << PM_AM_FLAG);
			write_buffer[2] |= dec2bcd((hour & 0x1F) - 12);
		} else {
			write_buffer[2] &= ~(1 << PM_AM_FLAG);
			write_buffer[2] |= dec2bcd(hour & 0x1F);
		}
	} else {
		write_buffer[2] &= ~(1 << TWELVE_FLAG);
		write_buffer[2] |= dec2bcd(hour & 0x3F);
	}

	if (HAL_I2C_Mem_Write(hi2c, DS3231_ADDR8, 0x07, 1, write_buffer, 3, 10)
			!= HAL_OK)
		return -1;
	return 0;
}

//untested
signed char DS3231_set_alarm(I2C_HandleTypeDef *hi2c,
		ds3231_alarm_t* alarm_time, TYPE_TIME_t alarm_number) {
	uint8_t alarm_register_addr = 0x07;
	volatile uint8_t day_date_byte = 0x00;
	uint8_t write_buffer[4];

//TODO last case
	switch (alarm_number) {
	case ALARM_ONE:
		alarm_register_addr = 0x07;
		break;
	case ALARM_TWO:
		alarm_register_addr = 0x0B;
		break;
	default:
		break;
	}

	write_buffer[0] = dec2bcd(alarm_time->sec) & 0x7F;
	write_buffer[1] = dec2bcd(alarm_time->min) & 0x7F;

	write_buffer[2] = 0x00;

	if (alarm_time->twelve_hour) {
		//12 hour
		if (alarm_time->pm == PM) {
			//PM
			write_buffer[2] = bcd2dec(alarm_time->hour & 0x1F);
			write_buffer[2] |= (1 << PM_AM_FLAG) | (1 << TWELVE_FLAG);
		} else {
			//AM
			write_buffer[2] = dec2bcd(alarm_time->hour & 0x1F);
			//set flags
			write_buffer[2] |= (1 << TWELVE_FLAG);
			write_buffer[2] &= ~(1 << PM_AM_FLAG);
		}
	} else {
		//24 hours
		write_buffer[2] = dec2bcd(alarm_time->hour & 0x3F);
		//set flags
		write_buffer[2] &= ~(1 << TWELVE_FLAG);
	}

	if (alarm_time->date_or_day == DAY_OF_MONTH) {
		//Day of month
		day_date_byte |= (1 << DY_DT_FLAG);
		day_date_byte |= dec2bcd(alarm_time->date & 0x3F);
	} else {
		//Day of week
		day_date_byte &= ~(1 << DY_DT_FLAG);
		day_date_byte |= dec2bcd(alarm_time->week_day & 0x0F);
	}

	write_buffer[3] = day_date_byte;

	if (alarm_number == ALARM_ONE
			&& alarm_time->alarm_type == ALARM_EVERY_SECOND) {
		write_buffer[0] |= (1 << ALARM_MASK_BITS);
		write_buffer[1] |= (1 << ALARM_MASK_BITS);
		write_buffer[2] |= (1 << ALARM_MASK_BITS);
		write_buffer[3] |= (1 << ALARM_MASK_BITS);
		if (HAL_I2C_Mem_Write(hi2c, DS3231_ADDR8, alarm_register_addr, 1,
				write_buffer, 3, 10) != HAL_OK)
			return -1;
		goto alarm_set;
	}

	switch (alarm_time->alarm_type) {
	case ALARM_MATCH_SECONDS:
		write_buffer[0] &= ~(1 << ALARM_MASK_BITS);
		write_buffer[1] |= (1 << ALARM_MASK_BITS);
		write_buffer[2] |= (1 << ALARM_MASK_BITS);
		write_buffer[3] |= (1 << ALARM_MASK_BITS);
		break;
	case ALARM_MATCH_MINUTES:
		write_buffer[0] &= ~(1 << ALARM_MASK_BITS);
		write_buffer[1] &= ~(1 << ALARM_MASK_BITS);
		write_buffer[2] |= (1 << ALARM_MASK_BITS);
		write_buffer[3] |= (1 << ALARM_MASK_BITS);
		break;
	case ALARM_MATCH_HOURS:
		write_buffer[0] &= ~(1 << ALARM_MASK_BITS);
		write_buffer[1] &= ~(1 << ALARM_MASK_BITS);
		write_buffer[2] &= ~(1 << ALARM_MASK_BITS);
		write_buffer[3] |= (1 << ALARM_MASK_BITS);
		break;
	case ALARM_MATCH_DATE_OR_DAY:
		write_buffer[0] &= ~(1 << ALARM_MASK_BITS);
		write_buffer[1] &= ~(1 << ALARM_MASK_BITS);
		write_buffer[2] &= ~(1 << ALARM_MASK_BITS);
		write_buffer[3] &= ~(1 << ALARM_MASK_BITS);
		break;
	default:
		break;

	}

	if (HAL_I2C_Mem_Write(hi2c, DS3231_ADDR8, alarm_register_addr, 1,
			write_buffer + sizeof(uint8_t), 4, 10) != HAL_OK)
		return -1;

	alarm_set: return 0;

}

//untested
signed char DS3231_get_alarm(I2C_HandleTypeDef *hi2c,
		ds3231_alarm_t* return_struct, TYPE_TIME_t alarm_number) {
	uint8_t read_buffer[4];
	volatile uint8_t alarm_register_addr = 0x00;

//TODO last case
	switch (alarm_number) {
	case ALARM_ONE:
		alarm_register_addr = 0x07;
		break;
	case ALARM_TWO:
		alarm_register_addr = 0x0A; /* first byte is ignored, alarm starts at 0x0B */
		break;
	default:
		break;
	}

	if (HAL_I2C_Mem_Read(hi2c, DS3231_ADDR8, alarm_register_addr, 1,
			read_buffer, 4, 10) != HAL_OK)
		return -1;

	if (alarm_number == ALARM_ONE)
		return_struct->sec = bcd2dec(read_buffer[0] & 0x7F);
	return_struct->min = bcd2dec(read_buffer[1] & 0x7F);

	if (read_buffer[2] & (1 << TWELVE_FLAG)) {
		//12 hour
		return_struct->twelve_hour = TRUE;
		if (read_buffer[2] & (1 << PM_AM_FLAG)) {
			//PM
			return_struct->pm = PM;
		} else {
			//AM
			return_struct->pm = AM;
		}
		return_struct->hour = bcd2dec(read_buffer[2] & 0x1F);
	} else {
		//24 hour
		return_struct->twelve_hour = FALSE;
		return_struct->pm = AM;
		return_struct->hour = bcd2dec(read_buffer[2] & 0x3F);
	}

	if (read_buffer[3] & (1 << DY_DT_FLAG)) {
		//day of month
		return_struct->date_or_day = DAY_OF_MONTH;
		return_struct->date = bcd2dec(read_buffer[3] & 0x3F);
	} else {
		//day of week
		return_struct->date_or_day = DAY_OF_WEEK;
		return_struct->week_day = bcd2dec(read_buffer[3] & 0x0F);
	}

	return 0;
}

//untested
void DS3231_enable_alarms(I2C_HandleTypeDef *hi2c, TYPE_TIME_t alarm_number) {
	uint8_t control_register = 0x00;

	HAL_I2C_Mem_Read(hi2c, DS3231_ADDR8, 0x0E, 1, &control_register, 1, 10);

	switch (alarm_number) {
	case ALARM_ONE:
		control_register |= (1 << ALARM1_CRTL_ENABLE);
		break;
	case ALARM_TWO:
		control_register |= (1 << ALARM2_CRTL_ENABLE);
		break;
	case BOTH:
		control_register |= (1 << ALARM1_CRTL_ENABLE);
		control_register |= (1 << ALARM2_CRTL_ENABLE);
		break;
	default:
		break;
	}

	HAL_I2C_Mem_Write(hi2c, DS3231_ADDR8, 0x0E, 1, &control_register, 1, 10);
}

//untested
void DS3231_enable_alarm_interrupt(I2C_HandleTypeDef *hi2c) {
	uint8_t control_register = 0x00;

	HAL_I2C_Mem_Read(hi2c, DS3231_ADDR8, 0x0E, 1, &control_register, 1, 10);

	control_register |= (1 << ALARM_INTERRUPT_ENABLE);

	HAL_I2C_Mem_Write(hi2c, DS3231_ADDR8, 0x0E, 1, &control_register, 1, 10);
}

//untested
void DS3231_disable_alarms(I2C_HandleTypeDef *hi2c, TYPE_TIME_t alarm_number) {
	uint8_t control_register = 0x00;

	HAL_I2C_Mem_Read(hi2c, DS3231_ADDR8, 0x0E, 1, &control_register, 1, 10);

	switch (alarm_number) {
	case ALARM_ONE:
		control_register &= ~(1 << ALARM1_CRTL_ENABLE);
		break;
	case ALARM_TWO:
		control_register &= ~(1 << ALARM2_CRTL_ENABLE);
		break;
	case BOTH:
		control_register &= ~(1 << ALARM1_CRTL_ENABLE);
		control_register &= ~(1 << ALARM2_CRTL_ENABLE);
		break;
	default:
		break;
	}

	HAL_I2C_Mem_Write(hi2c, DS3231_ADDR8, 0x0E, 1, &control_register, 1, 10);
}

//untested
void DS3231_disable_alarm_interrupt(I2C_HandleTypeDef *hi2c) {
	uint8_t control_register = 0x00;

	HAL_I2C_Mem_Read(hi2c, DS3231_ADDR8, 0x0E, 1, &control_register, 1, 10);

	control_register &= ~(1 << ALARM_INTERRUPT_ENABLE);

	HAL_I2C_Mem_Write(hi2c, DS3231_ADDR8, 0x0E, 1, &control_register, 1, 10);
}

//untested
void DS3231_change_wave_freq(I2C_HandleTypeDef *hi2c, WAVE_FREQ_t frequency) {
	uint8_t control_register = 0x00;

	HAL_I2C_Mem_Read(hi2c, DS3231_ADDR8, 0x0E, 1, &control_register, 1, 10);

	switch (frequency) {
	case ONE_K:
		control_register &= ~(1 << RS1 | 1 << RS2);
		break;
	case ONE_POINT_K:
		control_register |= (1 << RS1);
		control_register &= ~(1 << RS2);
		break;
	case FOUR_K:
		control_register &= ~(1 << RS1);
		control_register |= (1 << RS2);
		break;
	case EIGHT_K:
		control_register |= (1 << RS1 | 1 << RS2);
		break;
	default:
		break;
	}

	control_register |= (1 << ALARM_INTERRUPT_ENABLE);

	HAL_I2C_Mem_Write(hi2c, DS3231_ADDR8, 0x0E, 1, &control_register, 1, 10);
}

//untested
void DS3231_stop_triggered_alarms(I2C_HandleTypeDef *hi2c) {
	uint8_t status_register = 0x00;

	HAL_I2C_Mem_Read(hi2c, DS3231_ADDR8, 0x0F, 1, &status_register, 1, 10);

	status_register &= ~(1 << ALARM1_STATUS | 1 << ALARM1_STATUS);

	HAL_I2C_Mem_Write(hi2c, DS3231_ADDR8, 0x0F, 1, &status_register, 1, 10);
}
