MODULE := backends/platform/n64
MODULE_OBJS := \
    nintendo64.o \
    osys_n64.o

MODULE_OBJS := $(addprefix $(MODULE)/, $(MODULE_OBJS))
OBJS := $(MODULE_OBJS) $(OBJS)
MODULE_DIRS += $(sort $(dir $(MODULE_OBJS)))
