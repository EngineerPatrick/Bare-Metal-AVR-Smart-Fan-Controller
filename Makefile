MCU		:= atmega328p
PORT		:= /dev/ttyUSB0
PROGRAMMER	:= arduino
BAUD		:= 115200

CC   		:= avr-gcc
OBJCOPY		:= avr-objcopy
SIZE		:= avr-size
CSTD 		:= -std=c11

BLD_DIR		:= build
OBJ_DIR		:= $(BLD_DIR)/obj
BIN_DIR		:= $(BLD_DIR)/bin
APP_DIR		:= app
DRIV_DIR 	:= drivers
SERV_DIR 	:= services
BSP_DIR		:= bsp
CONF_DIR 	:= config
DOCS_DIR 	:= docs

INC_DIRS 	:= $(wildcard $(APP_DIR)/*/) $(wildcard $(DRIV_DIR)/*/) $(wildcard $(SERV_DIR)/*/) $(BSP_DIR) $(CONF_DIR)
WARN 		:= -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Woverflow
CPPFLAGS 	:= $(addprefix -I,$(INC_DIRS))
CFLAGS		:= -mmcu=$(MCU) -Os -MMD -MP
CFLAGS		+= $(CSTD) $(WARN)
LDFLAGS		:= -mmcu=$(MCU)

SRCS 		:= $(APP_DIR)/main.c $(wildcard $(APP_DIR)/*/*.c) $(wildcard $(DRIV_DIR)/*/*.c) $(wildcard $(SERV_DIR)/*/*.c)
OBJS 		:= $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

DEPS 		:= $(patsubst $(OBJ_DIR)/%.o, $(OBJ_DIR)/%.d, $(OBJS))

ELF		:= $(BIN_DIR)/launcher.elf
HEX		:= $(BIN_DIR)/launcher.hex

.PHONY: all flash clean docs

all: $(HEX)

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@
	
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(ELF): $(OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ -o $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	$(SIZE) --mcu=$(MCU) --format=avr $<

flash: $(HEX)
	avrdude -p m328p -c $(PROGRAMMER) -P $(PORT) -b $(BAUD) -U flash:w:$(HEX):i

clean:
	rm -rf $(BLD_DIR) $(DOCS_DIR)/html $(DOCS_DIR)/latex
	
docs:
	doxygen docs/Doxyfile
	
-include $(DEPS)
