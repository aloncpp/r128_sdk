# mbedtls-2.16.0
# MBEDTLS_LIBRABY_FILES
obj-y += library/cmac.o \
         library/chacha20.o \
         library/chachapoly.o \
         library/poly1305.o \
         library/md.o \
         library/sha256.o \
         library/arc4.o \
         library/net_sockets.o \
         library/aes.o \
         library/x509write_csr.o \
         library/hmac_drbg.o \
         library/dhm.o \
         library/x509.o \
         library/ripemd160.o \
         library/version.o \
         library/x509_crt.o \
         library/ssl_ciphersuites.o \
         library/base64.o \
         library/ssl_cache.o \
         library/ctr_drbg.o \
         library/cipher.o \
         library/x509_csr.o \
         library/padlock.o \
         library/ecp_curves.o \
         library/sha512.o \
         library/asn1write.o \
         library/ssl_ticket.o \
         library/ecjpake.o \
         library/threading.o \
         library/pk_wrap.o \
         library/aesni.o \
         library/oid.o \
         library/asn1parse.o \
         library/cipher_wrap.o \
         library/x509write_crt.o \
         library/timing.o \
         library/ecdsa.o \
         library/havege.o \
         library/md_wrap.o \
         library/memory_buffer_alloc.o \
         library/xtea.o \
         library/x509_create.o \
         library/pem.o \
         library/rsa_internal.o \
         library/entropy.o \
         library/md5.o \
         library/ccm.o \
         library/md2.o \
         library/pk.o \
         library/bignum.o \
         library/camellia.o \
         library/platform.o \
         library/platform_util.o \
         library/rsa.o \
         library/gcm.o \
         library/version_features.o \
         library/md4.o \
         library/pkcs5.o \
         library/debug.o \
         library/sha1.o \
         library/ssl_cookie.o \
         library/des.o \
         library/certs.o \
         library/ssl_srv.o \
         library/x509_crl.o \
         library/pkparse.o \
         library/ssl_tls.o \
         library/ecp.o \
         library/ecdh.o \
         library/error.o \
         library/entropy_poll.o \
         library/pkcs12.o \
         library/ssl_cli.o \
         library/blowfish.o \
         library/pkcs11.o \
         library/pkwrite.o \
         library/nist_kw.o \
         library/hkdf.o \
         library/aria.o \
         library/mbedtls.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
else
LWIP = lwip-2.1.2
endif

CFLAGS += -Iinclude/FreeRTOS_POSIX
CFLAGS += -Icomponents/common/thirdparty/network/include
CFLAGS += -Icomponents/common/thirdparty/network/mbedtls-2.16.0
CFLAGS += -Icomponents/common/thirdparty/network/mbedtls-2.16.0/include
CFLAGS += -Icomponents/common/thirdparty/network
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include/posix
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include/posix/sys/
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include/lwip
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -Icomponents/common/aw/xradio/include
CFLAGS += -Icomponents/common/aw/xradio/include/libc
CFLAGS += -Icomponents/common/thirdparty/network/mbedtls-2.16.0/include/mbedtls/configs
#PLT DEF
ifeq ($(CONFIG_ARCH_SUN8IW18P1), y)
CFLAGS += -Itools/gcc-arm-melis-eabi-8-2019-q3-update/arm-melis-eabi/include
CFLAGS += -Ikernel/FreeRTOS/Source/include/
CFLAGS += -Ikernel/FreeRTOS/Source/port/
CFLAGS += -Ikernel/FreeRTOS/Source/portable/GCC/ARM_CA9
else ifeq ($(CONFIG_ARCH_SUN20IW2P1), y)
ifeq ($(CONFIG_ARCH_RISCV)_$(CONFIG_ARCH_RISCV_C906), y_y)
CFLAGS += -Itools/riscv64-elf-x86_64-20201104/include
CFLAGS += -Ikernel/FreeRTOS-orig/Source/include/
CFLAGS += -Ikernel/FreeRTOS-orig/Source/port/
CFLAGS += -Ikernel/FreeRTOS-orig/Source/portable/GCC/RISC-V/
CFLAGS += -Iinclude/arch/riscv/
CFLAGS += -Iarch/risc-v/
else ifeq ($(CONFIG_ARCH_ARM_CORTEX_M33), y)
CFLAGS += -Itools/gcc-arm-none-eabi-8-2019-q3-update/arm-none-eabi/include
CFLAGS += -Ikernel/FreeRTOS-orig/Source/include/
CFLAGS += -Ikernel/FreeRTOS-orig/Source/port/
CFLAGS += -Ikernel/FreeRTOS-orig/Source/portable/GCC/ARM_CM33_NTZ/non_secure/
CFLAGS += -Iarch/arm/armv8m/include/cmsis/
endif
endif

CFLAGS += -Icomponents/common/aw/xradio/include/
CFLAGS += -DMBEDTLS_CONFIG_FILE='<config-xr-mini-cliserv.h>'
