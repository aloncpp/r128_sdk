
TINATEST_CORE = src/core/tt.o \
		src/core/task.o

TINATEST_INTERACT = src/interact/interact.o \
		    src/interact/interact-actor.o

TINATEST_OUTLOG = src/outlog/outlog.o \
		  src/outlog/outlog_serial.o \
		  src/outlog/outlog_dragonmat.o

TINATEST_SOCKET_DB = src/socket_db/init_entry.o \
		     src/socket_db/xml_packet.o \
		     src/socket_db/socket_main.o

obj-y += $(TINATEST_CORE)
obj-y += $(TINATEST_INTERACT)
obj-y += $(TINATEST_OUTLOG)
obj-y += $(TINATEST_SOCKET_DB)

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/compat/posix
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/posix
endif

CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I components/thirdparty/console
CFLAGS += -I components/common/aw/tinatest/include
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include

obj-$(CONFIG_COMPONENTS_AW_TINATEST) += testcase/
