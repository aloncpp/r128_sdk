IO_MULTIPLEX_DIR := components/thirdparty/io_multiplex

CFLAGS += -I $(IO_MULTIPLEX_DIR)/include \
	  -I components/thirdparty/finsh_cli \
	  -I components/thirdparty/ulog
obj-$(CONFIG_COMPONENT_IO_MULTIPLEX) += poll.o
obj-$(CONFIG_COMPONENT_IO_MULTIPLEX) += select.o
obj-$(CONFIG_COMPONENT_IO_MULTIPLEX) += test/test_io_multiplex.o
