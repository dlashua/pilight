/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mbedtls/sha256.h>

#include "../libs/pilight/core/log.h"
#include "../libs/pilight/core/CuTest.h"
#include "../libs/pilight/core/pilight.h"
#include "../libs/pilight/core/network.h"
#include "../libs/libuv/uv.h"

#include "alltests.h"
#include "gplv3.h"

#define NRSUITS 128

CuSuite *suite_common(void);
CuSuite *suite_network(void);
CuSuite *suite_binary(void);
CuSuite *suite_proc(void);
CuSuite *suite_datetime(void);
CuSuite *suite_json(void);
CuSuite *suite_cast(void);
CuSuite *suite_sha256cache(void);
CuSuite *suite_strptime(void);
CuSuite *suite_dso(void);
CuSuite *suite_options(void);
CuSuite *suite_eventpool(void);
CuSuite *suite_ssdp(void);
CuSuite *suite_ntp(void);
CuSuite *suite_arp(void);
CuSuite *suite_http(void);
CuSuite *suite_ping(void);
CuSuite *suite_mail(void);
CuSuite *suite_webserver(void);
CuSuite *suite_socket(void);
CuSuite *suite_log(void);
CuSuite *suite_protocols_433(void);
CuSuite *suite_protocols_api(void);
CuSuite *suite_protocols_api_openweathermap(void);
CuSuite *suite_protocols_api_wunderground(void);
CuSuite *suite_protocols_api_lirc(void);
CuSuite *suite_protocols_api_xbmc(void);
CuSuite *suite_protocols_network_ping(void);
CuSuite *suite_protocols_network_arping(void);
CuSuite *suite_protocols_core(void);
CuSuite *suite_protocols_generic(void);
CuSuite *suite_protocols_i2c(void);
CuSuite *suite_protocols_gpio_ds18x20(void);
CuSuite *suite_event_operators(void);
CuSuite *suite_event_functions(void);
CuSuite *suite_event_actions_switch(void);
CuSuite *suite_event_actions_toggle(void);
CuSuite *suite_event_actions_label(void);
CuSuite *suite_event_actions_dim(void);
CuSuite *suite_event_actions_mail(void);
CuSuite *suite_events(void);

CuString *output = NULL;
CuSuite *suite = NULL;
CuSuite *suites[NRSUITS];

const uv_thread_t pth_main_id;
int nr = 0;

// void _logprintf(int prio, char *file, int line, const char *str, ...) {
	// va_list ap;
	// char buffer[1024];

	// memset(buffer, 0, 1024);

	// va_start(ap, str);
	// vsnprintf(buffer, 1024, str, ap);
	// va_end(ap);

	// printf("(%s #%d) %s\n", file, line, buffer);
// }

int suiteFailed(void) {
	int i = 0;
	for(i=0;i<suite->count;i++) {
		if(suite->list[i]->failed == 1) {
			return 1;
		}
	}
	return 0;
}

int RunAllTests(void) {
	int i = 0;
	output = CuStringNew();
	suite = CuSuiteNew();

	const uv_thread_t pth_cur_id = uv_thread_self();
	memcpy((void *)&pth_main_id, &pth_cur_id, sizeof(uv_thread_t));

	memtrack();

	/*
	 * Logging is asynchronous. If the log is being filled
	 * when a test is already finished, xfree() is called
	 * which also garbage collects all open pointers.
	 *
	 * If the log is still trying to process everything
	 * you get free calls on already freed pointers.
	 * Logging should therefor be disabled for all tests
	 * except the log test itself. Which will enable and
	 * disable logging for proper testing.
	 */
	log_file_disable();
	log_shell_disable();

	/*
	 * Storage should be tested first
	 */
	suites[nr++] = suite_common();
	suites[nr++] = suite_network();
	suites[nr++] = suite_binary();
	suites[nr++] = suite_proc();
	suites[nr++] = suite_datetime();
	suites[nr++] = suite_json();
	suites[nr++] = suite_cast();
	suites[nr++] = suite_sha256cache();
	suites[nr++] = suite_strptime();
	suites[nr++] = suite_options();
	suites[nr++] = suite_dso(); // Fix the dll creation
	suites[nr++] = suite_eventpool();
	suites[nr++] = suite_log();
	suites[nr++] = suite_ssdp();
	suites[nr++] = suite_ping();
	suites[nr++] = suite_ntp();
	suites[nr++] = suite_arp();
	suites[nr++] = suite_http();
	suites[nr++] = suite_mail();
	suites[nr++] = suite_webserver();
	suites[nr++] = suite_socket();
	suites[nr++] = suite_protocols_433();
	suites[nr++] = suite_protocols_api();
#ifndef _WIN32
	suites[nr++] = suite_protocols_api_lirc();
#endif
	suites[nr++] = suite_protocols_api_xbmc();
	suites[nr++] = suite_protocols_api_openweathermap();
	suites[nr++] = suite_protocols_api_wunderground();
	suites[nr++] = suite_protocols_network_ping();
	suites[nr++] = suite_protocols_network_arping();
	suites[nr++] = suite_protocols_core();
	suites[nr++] = suite_protocols_generic();
#ifdef __linux__
	suites[nr++] = suite_protocols_i2c();
	suites[nr++] = suite_protocols_gpio_ds18x20();
#endif
	suites[nr++] = suite_event_operators();
	suites[nr++] = suite_event_functions();
	suites[nr++] = suite_event_actions_switch();
	suites[nr++] = suite_event_actions_toggle();
	suites[nr++] = suite_event_actions_label();
	suites[nr++] = suite_event_actions_dim();
	suites[nr++] = suite_event_actions_mail();
	suites[nr++] = suite_events();

	for(i=0;i<nr;i++) {
		CuSuiteAddSuite(suite, suites[i]);
	}

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);

	int r = (suite->failCount == 0) ? 0 : -1;

	for(i=0;i<nr;i++) {
		free(suites[i]);
	}
	CuSuiteDelete(suite);
	CuStringDelete(output);
	return r;
}


int main(int argc, char **argv) {
	remove("rapport.txt");
	remove("test.log");

	FILE *f = fopen("gplv3.txt", "w");
	fprintf(f, "%s", gplv3);
	fclose(f);

	printf("[ %-48s ]\n", "test_unittest");
	fflush(stdout);
	assert(17 == test_unittest());

	printf("[ %-48s ]\n", "test_memory");
	fflush(stdout);
	assert(0 == test_memory());

	return RunAllTests();
}
