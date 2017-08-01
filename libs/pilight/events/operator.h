/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _EVENT_OPERATOR_H_
#define _EVENT_OPERATOR_H_

#include "../core/common.h"

typedef struct event_operators_t {
	char *name;
	void (*callback)(struct varcont_t *a, struct varcont_t *b, char **ret);
	// void (*callback_number)(double a, double b, char **ret);
	unsigned short type;
	struct event_operators_t *next;
} event_operators_t;

struct event_operators_t *event_operators;

void event_operator_init(void);
void event_operator_register(struct event_operators_t **op, const char *name);
int event_operator_gc(void);

#endif
