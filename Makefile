#################################################
#	file:		Makefile
#	purpose:	makefile
#	remarks:	thanks to mamalala, updated by izhak2 
#				change cross-tool and sdk paths according to your system setup
#################################################

APP_NAME	= esphttpd
APP_VERSION	= OTA_1.0
CURRVER 	= 0
SHELL 		:= /bin/bash

BUILD_BASE	= build
FW_BASE		= firmware

# Base directory for the compiler
XTENSA_TOOLS_ROOT	?= /opt/Espressif/esp-open-sdk/xtensa-lx106-elf/bin

#Extra Tensilica includes from the ESS VM
SDK_EXTRA_INCLUDES	?= /opt/Espressif/esp-open-sdk/sdk/include
SDK_EXTRA_LIBS		?= /opt/Espressif/esp-open-sdk/sdk/lib

# base directory of the ESP8266 SDK package, absolute
SDK_ROOT			?= /opt/Espressif/esp-open-sdk/
SDK_VERSION			?= 1.3.0
SDK_BASE			?= $(SDK_ROOT)esp_iot_sdk_v$(SDK_VERSION)/

# Hardware info
ESP_FLASH_SIZE		?= 512
ESP_SPI_SIZE		?= 0
ESP_FLASH_MODE 		?= 0 
ESP_FLASH_FREQ_DIV 	?= 0

ESP_IPADDRESS		?= 192.168.4.1

#Esptool.py path and port
ESPTOOL				?= esptool
FW_TOOL				?= esptool
# Needed for old bootloader/SDK versions
JOIN_TOOL			?= gen_flashbin.py
# Needed for new bootloader/SDK versions
APPGEN_TOOL			?= gen_appbin.py
ESPTOOLPY			?= esptool/esptool.py
ESPPORT				?= /dev/ttyS1

#ESPDELAY indicates seconds to wait between flashing the two binary images
ESPDELAY			?= 3
ESPBAUD				?= 115200

# name for the target project
TARGET				?= httpd
OTA					?= true
# Bootloader and defaults
BLANK				= blank.bin
ESP_BOOT_VER		?= new

ifeq ("$(SDK_VERSION)","0.9.3")
ESP_BOOT_VER		= old
BOOTLOADER			?= boot_v1.1.bin
endif
ifeq ("$(ESP_BOOT_VER)","new")
ifeq ("$(SDK_VERSION)","1.0.0")
BOOTLOADER      	?= boot_v1.3\(b3\).bin
else
BOOTLOADER      	?= boot_v1.2.bin
endif
else
BOOTLOADER			?= boot_v1.1.bin
endif

ifeq ("$(ESP_BOOT_VER)","new")
BOOT_MODE 			= 2
else
BOOT_MODE 			= 1
endif

# which modules (subdirectories) of the project to include in compiling
MODULES			= user
EXTRA_INCDIR	= include 				\
				  ext_include 			\
				  . 					\
				  lib/heatshrink/ 		\
				  $(SDK_EXTRA_INCLUDES)

# libraries used in this project, mainly provided by the SDK
LIBS		= c gcc hal phy pp net80211 wpa main lwip

# compiler flags using during compilation of source files
CFLAGS		= -Os -ggdb -std=c99 -Wpointer-arith -Wundef -Wall -Wl,-EL -fno-inline-functions \
		      -nostdlib -mlongcalls -mtext-section-literals -DICACHE_FLASH -Wno-address

# linker flags used to generate the main object file
LDFLAGS		= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static -L$(SDK_EXTRA_LIBS)

# linker script used for the above linkier step
LD_SCRIPT	= eagle.app.v6.ld
ifeq ("$(SDK_VERSION)","0.9.3")
LD_SCRIPT1      = eagle.app.v6.app1.ld
LD_SCRIPT2      = eagle.app.v6.app2.ld
else
LD_SCRIPT1      = eagle.app.v6.$(ESP_BOOT_VER).$(ESP_FLASH_SIZE).app1.ld
LD_SCRIPT2      = eagle.app.v6.$(ESP_BOOT_VER).$(ESP_FLASH_SIZE).app2.ld
endif
# various paths from the SDK used in this project
SDK_LIBDIR		= lib
SDK_LDDIR		= ld
SDK_INCDIR		= include include/json
SDK_BIN			= bin
SDK_TOOLSDIR	= tools

