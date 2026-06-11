#include "ui.h"
#include "fault_manager.h"
#include "scheduler.h"

int main(void) {
	ui_errors error_code = 0;
	
	scheduler_timer_start();
	
	while(1) {
		error_code = ui_system_configure();
		
		if (error_code != UI_ERR_OK) {
			fault_manager_ui(error_code);
			break;
		}
		
		error_code = ui_system_update();
		
		if (error_code != UI_ERR_OK) {
			fault_manager_ui(error_code);
			break;
		}
	}
	
	scheduler_timer_stop();
	
	while (1) {}
	
	return 0;
}
