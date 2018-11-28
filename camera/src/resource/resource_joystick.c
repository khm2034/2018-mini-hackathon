#include <peripheral_io.h>
#include <unistd.h>
#include "log.h"
#include "resource/resource_joystick.h"
#include "resource/resource_joystick_internal.h"

typedef enum{
	JOYSTICK_CLOSE,
	JOYSTICK_READY
} joystick_state_e;

joystick_state_e joystick_state = JOYSTICK_CLOSE;
peripheral_spi_h spi_h;
uint8_t tx_data[3];
uint8_t rx_data[3];

int resource_joystick_close(){
	int ret = 0;
	ret = peripheral_spi_close(spi_h);
	if(ret != PERIPHERAL_ERROR_NONE){
		_E("failed to close spi");
		return -1;
	}
	joystick_state = JOYSTICK_CLOSE;
	return 0;
}

int _resource_joystick_init(){
	int ret = 0;
	ret = peripheral_spi_open(2, 0, &spi_h);
	if(ret != PERIPHERAL_ERROR_NONE){
		_E("failed to open spi with bus[0] and chip[0]");
		return -1;
	}
	joystick_state = JOYSTICK_READY;
	return 0;
}

int _resource_joystick_read_channel(uint8_t ch){
	int ret = 0;
	tx_data[0] = 1;
	tx_data[1] = (8+ch)<<4;
	tx_data[2] = 0;
	ret = peripheral_spi_transfer(spi_h, tx_data, rx_data, 3);
	if(ret != PERIPHERAL_ERROR_NONE){
		_E("failed to transfer spi with channel[%d]", ch);
		return -1;
	}
	return ((rx_data[1]&3) << 8) + rx_data[2];
}

int resource_joystick_read(int* x_pos, int* y_pos, int* sw_pos){
	int ret = 0;
	int vrx_pos, vry_pos, swt_val;
	if(joystick_state == JOYSTICK_CLOSE){
		ret = _resource_joystick_init();
		if(ret != PERIPHERAL_ERROR_NONE)
			return -1;
	}
	swt_val = _resource_joystick_read_channel(0);
	if(swt_val == -1){
		_E("failed to read channel[0]");
		return -1;
	}
	vrx_pos = _resource_joystick_read_channel(1);
	if(vrx_pos == -1){
		_E("failed to read channel[1]");
		return -1;
	}
	vry_pos = _resource_joystick_read_channel(2);
	if(vry_pos == -1){
		_E("failed to read channel[2]");
		return -1;
	}
	*x_pos = vrx_pos;
	*y_pos = vry_pos;
	*sw_pos = swt_val;
	return 0;
}