# we create two different files for uploading into the flash
# these are the names and options to generate them
FW_FILE_1		= 0x00000
FW_FILE_1_ARGS	= -bo $@ -bs .text -bs .data -bs .rodata -bc -ec
FW_FILE_2		= 0x40000
FW_FILE_2_ARGS	= -es .irom0.text $@ -ec
OTA_FW_FILE_1	= user1
OTA_FW_FILE_2	= user2
WEB_FW_FILE		= website

#Intermediate files for User1.bin and User2.bin
FW_PT_1_OFF			= 0x00000
#OTA_FW_1_PT_1	= 0x01000
OTA_FW_1_PT_1_ARGS	= -bo $(FW_PT_1) -bs .text -bs .data -bs .rodata -bc -ec

FW_PT_2_OFF			= 0x40000
OTA_FW_1_PT_2		= 0x11000
OTA_FW_1_PT_2_ARGS	= -es .irom0.text $(FW_PT_2) -ec

OTA_FW_2_PT_1		= 0x41000
OTA_FW_2_PT_1_ARGS	= -bo $(OTA_FW_2_PT_1) -bs .text -bs .data -bs .rodata -bc -ec

OTA_FW_2_PT_2		= 0x51000
OTA_FW_2_PT_2_ARGS	= -es .irom0.text $(OTA_FW_2_PT_2) -ec


# select which tools to use as compiler, librarian and linker
CC					:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR					:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD					:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
OBJCOPY				:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objcopy


####
#### no user configurable options below here
####
SRC_DIR			:= $(MODULES)
BUILD_DIR		:= $(addprefix $(BUILD_BASE)/,$(MODULES))
SDK_TOOLS		:= $(addprefix $(SDK_BASE)/,$(SDK_TOOLSDIR))
SDK_LIBDIR		:= $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR		:= $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))
SDK_BIN			:= $(addprefix $(SDK_BASE)/,$(SDK_BIN))
SRC				:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
OBJ				:= $(patsubst %.c,$(BUILD_BASE)/%.o,$(SRC))
LIBS			:= $(addprefix -l,$(LIBS))
APP_AR			:= $(addprefix $(BUILD_BASE)/,$(TARGET)_app.a)
TARGET_OUT		:= $(addprefix $(BUILD_BASE)/,$(TARGET).out)
BOOTLOADER		:= $(addprefix $(SDK_BIN)/,$(BOOTLOADER))
BLANK			:= $(addprefix $(SDK_BIN)/,$(BLANK))

JOIN_TOOL		:= $(addprefix $(SDK_TOOLS)/,$(JOIN_TOOL))
APPGEN_TOOL		:= $(addprefix $(SDK_TOOLS)/,$(APPGEN_TOOL))
ESPTOOLPY		:= $(addprefix $(SDK_ROOT)/,$(ESPTOOLPY))
ESPTOOLPYARGS	?= -o $(addprefix $(BUILD_BASE)/,$(TARGET).out-)
LD_SCRIPT		:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT))
LD_SCRIPT1		:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT1))
LD_SCRIPT2		:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT2))

INCDIR			:= $(addprefix -I,$(SRC_DIR))
EXTRA_INCDIR	:= $(addprefix -I,$(EXTRA_INCDIR))
MODULE_INCDIR	:= $(addsuffix /include,$(INCDIR))

FW_FILE_1		:= $(addprefix $(FW_BASE)/,$(FW_FILE_1).bin)
FW_FILE_2		:= $(addprefix $(FW_BASE)/,$(FW_FILE_2).bin)
OTA_FW_FILE_1	:= $(addprefix $(FW_BASE)/,$(OTA_FW_FILE_1).$(ESP_BOOT_VER).bin)
OTA_FW_FILE_2	:= $(addprefix $(FW_BASE)/,$(OTA_FW_FILE_2).$(ESP_BOOT_VER).bin)
WEB_FW_FILE		:= $(addprefix $(FW_BASE)/,$(WEB_FW_FILE).espfs)

