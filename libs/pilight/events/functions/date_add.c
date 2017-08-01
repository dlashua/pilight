/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __USE_XOPEN
	#define __USE_XOPEN
#endif
#include <time.h>
#ifndef _WIN32
	#include <sys/time.h>
	#include <unistd.h>
#endif

#ifdef _WIN32
#include "../../core/strptime.h"
#endif

#include "../function.h"
#include "../events.h"
#include "../../protocols/protocol.h"
#include "../../core/options.h"
#include "../../core/log.h"
#include "../../core/dso.h"
#include "../../core/pilight.h"
#include "../../core/datetime.h"
#include "date_add.h"

#define NRUNITS 6

static struct units_t {
	char name[255];
	int id;
} units[] = {
	{ "SECOND", 1 },
	{ "MINUTE", 2 },
	{ "HOUR", 4 },
	{ "DAY", 8 },
	{ "MONTH", 16 },
	{ "YEAR", 32 }
};

static void add(struct tm *tm, int *values, int type) {
	if(type == -1) {
		return;
	}
	if(((1 << 0x5) & type) == (1 << 0x5)) {
		tm->tm_year += values[5];
	}
	if(((1 << 0x4) & type) == (1 << 0x4)) {
		tm->tm_mon += values[4];
	}
	if(((1 << 0x3) & type) == (1 << 0x3)) {
		tm->tm_mday += values[3];
		tm->tm_wday += values[3];
	}
	if(((1 << 0x2) & type) == (1 << 0x2)) {
		tm->tm_hour += values[2];
	}
	if(((1 << 0x1) & type) == (1 << 0x1)) {
		tm->tm_min += values[1];
	}
	if(((1 << 0x0) & type) == (1 << 0x0)) {
		tm->tm_sec += values[0];
	}
}

static int run(struct rules_t *obj, struct JsonNode *arguments, char **ret, enum origin_t origin) {
	struct JsonNode *childs = json_first_child(arguments);
	struct protocol_t *protocol = NULL;
	struct tm tm;
	char *p = NULL, *datetime = NULL, *interval = NULL, **array = NULL;
	int values[NRUNITS], error = 0;
	int l = 0, i = 0, type = -1, match = 0, is_dev = 0;

	if(ret == NULL || *ret == NULL) {
		error = -1;
		goto close;
	}

	p = *ret;

	memset(&values, 0, NRUNITS);

	if(childs == NULL) {
		logprintf(LOG_ERR, "DATE_ADD requires two parameters e.g. DATE_ADD(datetime, 1 DAY)");
		error = -1;
		goto close;
	}

	/*
	 * TESTME
	 */
	if(devices_select(origin, childs->string_, NULL) == 0) {
		is_dev = 1;
		if(origin == ORIGIN_RULE) {
			event_cache_device(obj, childs->string_);
		}
		if(devices_select_protocol(origin, childs->string_, 0, &protocol) == 0) {
			if(protocol->devtype == DATETIME) {
				char *setting = NULL;
				struct varcont_t val;
				i = 0;
				while(devices_select_settings(origin, childs->string_, i++, &setting, &val) == 0) {
					if(strcmp(setting, "year") == 0) {
						tm.tm_year = val.number_-1900;
					}
					if(strcmp(setting, "month") == 0) {
						tm.tm_mon = val.number_-1;
					}
					if(strcmp(setting, "day") == 0) {
						tm.tm_mday = val.number_;
					}
					if(strcmp(setting, "hour") == 0) {
						tm.tm_hour = val.number_;
					}
					if(strcmp(setting, "minute") == 0) {
						tm.tm_min = val.number_;
					}
					if(strcmp(setting, "second") == 0) {
						tm.tm_sec = val.number_;
					}
					if(strcmp(setting, "weekday") == 0) {
						tm.tm_wday = val.number_-1;
					}
					if(strcmp(setting, "dst") == 0) {
						tm.tm_isdst = val.number_;
					}
				}
			} else {
				logprintf(LOG_ERR, "device \"%s\" is not a datetime protocol", childs->string_);
				error = -1;
				goto close;
			}
		}
	} else {
		datetime = childs->string_;
	}

	childs = childs->next;
	if(childs == NULL) {
		if(is_dev == 0) {
			logprintf(LOG_ERR, "DATE_ADD requires two parameters e.g. DATE_ADD(2000-01-01 12:00:00, 1 DAY)");
		} else {
			logprintf(LOG_ERR, "DATE_ADD requires two parameters e.g. DATE_ADD(datetime, 1 DAY)");
		}
		error = -1;
		goto close;
	}
	interval = childs->string_;

	if(childs->next != NULL) {
		if(is_dev == 0) {
			logprintf(LOG_ERR, "DATE_ADD requires two parameters e.g. DATE_ADD(2000-01-01 12:00:00, 1 DAY)");
		} else {
			logprintf(LOG_ERR, "DATE_ADD requires two parameters e.g. DATE_ADD(datetime, 1 DAY)");
		}
		error = -1;
		goto close;
	}

	l = explode(interval, " ", &array);
	if(l == 2) {
		if(isNumeric(array[0]) == 0) {
			for(i=0;i<NRUNITS;i++) {
				if(strcmp(array[1], units[i].name) == 0) {
					values[i] = atoi(array[0]);
					type = units[i].id;
					match = 1;
					break;
				}
			}
		} else {
			logprintf(LOG_ERR, "The DATE_ADD unit parameter requires a number and a unit e.g. \"1 DAY\"");
			error = -1;
			goto close;
		}
	} else {
		logprintf(LOG_ERR, "The DATE_ADD unit parameter is formatted as e.g. \"1 DAY\"");
		error = -1;
		goto close;
	}

	if(match == 0) {
		logprintf(LOG_ERR, "DATE_ADD does not accept \"%s\" as a unit", array[1]);
		error = -1;
		goto close;
	}

	if(is_dev == 0) {
		if(strptime(datetime, "%Y-%m-%d %H:%M:%S", &tm) == NULL) {
			logprintf(LOG_ERR, "DATE_ADD requires the datetime parameter to be formatted as \"%%Y-%%m-%%d %%H:%%M:%%S\"");
			error = -1;
			goto close;
		}
	}
	add(&tm, values, type);

	int year = tm.tm_year+1900;
	int month = tm.tm_mon+1;
	int day = tm.tm_mday;
	int hour = tm.tm_hour;
	int minute = tm.tm_min;
	int second = tm.tm_sec;
	int weekday = 0;

	datefix(&year, &month, &day, &hour, &minute, &second, &weekday);

	snprintf(p, BUFFER_SIZE, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

close:
	array_free(&array, l);
	return error;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void functionDateAddInit(void) {
	event_function_register(&function_date_add, "DATE_ADD");

	function_date_add->run = &run;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "DATE_ADD";
	module->version = "2.0";
	module->reqversion = "7.0";
	module->reqcommit = "94";
}

void init(void) {
	functionDateAddInit();
}
#endif
