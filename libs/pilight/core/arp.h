/*
	Copyright (C) 2015 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _ARP_H_
#define _ARP_H_

void arp_scan(void);
void arp_stop(void);
int arp_gc(void);

#endif
