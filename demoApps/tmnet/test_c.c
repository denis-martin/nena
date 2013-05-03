/*
 * test_c.c
 *
 *  Created on: 26 Jan 2012
 *      Author: denis
 */

#include "net_c.h"

#include <stdio.h>

int main(int argc, char** argv) {

	if (tmnet_init() != TMNET_OK) {
		printf("tmnet_init() failed.\n");
		return 1;
	}

	tmnet_pnhandle h;
	int e = tmnet_bind(&h, "udp://0.0.0.0:1025", "");
	if (e != TMNET_OK) {
		printf("test_c: bind failed: 0x%x\n", e);

	} else {
		tmnet_close(h);

	}

	printf("test_c: finished.\n");

	return 0;
}