FW_PT_1			:= $(addprefix $(BUILD_BASE)/,$(TARGET).out-$(FW_PT_1_OFF).bin)
FW_PT_2			:= $(addprefix $(BUILD_BASE)/,$(TARGET).out-$(FW_PT_2_OFF).bin)

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q 		:=
vecho 	:= @true
else
Q 		:= @
vecho 	:= @echo
endif

vpath %.c $(SRC_DIR)

ifdef DEBUG
CFLAGS	:= $(CFLAGS) -DDEBUG_VERSION=$(DEBUG)
endif

ifeq ("$(OTA)", "true")
CFLAGS	:= $(CFLAGS) -DOTA
endif

ifneq (,$findstring rawflash,$@)
CURRVER	= $(shell curl -s $(ESP_IPADDRESS)/getappver.cgi)
endif

CFLAGS := $(CFLAGS) -DSDK_VERSION=\"$(SDK_VERSION)\"
CFLAGS := $(CFLAGS) -DAPP_NAME=\"$(APP_NAME)\"
CFLAGS := $(CFLAGS) -DAPP_VERSION=\"$(APP_VERSION)\"

define compile-objects
$1/%.o: %.c
	$(vecho) "CC $$<"
	$(vecho) "$(Q) $(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS) -c $$< -o $$@"
	$(Q) $(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS)  -c $$< -o $$@
endef

.PHONY: all checkdirs clean

all: checkdirs $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2) website

ifeq ("$(ESP_BOOT_VER)","new")
$(OTA_FW_FILE_1): NEW_OTA_USER1
$(OTA_FW_FILE_2): NEW_OTA_USER2
else
$(OTA_FW_FILE_1): OLD_OTA_USER1
$(OTA_FW_FILE_2): OLD_OTA_USER2
endif

NEW_OTA_USER1: checkdirs $(TARGET_OUT)
	@$(OBJCOPY) --only-section .text -O binary $(TARGET_OUT)1 eagle.app.v6.text.bin
	@$(OBJCOPY) --only-section .data -O binary $(TARGET_OUT)1 eagle.app.v6.data.bin
	@$(OBJCOPY) --only-section .rodata -O binary $(TARGET_OUT)1 eagle.app.v6.rodata.bin
	@$(OBJCOPY) --only-section .irom0.text -O binary $(TARGET_OUT)1 eagle.app.v6.irom0text.bin
	@python $(APPGEN_TOOL) $(TARGET_OUT)1 $(BOOT_MODE) $(ESP_FLASH_MODE) $(ESP_FLASH_FREQ_DIV) $(ESP_SPI_SIZE)
	@rm -f eagle.app.v6.*
	
NEW_OTA_USER2: checkdirs $(TARGET_OUT)
	@$(OBJCOPY) --only-section .text -O binary $(TARGET_OUT)2 eagle.app.v6.text.bin
	@$(OBJCOPY) --only-section .data -O binary $(TARGET_OUT)2 eagle.app.v6.data.bin
	@$(OBJCOPY) --only-section .rodata -O binary $(TARGET_OUT)2 eagle.app.v6.rodata.bin
	@$(OBJCOPY) --only-section .irom0.text -O binary $(TARGET_OUT)2 eagle.app.v6.irom0text.bin
	@python $(APPGEN_TOOL) $(TARGET_OUT)2 $(BOOT_MODE) $(ESP_FLASH_MODE) $(ESP_FLASH_FREQ_DIV) $(ESP_SPI_SIZE)
	@rm -f eagle.app.v6.*

OLD_OTA_USER1: checkdirs $(TARGET_OUT)
	@$(ESPTOOLPY) elf2image $(ESPTOOLPYARGS) $(TARGET_OUT)1
	@$(JOIN_TOOL) $(FW_PT_1) $(TARGET_OUT)-$(OTA_FW_1_PT_2).bin
	
OLD_OTA_USER2: checkdirs $(TARGET_OUT)
	@$(ESPTOOLPY) elf2image $(ESPTOOLPYARGS) $(TARGET_OUT)2
	@$(JOIN_TOOL) $(FW_PT_1) $(TARGET_OUT)-$(OTA_FW_2_PT_2).bin
	
