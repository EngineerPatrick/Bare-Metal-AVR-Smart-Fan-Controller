#include <avr/interrupt.h>
#include "ui.h"
#include "fault_manager.h"
#include "scheduler.h"

int main(void) {
	ui_errors error_code = 0;
	
	sei();
	scheduler_timer_boot();
	
	while(1) {
		error_code = ui_system_config();
		
		if (error_code != UI_ERR_OK) {
			fault_manager_ui_report(error_code);
			break;
		}
		
		error_code = ui_system_runtime_loop();
		
		if (error_code != UI_ERR_OK) {
			fault_manager_ui_report(error_code);
			break;
		}
	}
	
	scheduler_timer_stop();
	
	while (1) {}
	
	return 0;
}
