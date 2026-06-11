MCU         := atmega328p
PORT        := /dev/ttyUSB0
PROGRAMMER  := arduino
BAUD        := 115200

CC   		:= avr-gcc
OBJCOPY     := avr-objcopy
SIZE        := avr-size

BLD_DIR		:= build
OBJ_DIR		:= $(BLD_DIR)/obj
APP_DIR		:= app
DRIV_DIR 	:= drivers
SERV_DIR 	:= services
BSP_DIR		:= bsp
CONF_DIR 	:= config
DOCS_DIR 	:= docs

INC_DIRS 	:= $(wildcard $(APP_DIR)/*/) $(wildcard $(DRIV_DIR)/*/) $(wildcard $(SERV_DIR)/*/) $(CONF_DIR) $(BSP_DIR)
CPPFLAGS 	+= $(addprefix -I,$(INC_DIRS))
CFLAGS      := -mmcu=$(MCU) -Os -std=c11 -MMD -MP
LDFLAGS     := -mmcu=$(MCU)
WARN 		:= -Wall -Wextra -Wpedantic -Wshadow

SRCS 		:= $(APP_DIR)/main.c $(wildcard $(APP_DIR)/*/*.c) $(wildcard $(DRIV_DIR)/*/*.c) $(wildcard $(SERV_DIR)/*/*.c)
OBJS 		:= $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

DEPS 		:= $(patsubst $(OBJ_DIR)/%.o, $(OBJ_DIR)/%.d, $(OBJS))

ELF         := $(BLD_DIR)/launcher.elf
HEX         := $(BLD_DIR)/launcher.hex

.PHONY: all flash clean docs

all: $(HEX)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
	
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CC) $(WARN) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(ELF): $(OBJS)
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