$(FW_FILE_1): FW_FILES
	$(vecho) "FW $@"
	@mv $(FW_PT_1) $(FW_FILE_1)

$(FW_FILE_2): FW_FILES
	$(vecho) "FW $@"
	@mv $(FW_PT_2) $(FW_FILE_2)

FW_FILES: checkdirs $(TARGET_OUT)
	$(Q) $(ESPTOOLPY) elf2image $(TARGET_OUT)
	
$(OTA_FW_FILE_1):
	$(vecho) "FW $@"
	@mv eagle.app.flash.bin $@

$(OTA_FW_FILE_2):
	$(vecho) "FW $@"
	@mv eagle.app.flash.bin $@

$(TARGET_OUT): $(APP_AR)
	$(vecho) "LD $@"
	$(Q) $(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@
	$(Q) $(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT1) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@1
	$(Q) $(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT2) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@2

$(APP_AR): $(OBJ)
	$(vecho) "AR $@"
	$(Q) $(AR) cru $@ $^

checkdirs: $(BUILD_DIR) $(FW_BASE)

$(BUILD_DIR):
	$(Q) mkdir -p $@

firmware:
	$(Q) mkdir -p $@

flash: $(FW_FILE_1) $(FW_FILE_2) website
	echo "Running..."
	$(ESPTOOLPY) --baud $(ESPBAUD) --port $(ESPPORT) write_flash 0x00000 firmware/0x00000.bin 0x12000 $(WEB_FW_FILE) 0x40000 firmware/0x40000.bin

cloud: $(OTA_FW_FILE_1) $(OTA_FW_FILE_2) website

flashcloud: cloud
	$(ESPTOOLPY) --baud $(ESPBAUD) --port $(ESPPORT) write_flash 0x00000 $(BOOTLOADER) 0x01000 $(OTA_FW_FILE_1) 0x41000 $(WEB_FW_FILE) 
	
website: html/ mkespfsimage/mkespfsimage
	$(vecho) "Building website..."
	$(Q) cd html; find | ../mkespfsimage/mkespfsimage > ../webpages.espfs; cd ..
	@if [ $$(stat -c '%s' webpages.espfs) -gt $$(( 0x2E000 )) ]; then echo "webpages.espfs too big!"; false; fi
	@mv webpages.espfs $(WEB_FW_FILE)

rawflashhtml: website
	$(vecho) "Flashing Web Application..."
	$(Q) curl -i -X POST $(ESP_IPADDRESS)/flashraw.cgi --data-binary "@$(WEB_FW_FILE)"
	$(vecho)
	
rawflashapp: cloud
	$(vecho) -n "Flashing OTA User Application "
	$(Q) if [ $(CURRVER) -eq 1 ]; then echo 2; else echo 1; fi
	$(Q) if [ $(CURRVER) -eq 1 ]; then \
		curl -i -X POST $(ESP_IPADDRESS)/flashapp.cgi --data-binary "@$(OTA_FW_FILE_2)"; else \
		curl -i -X POST $(ESP_IPADDRESS)/flashapp.cgi --data-binary "@$(OTA_FW_FILE_1)"; fi
	@sleep 2
	$(vecho)

rawflashwait:
	$(vecho) "Waiting for device to finish rebooting..."
	$(Q) curl --connect-timeout 1000 -s $(ESP_IPADDRESS)/getappver.cgi
	$(vecho)
	
rawflash: rawflashapp rawflashwait rawflashhtml
	$(vecho) "Your device should be ready to go.  Enjoy!"

mkespfsimage/mkespfsimage: mkespfsimage/
	make -C mkespfsimage

clean:
	$(Q) rm -f $(APP_AR)
	$(Q) rm -f $(TARGET_OUT)
	$(Q) find $(BUILD_BASE) -type f | xargs rm -f


	$(Q) rm -f $(FW_FILE_1)
	$(Q) rm -f $(FW_FILE_2)
	$(Q) rm -rf $(FW_BASE)

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))
