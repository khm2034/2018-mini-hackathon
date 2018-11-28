#ifndef __RESOURCE_UTIL_H__
#define __RESOURCE_UTIL_H__
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void resource_util_delay_microseconds_hard (unsigned int how_long);
void resource_util_delay_microseconds (unsigned int how_long);
void resource_util_delay (unsigned int how_long);

#endif
