CFLAGS += -DFT2_BUILD_LIBRARY

CFLAGS += -I components/common/thirdparty/freetype/include/

ifeq ($(CONFIG_LVGL8_USE_FREETYPE),y)
CFLAGS += -DFT_CONFIG_MODULES_H=\"../../littlevgl-8/lvgl/src/extra/libs/freetype/ftmodule.h\"
CFLAGS += -DFT_CONFIG_OPTIONS_H=\"../../littlevgl-8/lvgl/src/extra/libs/freetype/ftoption.h\"
endif

FREETYPE_OBJ += src/type42/type42.o
FREETYPE_OBJ += src/base/ftsynth.o
FREETYPE_OBJ += src/type42/type42.o
FREETYPE_OBJ += src/base/ftsynth.o
FREETYPE_OBJ += src/base/ftsystem.o
FREETYPE_OBJ += src/base/ftglyph.o
FREETYPE_OBJ += src/psnames/psnames.o
FREETYPE_OBJ += src/base/ftbbox.o
FREETYPE_OBJ += src/truetype/truetype.o
FREETYPE_OBJ += src/pfr/pfr.o
FREETYPE_OBJ += src/base/ftinit.o
FREETYPE_OBJ += src/sfnt/sfnt.o
FREETYPE_OBJ += src/raster/raster.o
FREETYPE_OBJ += src/cache/ftcache.o
FREETYPE_OBJ += src/bzip2/ftbzip2.o
FREETYPE_OBJ += src/lzw/ftlzw.o
FREETYPE_OBJ += src/psaux/psaux.o
FREETYPE_OBJ += src/base/ftpatent.o
FREETYPE_OBJ += src/base/ftmm.o
FREETYPE_OBJ += src/base/ftcid.o
FREETYPE_OBJ += src/winfonts/winfnt.o
FREETYPE_OBJ += src/sdf/sdf.o
FREETYPE_OBJ += src/pshinter/pshinter.o
FREETYPE_OBJ += src/pcf/pcf.o
FREETYPE_OBJ += src/base/ftfstype.o
FREETYPE_OBJ += src/bdf/bdf.o
FREETYPE_OBJ += src/svg/svg.o
FREETYPE_OBJ += src/base/ftbase.o
FREETYPE_OBJ += src/base/fttype1.o
FREETYPE_OBJ += src/base/ftbdf.o
FREETYPE_OBJ += src/base/ftgasp.o
FREETYPE_OBJ += src/cff/cff.o
FREETYPE_OBJ += src/base/ftwinfnt.o
FREETYPE_OBJ += src/base/ftpfr.o
FREETYPE_OBJ += src/base/ftstroke.o
FREETYPE_OBJ += src/cid/type1cid.o
FREETYPE_OBJ += src/gzip/ftgzip.o
FREETYPE_OBJ += src/autofit/autofit.o
FREETYPE_OBJ += src/base/ftotval.o
FREETYPE_OBJ += src/type1/type1.o
FREETYPE_OBJ += src/smooth/smooth.o
FREETYPE_OBJ += src/base/ftbitmap.o
FREETYPE_OBJ += src/base/ftgxval.o
FREETYPE_OBJ += src/base/ftdebug.o
#FREETYPE_OBJ += src/dlg/dlg.o
#FREETYPE_OBJ += src/base/ftobjs.o

obj-y += $(FREETYPE_OBJ)
