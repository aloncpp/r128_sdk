LVDRVDIR=../lv_drivers

LVGL_DISPFILES=$(LVDRVDIR)/display/drm.o \
	$(LVDRVDIR)/display/fbdev.o \
	$(LVDRVDIR)/display/GC9A01.o \
	$(LVDRVDIR)/display/ILI9341.o \
	$(LVDRVDIR)/display/R61581.o \
	$(LVDRVDIR)/display/SHARP_MIP.o \
	$(LVDRVDIR)/display/SSD1963.o \
	$(LVDRVDIR)/display/ST7565.o \
	$(LVDRVDIR)/display/UC1610.o \
	$(LVDRVDIR)/display/sunximem.o \
	$(LVDRVDIR)/display/sunxig2d.o \
	$(LVDRVDIR)/display/spilcd.o \
	$(LVDRVDIR)/display/disp_cfg_layer.o \
	$(LVDRVDIR)/display/disp2.o \
	$(LVDRVDIR)/display/sunxifb.o

LVGL_GTKDRVFILES=$(LVDRVDIR)/gtkdrv/gtkdrv.o

LVGL_INDEVFILES=$(LVDRVDIR)/indev/AD_touch.o \
	$(LVDRVDIR)/indev/evdev.o \
	$(LVDRVDIR)/indev/FT5406EE8.o \
	$(LVDRVDIR)/indev/libinput.o \
	$(LVDRVDIR)/indev/xkb.o \
	$(LVDRVDIR)/indev/XPT2046.o

LVGL_SDLFILES=$(LVDRVDIR)/sdl/sdl.o \
	$(LVDRVDIR)/sdl/sdl_gpu.o

LVGL_WAYLANDFILES=$(LVDRVDIR)/wayland/wayland.o

LVGL_WIN32DRVFILES=$(LVDRVDIR)/win32drv/win32drv.o

LVDRIVERS_OBJECTS=$(LVDRVDIR)/win_drv.o \
	$(LVGL_DISPFILES) \
	$(LVGL_GTKDRVFILES) \
	$(LVGL_INDEVFILES) \
	$(LVGL_SDLFILES) \
	$(LVGL_WAYLANDFILES) \
	$(LVGL_WIN32DRVFILES)

obj-y += $(LVDRIVERS_OBJECTS)
