# mbedtls-2.7.0
# MBEDTLS_PROGRAMS_FILE
obj-$(CONFIG_COMPONENTS_MBEDTLS) += programs/wince_main.o  \
		       programs/ssl/ssl_server.o  \
		       programs/ssl/ssl_server2.o  \
		       programs/ssl/ssl_client2.o  \
		       programs/ssl/ssl_mail_client.o  \
		       programs/ssl/dtls_client.o  \
		       programs/ssl/ssl_fork_server.o  \
		       programs/ssl/ssl_pthread_server.o  \
		       programs/ssl/mini_client.o  \
		       programs/ssl/dtls_server.o  \
		       programs/ssl/ssl_client1.o  \
		       programs/x509/crl_app.o  \
		       programs/x509/cert_app.o  \
		       programs/x509/cert_req.o  \
		       programs/x509/cert_write.o  \
		       programs/x509/req_app.o  \
		       programs/util/strerror.o  \
		       programs/util/pem2der.o  \
		       programs/pkey/key_app.o  \
		       programs/pkey/rsa_decrypt.o  \
		       programs/pkey/rsa_verify.o  \
		       programs/pkey/ecdh_curve25519.o  \
		       programs/pkey/rsa_sign.o  \
		       programs/pkey/dh_server.o  \
		       programs/pkey/key_app_writer.o  \
		       programs/pkey/pk_verify.o  \
		       programs/pkey/rsa_genkey.o  \
		       programs/pkey/mpi_demo.o  \
		       programs/pkey/dh_client.o  \
		       programs/pkey/ecdsa.o  \
		       programs/pkey/pk_sign.o  \
		       programs/pkey/gen_key.o  \
		       programs/pkey/dh_genprime.o  \
		       programs/pkey/rsa_sign_pss.o  \
		       programs/pkey/rsa_verify_pss.o  \
		       programs/pkey/rsa_encrypt.o  \
		       programs/pkey/pk_decrypt.o  \
		       programs/pkey/pk_encrypt.o  \
		       programs/hash/generic_sum.o  \
		       programs/hash/hello.o  \
		       programs/aes/crypt_and_hash.o  \
		       programs/aes/aescrypt2.o  \
		       programs/test/selftest.o  \
		       programs/test/ssl_cert_test.o  \
		       programs/test/udp_proxy.o  \
		       programs/test/benchmark.o  \
		       programs/random/gen_random_havege.o  \
		       programs/random/gen_entropy.o  \
		       programs/random/gen_random_ctr_drbg.o
# MEBDTLS_LIBRARY_FILES 
obj-$(CONFIG_COMPONENTS_MBEDTLS) += library/cmac.o \
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
		     library/xtea.o  \
                     library/x509_create.o  \
                     library/pem.o  \
                     library/rsa_internal.o  \
                     library/entropy.o  \
                     library/md5.o  \
                     library/ccm.o  \
                     library/md2.o  \
                     library/pk.o  \
                     library/bignum.o  \
                     library/camellia.o  \
                     library/platform.o  \
                     library/rsa.o  \
                     library/gcm.o  \
                     library/version_features.o  \
                     library/md4.o  \
                     library/pkcs5.o  \
                     library/debug.o  \
                     library/sha1.o  \
                     library/ssl_cookie.o  \
                     library/des.o  \
                     library/certs.o  \
                     library/ssl_srv.o  \
                     library/x509_crl.o  \
                     library/pkparse.o  \
                     library/ssl_tls.o  \
                     library/ecp.o  \
                     library/ecdh.o  \
                     library/error.o  \
                     library/entropy_poll.o  \
                     library/pkcs12.o  \
                     library/ssl_cli.o  \
                     library/blowfish.o  \
                     library/pkcs11.o  \
                     library/pkwrite.o \
                     library/mbedtls.o
obj-$(CONFIG_COMPONENTS_MBEDTLS_TEST)+= yotta/data/entropy_hardware_poll.o \
                    			yotta/data/example-authcrypt/authcrypt.o \
                    			yotta/data/example-benchmark/benchmark.o \
                    			yotta/data/example-hashing/hashing.o \
								yotta/data/example-selftest/selftest.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

CFLAGS += -Iinclude/FreeRTOS_POSIX/ \
 	      -Icomponents/common/thirdparty/network/include \
 	      -Icomponents/common/thirdparty/network/mbedtls-2.7.0 \
 	      -Icomponents/common/thirdparty/network/mbedtls-2.7.0/include \
 	      -Icomponents/common/thirdparty/network/ \
 	      -Icomponents/common/thirdparty/network/$(LWIP)/src/include/posix \
 	      -Icomponents/common/thirdparty/network/$(LWIP)/src/include/posix/sys/ \
 	      -Icomponents/common/thirdparty/network/$(LWIP)/src/include/lwip/ \
 	      -Icomponents/common/thirdparty/network/$(LWIP)/src/include/ \
		  -Icomponents/common/aw/xradio/include/ 
CXXFLAGS += -Iinclude/FreeRTOS_POSIX/ \
          	-Icomponents/common/thirdparty/network/include \
     	         -Icomponents/common/thirdparty/network/mbedtls-2.7.0 \
     	         -Icomponents/common/thirdparty/network/mbedtls-2.7.0/include \
     	         -Icomponents/common/thirdparty/network/ \
     	         -Icomponents/common/thirdparty/network/$(LWIP)/src/include/posix \
     	         -Icomponents/common/thirdparty/network/$(LWIP)/src/include/posix/sys/ \
     	         -Icomponents/common/thirdparty/network/$(LWIP)/src/include/lwip/ \
     	         -Icomponents/common/thirdparty/network/$(LWIP)/src/include/

CFLAGS += -DMBEDTLS_ON_LWIP  \
	      -DLWIP_ERRNO_INCLUDE=1

$(eval $(call BuildPackage,mbedtls))