/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <limits.h>

#ifndef _WIN32
	#include <libgen.h>
	#include <dirent.h>
	#include <unistd.h>
#endif

#include "../core/pilight.h"
#include "../core/common.h"
#include "../core/dso.h"
#include "../core/log.h"

#include "operator.h"
#include "operators/operator_header.h"

#ifndef _WIN32
void event_operator_remove(char *name) {
	struct event_operators_t *currP, *prevP;

	prevP = NULL;

	for(currP = event_operators; currP != NULL; prevP = currP, currP = currP->next) {

		if(strcmp(currP->name, name) == 0) {
			if(prevP == NULL) {
				event_operators = currP->next;
			} else {
				prevP->next = currP->next;
			}

			logprintf(LOG_DEBUG, "removed operator %s", currP->name);
			FREE(currP->name);
			FREE(currP);

			break;
		}
	}
}
#endif

void event_operator_init(void) {
	#include "operators/operator_init.h"

#ifndef _WIN32
	void *handle = NULL;
	void (*init)(void);
	void (*compatibility)(struct module_t *module);
	char path[PATH_MAX];
	struct module_t module;
	char pilight_version[strlen(PILIGHT_VERSION)+1];
	char pilight_commit[3];
	char *operator_root = NULL;
	int check1 = 0, check2 = 0, valid = 1, operator_root_free = 0;
	strcpy(pilight_version, PILIGHT_VERSION);

	struct dirent *file = NULL;
	DIR *d = NULL;
	struct stat s;

	memset(pilight_commit, '\0', 3);

	if(settings_select_string(ORIGIN_MASTER, "operators-root", &operator_root) != 0) {
		/* If no operator root was set, use the default operator root */
		if((operator_root = MALLOC(strlen(OPERATOR_ROOT)+1)) == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		strcpy(operator_root, OPERATOR_ROOT);
		operator_root_free = 1;
	}
	size_t len = strlen(operator_root);
	if(operator_root[len-1] != '/') {
		strcat(operator_root, "/");
	}

	if((d = opendir(operator_root))) {
		while((file = readdir(d)) != NULL) {
			memset(path, '\0', PATH_MAX);
			sprintf(path, "%s%s", operator_root, file->d_name);
			if(stat(path, &s) == 0) {
				/* Check if file */
				if(S_ISREG(s.st_mode)) {
					if(strstr(file->d_name, ".so") != NULL) {
						valid = 1;

						if((handle = dso_load(path)) != NULL) {
							init = dso_function(handle, "init");
							compatibility = dso_function(handle, "compatibility");
							if(init != NULL && compatibility != NULL) {
								compatibility(&module);
								if(module.name != NULL && module.version != NULL && module.reqversion != NULL) {
									char ver[strlen(module.reqversion)+1];
									strcpy(ver, module.reqversion);

									if((check1 = vercmp(ver, pilight_version)) > 0) {
										valid = 0;
									}

									if(check1 == 0 && module.reqcommit != NULL) {
										char com[strlen(module.reqcommit)+1];
										strcpy(com, module.reqcommit);
										sscanf(HASH, "v%*[0-9].%*[0-9]-%[0-9]-%*[0-9a-zA-Z\n\r]", pilight_commit);

										if(strlen(pilight_commit) > 0 && (check2 = vercmp(com, pilight_commit)) > 0) {
											valid = 0;
										}
									}
									if(valid == 1) {
										char tmp[strlen(module.name)+1];
										strcpy(tmp, module.name);
										event_operator_remove(tmp);
										init();
										logprintf(LOG_DEBUG, "loaded operator %s v%s", file->d_name, module.version);
									} else {
										if(module.reqcommit != NULL) {
											logprintf(LOG_ERR, "event operator %s requires at least pilight v%s (commit %s)", file->d_name, module.reqversion, module.reqcommit);
										} else {
											logprintf(LOG_ERR, "event operator %s requires at least pilight v%s", file->d_name, module.reqversion);
										}
									}
								} else {
									logprintf(LOG_ERR, "invalid module %s", file->d_name);
								}
							}
						}
					}
				}
			}
		}
		closedir(d);
	}
	if(operator_root_free) {
		FREE(operator_root);
	}
#endif
}

void event_operator_register(struct event_operators_t **op, const char *name) {
	if((*op = MALLOC(sizeof(struct event_operators_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	if(((*op)->name = MALLOC(strlen(name)+1)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	strcpy((*op)->name, name);

	(*op)->callback = NULL;
	// (*op)->callback_number = NULL;

	(*op)->next = event_operators;
	event_operators = (*op);
}

int event_operator_gc(void) {
	struct event_operators_t *tmp_operator = NULL;
	while(event_operators) {
		tmp_operator = event_operators;
		FREE(tmp_operator->name);
		event_operators = event_operators->next;
		FREE(tmp_operator);
	}

	logprintf(LOG_DEBUG, "garbage collected event operator library");
	return 0;
}
