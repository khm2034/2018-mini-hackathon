#include "1602A_lcd_util.h"

void print_lcd(char* s, lcd_id_e id){
	//resource_set_1602A_LCD_configuration(0, 2, 16, 4, 46, 130, 26, 25, 27, 0, 0, 0, 0, 0);
	resource_1602A_LCD_position(id, 0, 0);
	resource_1602A_LCD_puts(id, s);
	//resource_1602A_LCD_close(id);
}

void print_info_lcd(char* s, lcd_id_e id){
	//char bell_id[20] = "B_ID : ";
	char order_num[20] = "O_N : ";
	int bell_l = 7;
	int order_l = 6;
	int i;
	//for(i=0; s[i] != ','; i++) bell_id[bell_l++] = s[i];
	//bell_id[bell_l] = '\0';

	_D("O_N : %s",  order_num);
	for(i = 0; s[i]; i++) order_num[order_l++] = s[i];
	order_num[order_l] = '\0';
	_D("O_N : %s",  order_num);
	print_lcd(order_num, id);
//	resource_set_1602A_LCD_configuration(0, 2, 16, 4, 27, 22, 6, 13, 19, 26, 0, 0, 0, 0);
//	resource_1602A_LCD_position(id, 0, 0);
//	resource_1602A_LCD_puts(id, bell_id);
//	resource_1602A_LCD_position(id, 0, 1);
//	resource_1602A_LCD_puts(id, order_num);
//	resource_1602A_LCD_close(id);
}


