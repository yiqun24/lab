#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;
        char* exp;
        uint32_t value;
	/* TODO: Add more members if necessary */


} WP;
WP *new_wp();
void free_wp(WP* );
int check_wp();
void delete_wp(int );
void info_wp();
#endif
