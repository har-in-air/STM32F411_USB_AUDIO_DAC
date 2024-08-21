TARGET = usb_audio_i2s

# To build for STM32F411CEU6 versus STM32F401CCU6,
# set CPU_TARGET, ASM_SOURCES and LDSCRIPT correctly

# STM32F411CEU6 Black Pill (512kB flash, 256kB RAM, 100MHz)
CPU_TARGET = STM32F411xE
ASM_SOURCES =  startup_stm32f411ceux.s
LDSCRIPT = STM32F411CEUX_FLASH.ld

# STM32F401CCU6 Black Pill (256kB flash, 64kB RAM, 84MHz)
#CPU_TARGET = STM32F401xC 
#ASM_SOURCES = startup_stm32f401ccux.s
#LDSCRIPT = STM32F401CCUX_FLASH.ld

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DUSE_FULL_ASSERT \
-D$(CPU_TARGET)
#-DDEBUG_FEEDBACK_ENDPOINT 
#-DUSE_MCLK_OUT 
# Note : MCLK output is only possible on F411 mcu

# This is a pure Makefile project. It does not require the IDE to set up the environment, so
# ensure the paths to the toolchain binaries are added to your environment PATH variable. 
# E.g. for my specific installation with STM32CubeIDE 1.16.0 on Ubuntu 22.04 LTS, the compiler and tools are at 
# /opt/st/stm32cubeide_1.16.0/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.12.3.rel1.linux64_1.0.200.202406132123/tools/bin
# /opt/st/stm32cubeide_1.16.0/plugins/com.st.stm32cube.ide.mcu.externaltools.make.linux64_2.1.100.202310302056/tools/bin
# I installed st-flash by downloading the .deb package from https://github.com/stlink-org/stlink/releases 
# sudo apt install ./stlink_1.8.0-1_amd64.deb

CC = arm-none-eabi-gcc
AS = arm-none-eabi-gcc -x assembler-with-cpp
CP = arm-none-eabi-objcopy
SZ = arm-none-eabi-size

HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################
# CFLAGS
#######################################
CPU = -mcpu=cortex-m4

FPU = -mfpu=fpv4-sp-d16

FLOAT-ABI = -mfloat-abi=hard

MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

AS_DEFS = 
AS_INCLUDES = 

DEBUG = 0
OPT = -O2

BUILD_DIR = build

C_SOURCES =  \
src/main.c \
src/usart.c \
src/usbd_conf.c \
src/usbd_desc.c \
src/usbd_audio_if.c \
src/stm32f4xx_it.c \
src/system_stm32f4xx.c \
drivers/usb/Core/Src/usbd_core.c \
drivers/usb/Core/Src/usbd_ctlreq.c \
drivers/usb/Core/Src/usbd_ioreq.c \
drivers/usb/Class/AUDIO/Src/usbd_audio.c \
drivers/BSP/bsp_misc.c \
drivers/BSP/bsp_audio.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.c \
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.c \
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
drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c

C_INCLUDES =  \
-Isrc \
-Idrivers/BSP \
-Idrivers/usb/Core/Inc \
-Idrivers/usb/Class/AUDIO/Inc \
-Idrivers/CMSIS/Device/ST/STM32F4xx/Include \
-Idrivers/CMSIS/Include \
-Idrivers/STM32F4xx_HAL_Driver/Inc \
-Idrivers/STM32F4xx_HAL_Driver/Inc/Legacy


ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif


# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"


# libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs  -u _printf_float -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin


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

clean:
	-rm -fR $(BUILD_DIR)
  
flash:
	-st-flash --reset write $(BUILD_DIR)/$(TARGET).bin 0x08000000

# dependencies
-include $(wildcard $(BUILD_DIR)/*.d)

