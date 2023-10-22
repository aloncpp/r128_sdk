#nghttp2
obj-y += src/nghttp2_session.o \
      	  	   src/nghttp2_npn.o \
      	 	   src/nghttp2_option.o \
      	 	   src/nghttp2_stream.o \
      	 	   src/nghttp2_callbacks.o \
      	 	   src/nghttp2_buf.o \
      	 	   src/nghttp2_frame.o \
      	 	   src/nghttp2_submit.o \
      	 	   src/nghttp2_hd.o \
      	 	   src/nghttp2_map.o \
      	 	   src/nghttp2_outbound_item.o \
      	 	   src/nghttp2_queue.o \
      	 	   src/nghttp2_http.o \
      	 	   src/nghttp2_priority_spec.o \
      	 	   src/nghttp2_version.o \
      	 	   src/nghttp2_net.o \
      	 	   src/nghttp2_pq.o \
      	 	   src/nghttp2_hd_huffman.o \
      	 	   src/nghttp2_mem.o \
      	 	   src/nghttp2_helper.o \
      	 	   src/nghttp2_hd_huffman_data.o
			   
ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

CFLAGS += -Iinclude/FreeRTOS_POSIX/

CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include/lwip
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include/posix
CFLAGS += -Icomponents/common/thirdparty/network/nghttp2
CFLAGS += -Icomponents/common/thirdparty/network/nghttp2/src
CFLAGS += -Icomponents/common/thirdparty/network/nghttp2/include
CFLAGS += -Icomponents/common/aw/xradio/include

CC_FLAGS += -DDHCPD_LWIP \
            -DDHCPD_HEAP_REPLACE_STACK \
            -DDHCPD_USRCFG \
            -DDHCPD_FREERTOS \
            -DDHCPD_ICMPPING \
            -DDHCPD_LOW_LEVEL \
            -DDHCPD_DNS \
            -DDHCPD_USE_DEFAULT_INIT  \
	    -DLWIP_ERRNO_INCLUDE=1

CFLAGS += -I $(CC_FLAGS)