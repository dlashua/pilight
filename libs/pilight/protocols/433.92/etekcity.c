/*
	Copyright (C) 2013 CurlyMo

	This file is part of pilight.

	pilight is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	pilight is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "etekcity.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	160
#define MAX_PULSE_LENGTH	180
#define AVG_PULSE_LENGTH	175
#define RAW_LENGTH				50

static int validate(void) {
	if(etekcity_switch->rawlen == RAW_LENGTH) {
		if(etekcity_switch->raw[etekcity_switch->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   etekcity_switch->raw[etekcity_switch->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int unit, int state) {
	etekcity_switch->message = json_mkobject();
	json_append_member(etekcity_switch->message, "id", json_mknumber(id, 0));
	json_append_member(etekcity_switch->message, "unit", json_mknumber(unit, 0));
        if(state == 1) {
                json_append_member(etekcity_switch->message, "state", json_mkstring("on"));
        } else {
                json_append_member(etekcity_switch->message, "state", json_mkstring("off"));
        }
}

static void parseCode(void) {
	int x = 0, binary[RAW_LENGTH/2];

	/* Convert the one's and zero's into binary */
	for(x=0;x<etekcity_switch->rawlen-2;x+=2) {
		if(etekcity_switch->raw[x+0] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[x/2]=1;
		} else {
			binary[x/2]=0;
		}
	}

	int unit = binToDecRev(binary, 0, 9);
	int state = binary[23];
	int id = 0;
	if(binary[10] == 1) {
		id = 5;
	}
	if(binary[12] == 1) {
		id = 4;
	}
	if(binary[14] == 1) {
		id = 3;
	}
	if(binary[16] == 1) {
		id = 2;
	}
	if(binary[18] == 1) {
		id = 1;
	}


	createMessage(id, unit, state);
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		etekcity_switch->raw[i]=(AVG_PULSE_LENGTH);
		etekcity_switch->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		etekcity_switch->raw[i+0]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		etekcity_switch->raw[i+1]=(AVG_PULSE_LENGTH);
	}
}

static void clearCode(void) {
	createLow(0,49);
}

static void createUnit(int unit) {
	int binary[255];
	int start = 0;
	int end = start + 9;
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(unit, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x = (end - i) * 2;
			createHigh(x, x+1);
		}
	}
}

static void createId(int id) {
	int binary[255];
	int start = 10;
	int end = start + 9;
	int length = 0;
	int i=0, x=0;
	int convid = 0;

	if(id == 1) {
		convid = 339;
	}
	if(id == 2) {
		convid = 348;
	}
	if(id == 3) {
		convid = 368;
        }
        if(id == 4) {
                convid = 464;
        }
	if(id == 5) {
		convid = 848;
	}
	
	length = decToBinRev(convid, binary);
	for(i=0;i<=length;i++) {
		x = (end - i) * 2;
		if(binary[i]==1) {
			createHigh(x,x+1);
		}
	}
}

static void createState(int state) {
	int start = 20;
	int x = 0;

	if(state == 1) {
		x = (start + 2) * 2;
		createHigh(x,x+3);
	}
	if(state == 0) {
		x = (start) * 2;
		createHigh(x,x+3);

	}
}

static void createFooter(void) {
	etekcity_switch->raw[48]=(AVG_PULSE_LENGTH);
	etekcity_switch->raw[49]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code) {
	int id = -1;
	int unit = -1;
	int state = -1;
	double itmp;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;
	if(json_find_number(code, "unit", &itmp) == 0)
		unit = (int)round(itmp);

	if(id == -1 || unit == -1 || state == -1) {
		logprintf(LOG_ERR, "etekcity_switch: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(id < 0 || id > 5) {
		logprintf(LOG_ERR, "etekcity_switch: invalid id range");
		return EXIT_FAILURE;
	} else if(unit < 1 || unit > 99) {
		logprintf(LOG_ERR, "etekcity_switch: invalid unit range");
		return EXIT_FAILURE;
	} else {
		createMessage(id, unit, state);
		clearCode();
		createUnit(unit);
		createState(state);
		createId(id);
		createFooter();
		etekcity_switch->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
	printf("\t -u --unit=unit\t\t\tcontrol a device with this unit code\n");
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void etekcitySwitchInit(void) {

	protocol_register(&etekcity_switch);
	protocol_set_id(etekcity_switch, "etekcity_switch");
	protocol_device_add(etekcity_switch, "etekcity_switch", "ETEK City Switches");
	etekcity_switch->devtype = SWITCH;
	etekcity_switch->hwtype = RF433;
	etekcity_switch->minrawlen = RAW_LENGTH;
	etekcity_switch->maxrawlen = RAW_LENGTH;
	etekcity_switch->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	etekcity_switch->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&etekcity_switch->options, 't', "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&etekcity_switch->options, 'f', "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&etekcity_switch->options, 'u', "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^[0-9]{2}$");
	options_add(&etekcity_switch->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^[1-5]{1}$");

	options_add(&etekcity_switch->options, 0, "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&etekcity_switch->options, 0, "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	etekcity_switch->parseCode=&parseCode;
	etekcity_switch->createCode=&createCode;
	etekcity_switch->printHelp=&printHelp;
	etekcity_switch->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "etekcity_switch";
	module->version = "0.1";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	etekcitySwitchInit();
}
#endif
