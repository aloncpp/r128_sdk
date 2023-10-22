obj-$(CONFIG_COMPONENT_TINYCRYPT) += source/cmac_mode.o
obj-$(CONFIG_COMPONENT_TINYCRYPT) += source/aes_encrypt.o
obj-$(CONFIG_COMPONENT_TINYCRYPT) += source/utils.o
obj-$(CONFIG_COMPONENT_TINYCRYPT) += source/sha256.o
obj-$(CONFIG_COMPONENT_TINYCRYPT) += source/hmac.o
obj-$(CONFIG_COMPONENT_TINYCRYPT) += source/hmac_prng.o
obj-$(CONFIG_COMPONENT_TINYCRYPT) += source/ecc.o
obj-$(CONFIG_COMPONENT_TINYCRYPT) += source/ecc_dh.o

#ifeq ($(CONFIG_COMPONENT_TINYCRYPT), y)
CFLAGS += -Icomponents/thirdparty/tinycrypt/include
#endif

