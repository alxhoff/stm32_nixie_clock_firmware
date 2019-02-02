/*
 * screen.c
 *
 *  Created on: Sep 16, 2017
 *      Author: alxhoff
 */

#include "error.h"
#include "screen_API.h"
#include "config.h"

#ifdef FREERTOS
#include "cmsis_os.h"
#endif

#include <stdlib.h>
#include <string.h>

typedef struct screen_device {
	int rows;
	int cols;

#ifdef FREERTOS
	SemaphoreHandle_t framebuffer_lock;
	SemaphoreHandle_t cursor_lock;
	osThreadId refresh_task;
	TimerHandle_t cursor_timer;
#endif

	char **framebuffer;

	unsigned char cursor_on;
	int cursor_period;
	int cursor_location;

	void (*draw_text)(char **);
	void (*draw_cursor)(unsigned char);
	void (*clear_screen)(void);
	unsigned char (*update_screen)(void);
	void (*mv_cursor_left)(void);
	void (*mv_cursor_right)(void);
} screen_device_t;

screen_device_t screen_dev = { .cursor_period = SCREEN_CURSOR_PERIOD,
		.draw_text = &SCREEN_DRAW_TEXT, .draw_cursor = &SCREEN_DRAW_CURS,
		.clear_screen = &SCREEN_CLEAR, .update_screen = &SCREEN_REFRESH,
		.mv_cursor_left = &SCREEN_MV_CUR_LEFT, .mv_cursor_right =
				&SCREEN_MV_CUR_RIGHT };

#ifdef FREERTOS
void screen_cursor_callback(TimerHandle_t timer) {
	xSemaphoreTake(screen_dev.cursor_lock, portMAX_DELAY);
	screen_dev.cursor_on = !screen_dev.cursor_on;
	xSemaphoreGive(screen_dev.cursor_lock);
}
#endif

void screen_set_framebuffer(char **fb) {
	screen_dev.framebuffer = fb;
}

void screen_refresh(void const *args) {
#ifdef FREERTOS
	TickType_t xLastWakeTime = xTaskGetTickCount();
	TickType_t xPeriod = 20;

	while (1) {
		xSemaphoreTake(screen_dev.framebuffer_lock, portMAX_DELAY);
		xSemaphoreTake(screen_dev.cursor_lock, portMAX_DELAY);
#endif
	screen_dev.clear_screen();
	screen_dev.draw_text(screen_dev.framebuffer);
	screen_dev.draw_cursor(screen_dev.cursor_on);
	screen_dev.update_screen();
#ifdef FREERTOS
	xSemaphoreGive(screen_dev.cursor_lock);
	xSemaphoreGive(screen_dev.framebuffer_lock);

	xPeriod = SCREEN_PERIOD - (xLastWakeTime - xTaskGetTickCount());
	vTaskDelayUntil(&xLastWakeTime, xPeriod);
}
#endif
}

unsigned char screen_init(void) {
	screen_dev.rows = SCREEN_GET_ROWS
	;
	screen_dev.cols = SCREEN_GET_COLS
	;

	screen_dev.framebuffer = calloc(screen_dev.rows * screen_dev.cols,
			sizeof(char));
	if (!screen_dev.framebuffer)
		goto buffer_error;

#ifdef FREERTOS
	screen_dev.cursor_timer = xTimerCreate("Cursor Timer",
			screen_dev.cursor_period, 1, NULL, screen_cursor_callback);
	if (!screen_dev.cursor_timer)
	goto timer_error;

	screen_dev.cursor_lock = xSemaphoreCreateMutex();
	if (!screen_dev.cursor_lock)
	goto c_lock_error;

	screen_dev.framebuffer_lock = xSemaphoreCreateMutex();
	if (!screen_dev.framebuffer_lock)
	goto f_lock_error;

	xTimerStart(screen_dev.cursor_timer, 0);

	osThreadDef(screenTask, screen_refresh, osPriorityNormal, 0, 128);
	screen_dev.refresh_task = osThreadCreate(osThread(screenTask), NULL);
#endif
	return 0;

#ifdef FREERTOS
	f_lock_error:
	vSemaphoreDelete(screen_dev.cursor_lock);
	c_lock_error:
	xTimerDelete(screen_dev.cursor_timer, portMAX_DELAY);
	timer_error: free(screen_dev.framebuffer);
#endif
	buffer_error: return -ENOINIT;
}

void screen_add_line(char *line) {
	if (screen_dev.framebuffer[screen_dev.rows - 1]) /* If the screen is full prepare for shift up */
		free(screen_dev.framebuffer[screen_dev.rows - 1]);

	for (unsigned char i = screen_dev.rows - 1; i > 0; i--)
		screen_dev.framebuffer[i] = screen_dev.framebuffer[i - 1];

	screen_dev.framebuffer[0] = malloc(sizeof(char) * (strlen(line) + 1));
	strcpy(screen_dev.framebuffer[0], line);
}
