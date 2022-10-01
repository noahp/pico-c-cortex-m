# disable echoing rule commands, run make with --trace to see them
.SILENT:

ARM_CC ?= arm-none-eabi-gcc
# if cc isn't set by the user, set it to ARM_CC
ifeq ($(origin CC),default)
CC := $(ARM_CC)
endif

# use ccache if available
CCACHE := $(shell command -v ccache 2> /dev/null)
ifdef CCACHE
CC := ccache $(CC)
endif

RM = rm -rf

BUILDDIR = build
TARGET = $(BUILDDIR)/main.elf

ARCHFLAGS +=
GDB_RELOAD_CMD =

CFLAGS += \
  -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
  -Os -ggdb3 -std=c11 \
  -fdebug-prefix-map=$(abspath .)=. \
  -I. \
  -ffunction-sections -fdata-sections \
  -Werror \
  -Wall \
  -Wextra \
  -Wundef \

# Add a git version identifier; either clean tag, or with -dirty suffix if
# working tree is dirty.
GIT_VERSION ?= $(shell git describe --dirty --always --tags)

CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"

LDFLAGS += \
  --specs=rdimon.specs \
  --specs=nano.specs \
  -Wl,--gc-sections,-Map,$(TARGET).map,--build-id \
  -Wl,--print-memory-usage

SRCS += \
  main.c

all: $(TARGET)

OBJS = $(patsubst %.c, %.o, $(SRCS))
OBJS := $(addprefix $(BUILDDIR)/,$(OBJS))

# add a binary file that's generated separately from the main compilation steps:
# 1. generate the raw data
$(BUILDDIR)/calibration_constants.bin: calibration_data.json
	./generate_calibration_constants.py calibration_data.json $@

# 2. convert the binary calibration data into an object file that can be linked
#    into the main application. rename the auto-generated symbols for the start
#    and end of the object data so we can reference them from the application.
#    rename the section to something that can be placed explicitly from the
#    linker script, if necessary
$(BUILDDIR)/calibration_constants.o: $(BUILDDIR)/calibration_constants.bin
	mkdir -p $(dir $@)
	arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm \
		--rename-section .data=.calibration_constants $^ $@

ifneq ($(INCLUDE_CALIBRATION_BIN),)
OBJS += $(BUILDDIR)/calibration_constants.o
endif

# depfiles for tracking include changes
DEPFILES = $(OBJS:%.o=%.o.d)
DEPFLAGS = -MT $@ -MMD -MP -MF $@.d
-include $(DEPFILES)

$(BUILDDIR):
	mkdir -p $@

clean:
	$(RM) $(BUILDDIR)

$(BUILDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(info Compiling $<)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(TARGET): stm32f407.ld $(OBJS)
	$(info Linking $@)
	$(CC) $(CFLAGS) -T$^ $(LDFLAGS) -o $@
	arm-none-eabi-size $(TARGET)

debug: $(TARGET)
	openocd -f stm32f4.openocd.cfg

gdb: $(TARGET)
	arm-none-eabi-gdb-py $(TARGET) -ex "source .gdb-startup" -ex "openocd-reload"

.PHONY: all clean flash gdb
