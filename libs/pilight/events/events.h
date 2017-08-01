/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _EVENTS_H_
#define _EVENTS_H_

#include "../storage/storage.h"
#include "../core/common.h"

void event_cache_device(struct rules_t *obj, char *device);
int event_lookup_variable(char *var, struct rules_t *obj, struct varcont_t *varcont, unsigned short validate, enum origin_t origin);
int event_parse_rule(char *rule, struct rules_t *obj, int depth, unsigned short validate);
int events_gc(void);
void event_init(void);
void *events_loop(int, void *);

#endif
