
######################################
# target
######################################
TARGET = f411_usb_audio_i2s


######################################
# building variables
######################################
# debug build?
DEBUG = 0
# optimization
OPT = -O2

#######################################
# paths
#######################################
# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources
C_SOURCES =  \
src/main.c \
src/usart.c \
src/usbd_conf.c \
src/usbd_desc.c \
src/usbd_audio_if.c \
src/stm32f4xx_it.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c_ex.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c \
drivers/BSP/bsp_misc.c \
drivers/BSP/bsp_audio.c \
src/system_stm32f4xx.c \
stm32_usb/Core/Src/usbd_core.c \
stm32_usb/Core/Src/usbd_ctlreq.c \
stm32_usb/Core/Src/usbd_ioreq.c \
stm32_usb/Class/AUDIO/Src/usbd_audio.c


# ASM sources
ASM_SOURCES =  startup_stm32f411ceux.s


#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m4

# fpu
FPU = -mfpu=fpv4-sp-d16

# float-abi
FLOAT-ABI = -mfloat-abi=hard

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS = 

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32F411xE \
-DUSE_FULL_ASSERT \
-DDEBUG_FEEDBACK_ENDPOINT\
#-DUSE_MCLK_OUT


# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES =  \
-Iinc \
-Idrivers/STM32F4xx_HAL_Driver/Inc \
-Idrivers/STM32F4xx_HAL_Driver/Inc/Legacy \
-Istm32_usb/Core/Inc \
-Istm32_usb/Class/AUDIO/Inc \
-Idrivers/CMSIS/Device/ST/STM32F4xx/Include \
-Idrivers/CMSIS/Include \
-Idrivers/BSP



# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif


# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32F411CEUX_FLASH.ld

# libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs  -u _printf_float -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@		

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)
  
#######################################
# Flash binary
#######################################
flash:
	-st-flash --reset write $(BUILD_DIR)/$(TARGET).bin 0x08000000

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***
