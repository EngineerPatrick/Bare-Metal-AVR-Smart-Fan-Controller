#include <avr/interrupt.h>
#include "fan_driver.h"
#include "display.h"
#include "scheduler.h"
#include "ui.h"

int main(void) {
	sei();
	scheduler_timer_boot();
	
	ui_system_config();
	
	display_units_write();
	
	uint16_t reading = /*fan_driver_update_delay_test()*/fan_driver_speed_test();
	
	display_speed_write(reading/*/ 10*/);
	
	scheduler_timer_stop();
	
	while (1) {}
	
	return 0;
}
