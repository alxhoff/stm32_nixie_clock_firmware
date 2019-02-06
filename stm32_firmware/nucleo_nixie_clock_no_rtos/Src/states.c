/*
 * states.c
 *
 *  Created on: 21 Jan 2019
 *      Author: alxhoff
 */

#include "states.h"
#include "error.h"

//states
#include "draw.h"

#include <string.h>
#include <stdlib.h>

typedef struct state state_t;
struct state {
	unsigned char id;

	void* data;

	char* name;

	unsigned char initd;

	unsigned char (*probe)(void);

	void (*enter)(void);
	void (*run)(void);
	void (*exit)(void);
};

typedef struct state_machine {
	state_t *current_state;
	state_t *next_state;

	state_t **states;
	unsigned char count;
} state_machine_t;

state_machine_t state_machine_dev = { 0 };

unsigned char states_run(void) {
	if (state_machine_dev.next_state->id
			!= state_machine_dev.current_state->id) {
		if (state_machine_dev.current_state->exit) /* Exit current state*/
			(state_machine_dev.current_state->exit)();
		if (state_machine_dev.next_state->enter) /* Enter next state*/
			(state_machine_dev.next_state->enter)();
		state_machine_dev.current_state = state_machine_dev.next_state; /* Change states*/
	}

	if (state_machine_dev.current_state->run) /* Run current state*/
		(state_machine_dev.current_state->run)();

	return 0;
}

static unsigned char states_add(unsigned char (*probe)(void),
		void (*enter)(void), void (*run)(void), void (*exit)(void), state_e id,
		char* name) {
	unsigned char error = 0;
	state_t *ret = calloc(1, sizeof(state_t));
	if (!ret)
		return -ENOMEM;

	ret->enter = enter;
	ret->run = run;
	ret->exit = exit;
	ret->probe = probe;

	ret->id = id;
	ret->name = malloc(sizeof(char) * (strlen(name) + 1));
	if (!ret->name)
		return -ENOMEM;
	strcpy(ret->name, name);

	state_machine_dev.states = realloc(state_machine_dev.states,
			sizeof(state_t*) * (state_machine_dev.count + 1));
	state_machine_dev.states[state_machine_dev.count] = ret;
	state_machine_dev.count++;

	error = ret->probe();
	if (error)
		return -ENOINIT;

	ret->initd = 1;

	return 0;
}

void *get_state_data(void) {
	return state_machine_dev.current_state->data;
}

void set_state_data(void *data) {
	state_machine_dev.current_state->data = data;
}

static unsigned char states_init_states(void) {

	for (int i = 0; i < state_machine_dev.count; i++)
		if (!state_machine_dev.states[i]->initd) {
			if ((state_machine_dev.states[i]->probe)())
				return -ENOINIT;
			else
				state_machine_dev.states[i]->initd = 1;
		}

	return 0;
}

#define SET_INITIAL_STATE( STATE )	\
			state_machine_dev.current_state = state_machine_dev.states[STATE];	\
			state_machine_dev.next_state = state_machine_dev.states[STATE];

unsigned char states_init(void) {

	//send state
	states_add(draw_time_init, draw_time_enter, draw_time_run, draw_time_exit,
			state_time, "Show time");
	states_add(draw_set_time_init, draw_set_time_enter, draw_set_time_run,
			draw_set_time_exit, state_time_set, "Set time");
	states_add(draw_alarm1_init, draw_alarm1_enter, draw_alarm1_run,
			draw_alarm1_exit, state_alarm_1_set, "Set alarm 1");
	states_add(draw_alarm2_init, draw_alarm2_enter, draw_alarm2_run,
			draw_alarm2_exit, state_alarm_2_set, "Set alarm 2");

	//set initial state
	SET_INITIAL_STATE(state_time);

	states_init_states();
	return 0;
}