ifeq ($(CONFIG_ARCH_ARM_CORTEX_A7),y)
MULTIMEDIA_LIBS_DIR := components/aw/multimedia/lib_a7
SPEEX_LIBS_DIR := components/thirdparty/speex/libs/lib_a7
else ifeq ($(CONFIG_ARCH_ARM_CORTEX_M33),y)
MULTIMEDIA_LIBS_DIR := components/aw/multimedia/lib_m33
SPEEX_LIBS_DIR := components/thirdparty/speex/libs/lib_m33
else ifeq ($(CONFIG_ARCH_RISCV),y)
MULTIMEDIA_LIBS_DIR := components/aw/multimedia/lib_riscv
SPEEX_LIBS_DIR := components/thirdparty/speex/libs/lib_riscv
else ifeq ($(CONFIG_ARCH_DSP),y)
MULTIMEDIA_LIBS_DIR := components/aw/multimedia/lib_dsp
SPEEX_LIBS_DIR := components/thirdparty/speex/libs/lib_dsp
endif
LDFLAGS-$(CONFIG_RTOPUS_TEST) += -L$(MULTIMEDIA_LIBS_DIR) -law_opuscodec
LDFLAGS-$(CONFIG_LIB_RECORDER) += -L$(MULTIMEDIA_LIBS_DIR) -lxrecorder -lstream -larecoder -lawrecorder -lmuxer -lrecord -law_amrenc -lcdx_base
ifeq ($(CONFIG_ARCH_SUN20IW2),y)
  LDFLAGS-$(CONFIG_LIB_RECORDER) += -L$(MULTIMEDIA_LIBS_DIR) -law_mp3enc
endif
#-law_oggdec
#LDFLAGS-$(CONFIG_RTPLAYER_TEST) += -L$(MULTIMEDIA_LIBS_DIR) -lrtplayer -lxplayer -lcdx_base -lplayback -lparser \
#			-lstream -ladecoder -law_aacdec -law_mp3dec -law_wavdec -law_opuscodec
LDFLAGS-$(CONFIG_LIB_MULTIMEDIA) += -L$(MULTIMEDIA_LIBS_DIR) -lrtplayer -lxplayer -lplayback -lparser -lstream -lcdx_base -ladecoder -law_aacdec -law_mp3dec -law_opuscodec  -law_wavdec -ladecoder -law_flacdec -law_amrenc -law_amrdec -law_oggdec

#LDFLAGS-$(CONFIG_RTPLAYER_TEST) += -L$(MULTIMEDIA_LIBS_DIR) -lrtosplayer -law_aacdec -law_mp3dec -law_opuscodec  -law_wavdec -lrtosplayer

LDFLAGS-$(CONFIG_COMPONENT_SPEEX) += -L$(SPEEX_LIBS_DIR) -law_speex

ifeq ($(CONFIG_COMPONENTS_USB_GADGET_ADBD),y)
       LDFLAGS += -Lcomponents/common/aw/usb/gadget/adbd/ -law-adbd
endif

ifeq ($(CONFIG_DRIVERS_XRADIO),y)
  ifeq ($(CONFIG_DRIVER_R128), y)
	LDFLAGS += -Ldrivers/drv/wireless/xradio/lib/sun20iw2
  else
	LDFLAGS += -Ldrivers/drv/wireless/xradio/lib
  endif

  ifeq ($(CONFIG_DRIVER_R128), y)
    ifeq ($(CONFIG_WLAN_STA)_$(CONFIG_WLAN_AP), y_y)
      ifeq ($(CONFIG_WLAN_STA_WPS), y)
        LIB_WPA += -lxrwifi_wpas_wps_hostapd
      else
        LIB_WPA += -lxrwifi_wpas_hostapd
      endif
    else
      ifeq ($(CONFIG_WLAN_STA), y)
        ifeq ($(CONFIG_WLAN_STA_WPS), y)
          LIB_WPA += -lxrwifi_wpas_wps
        else
          LIB_WPA += -lxrwifi_wpas
        endif
      endif
      ifeq ($(CONFIG_WLAN_AP), y)
        LIB_WPA += -lxrwifi_hostapd
      endif
    endif
  endif

  ifeq ($(CONFIG_ETF), y)
    ifeq ($(CONFIG_DRIVER_R128), y)
      LDFLAGS += -lxrwifi_etf
    else
      LDFLAGS += -lxretf
    endif
  else
    ifeq ($(CONFIG_DRIVER_XR819), y)
      LDFLAGS += -lxr819
    else ifeq ($(CONFIG_DRIVER_XR829)_$(CONFIG_XR829_40M_FW), y_y)
      LDFLAGS += -lxr829_40M
    else ifeq ($(CONFIG_DRIVER_R128), y)
      ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
        ifeq ($(CONFIG_COMPONENTS_AMP), y)
          LDFLAGS += -lxrwifi_wlan_m33
        else
          LDFLAGS += -lxrwifi_wlan
        endif
        LDFLAGS += -lxrwifi_mac $(LIB_WPA)
        LDFLAGS += -lxrwifi_wireless_phy
      endif
      ifeq ($(CONFIG_COMPONENTS_AMP)_$(CONFIG_ARCH_RISCV_RV64), y_y)
        LDFLAGS += -lxrwifi_wlan_rv
      endif
    endif
  endif
endif


