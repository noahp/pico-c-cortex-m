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
