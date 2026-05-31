include Makefile.variable

#wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
# Add your source files here
#wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
# firmware output file name
PROJ_NAME=main

# linker scipt
LINKER_SCRIPT=./00_sdk/STM32F407VGTX_FLASH.ld


# startup file
STARTUP_FILE=./00_sdk/startup_stm32f407vgtx.s

BUILD_DIR = ./build

C_SRCS = main.c 
C_SRCS += ./00_sdk/system_stm32f4xx.c


INC_PATHS =  -I./
INC_PATHS += -I./00_sdk/

include ./01_common/Makefile.common
include ./02_drivers/Makefile.drivers
include ./02_drivers/STM32F4xx_HAL_Driver/Makefile.stm32_hal
# include ./02_drivers/usb/Makefile.usb
include ./03_hal/Makefile.hal
include ./04_middleware/Makefile.middleware
include ./05_application/Makefile.application


CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)
C_DEFS =  \
-DSTM32F407xx \
-DUSE_HAL_DRIVER \
-DUSE_FULL_LL_DRIVER

# -DSTM32_THREAD_SAFE_STRATEGY=2

# -DSTM32U585xx \
#wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
# Normally you shouldn't need to change anything below this line!
#wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
CURRENT_DIR = $(shell pwd)
BUILD_DIR = $(CURRENT_DIR)/build

CFLAGS  = -g -O2 -Wall  -T$(LINKER_SCRIPT)  $(MCU)
CFLAGS += -mabi=aapcs --std=gnu99
CFLAGS += -ffunction-sections -fdata-sections  
CFLAGS += -Wl,--gc-sections 
CFLAGS +=  $(C_DEFS)

ASMFLAGS = -x assembler-with-cpp

# keep every function in separate section. This will allow linker to dump unused functions
#LDFLAGS = -Xlinker -Map=$(BUILD_DIR)/$(PROJ_NAME).map
LDFLAGS += $(MCU) -mabi=aapcs -L./ -T$(LINKER_SCRIPT) 
LDFLAGS += -Wl,--gc-sections 


# add startup file to build
A_SRCS = $(STARTUP_FILE) #wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
# Makefile functions and constants
#wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
#function for removing duplicates in a list
remduplicates = $(strip $(if $1,$(firstword $1) $(call remduplicates,$(filter-out $(firstword $1),$1))))

# coloring constants
ccblack=@echo "\033[0;30m"
ccred=@echo "\033[0;31m"
ccgreen=@echo "\033[0;32m"
ccyellow=@echo "\033[0;33m"
ccblue=@echo "\033[0;34m"
ccmagenta=@echo "\033[0;35m"
cccyan=@echo "\033[0;36m"
ccgray=@echo "\033[0;37m"
ccend="\033[0m"

#wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
# C source files processing
#wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
# extract all file names from the C sources
C_SOURCE_FILE_NAMES = $(notdir $(C_SRCS))
# get all sub paths from the C sources
C_PATHS = $(call remduplicates, $(dir $(C_SRCS)))
# create object file file names for all C source files
C_OBJECTS = $(addprefix $(BUILD_DIR)/, $(C_SOURCE_FILE_NAMES:.c=.o) )

#wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
# ASM source files processing
#wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
# extract all file names from the ASM sources
ASM_SOURCE_FILE_NAMES = $(notdir $(A_SRCS))
# get all sub paths from the ASM sources
ASM_PATHS = $(call remduplicates, $(dir $(A_SRCS)))
# create object file file names for all ASM source files
ASM_OBJECTS = $(addprefix $(BUILD_DIR)/, $(ASM_SOURCE_FILE_NAMES:.s=.o) )

# get all C and ASM source full paths
vpath %.c $(C_PATHS)
vpath %.s $(ASM_PATHS)

OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

#wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
# compilation recipes
#wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
.PHONY: proj

all: 
	@$(MAKE) -s --no-print-directory preaction 
	@$(MAKE) -s --no-print-directory $(OBJECTS) 
	@$(MAKE) -s --no-print-directory compile 
	@$(MAKE) -s --no-print-directory postaction

compile:	
	$(ccyellow)linking target...$(ccend)
	@$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -lm -lc -lnosys -o  $(BUILD_DIR)/$(PROJ_NAME).elf
	$(ccyellow)creating executables...$(ccend)
#	@$(OBJCOPY) -O ihex $(BUILD_DIR)/$(PROJ_NAME).elf $(BUILD_DIR)/$(PROJ_NAME).hex
	@$(OBJCOPY) -O binary $(BUILD_DIR)/$(PROJ_NAME).elf $(BUILD_DIR)/$(PROJ_NAME).bin 
	
preaction:
	@python3 ./06_scripts/fwinfo.py
	
# override default make recipe for C files
$(BUILD_DIR)/%.o: %.c
	$(ccgray)compiling c file:$(ccend)   $(notdir $@)
	@$(CC) $(CFLAGS) $(INC_PATHS) -c -lm -lc -lnosys -o $@ $<

# override default make recipe for ASM files
$(BUILD_DIR)/%.o: %.s
	@echo compiling asm file: $(notdir $@)
	@$(CC) $(ASMFLAGS) $(INC_PATHS) -c -o $@ $<

postaction: 
	@$(SIZE) -B  $(BUILD_DIR)/$(PROJ_NAME).elf

dump:
	@$(OBJDUMP) -D $(BUILD_DIR)/$(PROJ_NAME).elf > $(BUILD_DIR)/$(PROJ_NAME).lst
	  
clean:
	$(ccyellow)cleaning files...$(ccend)
	@rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.elf $(BUILD_DIR)/*.hex $(BUILD_DIR)/*.map $(BUILD_DIR)/*.lst
	@echo done

upload: $(BUILD_DIR)/$(PROJ_NAME).bin
	$(STM32CUBEPROG) -d $(BUILD_DIR)/$(PROJ_NAME).bin  0x08000000 
	@sleep 1
	@$(STM32CUBEPROG) -hardRst 
		
upload-release: $(BUILD_DIR)/$(PROJ_NAME).bin
	$(STM32CUBEPROG) -d ./release/intraDrive.bin  0x08000000 
	@sleep 1
	@$(STM32CUBEPROG) -hardRst 

reset:
	@$(STM32CUBEPROG) -hardRst 
	
erase:
	@$(STM32CUBEPROG) -e all
	@sleep 1
	@$(STM32CUBEPROG) -hardRst

help:
	$(STM32CUBEPROG) --help	


project_update:
	./06_scripts/makefile_update.sh  01_common ./01_common/Makefile.common
	./06_scripts/makefile_update.sh  02_drivers ./02_drivers/Makefile.drivers
	./06_scripts/makefile_update.sh  02_drivers/STM32F4xx_HAL_Driver ./02_drivers/STM32F4xx_HAL_Driver/Makefile.stm32_hal
	
	./06_scripts/makefile_update.sh  03_hal ./03_hal/Makefile.hal
	./06_scripts/makefile_update.sh  04_middleware ./04_middleware/Makefile.middleware
	./06_scripts/makefile_update.sh  05_application ./05_application/Makefile.application
	./06_scripts/update_layer_header.sh  02_drivers ./02_drivers/drivers_inc.h "drivers.h"
	./06_scripts/update_layer_header.sh  03_hal ./03_hal/hal.h

