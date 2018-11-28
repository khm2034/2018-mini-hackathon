#ifndef __1602A_LCD_UTIL_H__
#define __1602A_LCD_UTIL_H__

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "resource/resource_1602A_LCD.h"
#include "resource/resource_1602A_LCD_internal.h"
#include "resource/resource_util.h"
#include "log.h"

void print_lcd(char* s, lcd_id_e id);
void print_info_lcd(char* s, lcd_id_e id);

#endif
