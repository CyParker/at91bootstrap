$(shell $(CC) --target-help > tmp.file)
gcc_cortexa5=$(shell grep cortex-a5 tmp.file)

ifeq (, $(findstring cortex-a5,$(gcc_cortexa5)))
CPPFLAGS += -DCONFIG_SAMA5D3_FSW

ASFLAGS += \
	-DCONFIG_SAMA5D3_FSW
else
CPPFLAGS += \
	-DCONFIG_SAMA5D3_FSW \
	-mtune=cortex-a5

ASFLAGS += \
	-DCONFIG_SAMA5D3_FSW \
	-mcpu=cortex-a5
endif

$(shell rm tmp.file)
