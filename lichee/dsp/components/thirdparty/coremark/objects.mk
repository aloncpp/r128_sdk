
COMPONENTS_COREMARK_OBJECTS += $(BUILD_DIR)/components/thirdparty/coremark/barebones/core_portme.o
COMPONENTS_COREMARK_OBJECTS += $(BUILD_DIR)/components/thirdparty/coremark/barebones/cvt.o
COMPONENTS_COREMARK_OBJECTS += $(BUILD_DIR)/components/thirdparty/coremark/barebones/ee_printf.o
COMPONENTS_COREMARK_OBJECTS += $(BUILD_DIR)/components/thirdparty/coremark/core_list_join.o
COMPONENTS_COREMARK_OBJECTS += $(BUILD_DIR)/components/thirdparty/coremark/core_main.o
COMPONENTS_COREMARK_OBJECTS += $(BUILD_DIR)/components/thirdparty/coremark/core_matrix.o
COMPONENTS_COREMARK_OBJECTS += $(BUILD_DIR)/components/thirdparty/coremark/core_state.o
COMPONENTS_COREMARK_OBJECTS += $(BUILD_DIR)/components/thirdparty/coremark/core_util.o

$(COMPONENTS_COREMARK_OBJECTS):CFLAGS += -I $(BASE)/components/thirdparty/coremark
$(COMPONENTS_COREMARK_OBJECTS):CFLAGS += -I $(BASE)/components/thirdparty/coremark/barebones

$(COMPONENTS_COREMARK_OBJECTS):MODULE_NAME="components-coremark"

OBJECTS += $(COMPONENTS_COREMARK_OBJECTS)
