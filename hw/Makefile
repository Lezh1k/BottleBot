AVR_PREFIX = avr-

CC = $(AVR_PREFIX)gcc
OBJCOPY = $(AVR_PREFIX)objcopy
OBJDUMP = $(AVR_PREFIX)objdump
AR = $(AVR_PREFIX)ar
NM = $(AVR_PREFIX)nm
AS = $(AVR_PREFIX)as
LD = $(AVR_PREFIX)ld
SIZE = $(AVR_PREFIX)size

LIBS = 
DEFS = -DNDEBUG -D__AVR_ATtiny2313__ 

# Directories
INCLUDE_DIR = include
BUILD_DIR = build
SRC_DIR = src
BIN_DIR = bin

#device and program
PRG = BottleDriver
MCU=mcu=attiny2313
MMCU = -m$(MCU)

OPTIMIZE = -Os
INCLUDES = -Iinclude 

CFLAGS = $(INCLUDES) $(MMCU) $(OPTIMIZE) $(DEFS) -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -std=gnu99 \
  -Wl,--start-group  -Wl,--end-group -Wl,--gc-sections
ASFLAGS = -f coff
LDFLAGS = -Wl,-Map,$(BIN_DIR)/$(PRG).map $(MMCU) 

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS_C = $(patsubst %,$(BUILD_DIR)/%.o, $(subst src/,,$(subst .c,,$(SOURCES))))
OBJECTS_AS = $(patsubst %,$(BUILD_DIR)/%.o, $(subst src/,,$(subst .s,,$(SOURCES))))

all: directories $(PRG) 
$(PRG): $(BIN_DIR)/$(PRG).elf $(BIN_DIR)/lst $(BIN_DIR)/text print_size 

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -Wall $(CFLAGS) -c $< -o $@
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	$(AS) $(ASFLAGS) $< -o $@

$(BIN_DIR)/$(PRG).elf: $(OBJECTS_C)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BIN_DIR)/lst: $(BIN_DIR)/$(PRG).lst
$(BIN_DIR)/%.lst: $(BIN_DIR)/%.elf 
	$(OBJDUMP) -h -S $< > $@

$(BIN_DIR)/text: $(BIN_DIR)/hex $(BIN_DIR)/bin

$(BIN_DIR)/hex: $(BIN_DIR)/$(PRG).hex
$(BIN_DIR)/%.hex: $(BIN_DIR)/%.elf
	$(OBJCOPY) -O ihex $< $@

$(BIN_DIR)/bin: $(BIN_DIR)/$(PRG).bin
$(BIN_DIR)/%.bin: $(BIN_DIR)/%.elf
	$(OBJCOPY) -O binary $< $@

print_size:
	$(SIZE) -C --$(MCU) $(BIN_DIR)/$(PRG).elf

directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

clean:
	@rm -rf $(BUILD_DIR)/*
	@rm -rf $(BIN_DIR)/*

mrproper:
	@rm -rf $(BUILD_DIR) 
	@rm -rf $(BIN_DIR)

program:
	@avrdude -c usbasp -p t2313 -U flash:w:$(BIN_DIR)/$(PRG).hex

fuse_program:
	@avrdude -c usbasp -p t2313 -U lfuse:w:0xde:m -U hfuse:w:0xdf:m -U efuse:w:0xfe:m 
