CFLAGS += -Icomponents/common/thirdparty/libjpeg-turbo/include
CFLAGS += -Icomponents/common/thirdparty/libjpeg-turbo/source

ifeq ($(CONFIG_COMPONENTS_LIBJPEG_TURBO), y)
obj-y += source/jdicc.o
obj-y += source/wrtarga.o
obj-y += source/wrppm.o
obj-y += source/jdhuff.o
obj-y += source/jdphuff.o
obj-y += source/jdcolor.o
obj-y += source/jquant2.o
obj-y += source/jdmainct.o
obj-y += source/jdcoefct.o
obj-y += source/jaricom.o
obj-y += source/jdarith.o
obj-y += source/jidctflt.o
obj-y += source/jidctfst.o
obj-y += source/jidctint.o
obj-y += source/jidctred.o
obj-y += source/jddctmgr.o
obj-y += source/jdpostct.o
obj-y += source/jdmerge.o
obj-y += source/jquant1.o
obj-y += source/jsimd_none.o
obj-y += source/jdsample.o
obj-y += source/jdapistd.o
obj-y += source/jdmaster.o
obj-y += source/wrbmp.o
obj-y += source/jdatasrc.o
obj-y += source/cdjpeg.o
obj-y += source/jdinput.o
obj-y += source/jutils.o
obj-y += source/jmemnobs.o
obj-y += source/jmemmgr.o
obj-y += source/jdapimin.o
obj-y += source/jcomapi.o
obj-y += source/jerror.o
obj-y += source/rdcolmap.o
obj-y += source/jdmarker.o
obj-y += source/rdppm.o
obj-y += source/rdbmp.o
obj-y += source/jctrans.o
obj-y += source/jdtrans.o
obj-y += source/transupp.o
obj-y += source/jchuff.o
obj-y += source/jcphuff.o
obj-y += source/jcmarker.o
obj-y += source/jcmainct.o
obj-y += source/jccoefct.o
obj-y += source/jcarith.o
obj-y += source/jfdctflt.o
obj-y += source/jfdctfst.o
obj-y += source/jfdctint.o
obj-y += source/jcdctmgr.o
obj-y += source/jcprepct.o
obj-y += source/jcsample.o
obj-y += source/jccolor.o
obj-y += source/jcmaster.o
obj-y += source/jcinit.o
obj-y += source/jcapistd.o
obj-y += source/jdatasrc-tj.o
obj-y += source/jcparam.o
obj-y += source/jdatadst-tj.o
obj-y += source/jcapimin.o
obj-y += source/turbojpeg.o
obj-y += source/jcicc.o
obj-y += source/jdatadst.o
obj-y += source/rdtarga.o
obj-y += source/rdswitch.o
endif

obj-$(CONFIG_LIBJPEG_DJPEG) += source/djpeg.o
obj-$(CONFIG_LIBJPEG_CJPEG) += source/cjpeg.o
obj-$(CONFIG_LIBJPEG_TEST_DEMO) += source/tjexample.o
obj-$(CONFIG_LIBJPEG_TEST_DEMO) += source/tj_test.o
