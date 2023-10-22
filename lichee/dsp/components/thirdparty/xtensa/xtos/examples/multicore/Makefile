
# Makefile for XTOS multicore example.
# Expects Xtensa tools in the path and XTENSA_SYSTEM and XTENSA_CORE set
# up properly in the environment.

# Detect host environment and select host tools

CONFIGDIR = $(shell xt-xcc --show-config=config)
include $(CONFIGDIR)/misc/hostenv.mk

ifeq ($(HOSTTYPE),win)
GREP = findstr
endif
ifeq ($(HOSTTYPE),unix)
GREP = grep
endif

# Detect target hardware type

VERSION = $(word 3, $(shell $(GREP) TargetHWVersion $(CONFIGDIR)$(S)config$(S)default-params))

ifneq (,$(findstring LX,$(VERSION)))
INC = template_lx.inc
else
INC = template_nx.inc
endif

# Set up other build params

CC     = xt-xcc
EXE    = mc_demo.exe
LSP    = sim-mc
CFLAGS = -Os -g -mlongcalls
MAP    = -Wl,-Map -Wl,mapfile


all : $(EXE)

run : all
	xtsc-run -define=PROG_NAME=$(EXE) \
	-define=XTSYSTEM=$(XTENSA_SYSTEM) \
	-define=XTCORE=$(XTENSA_CORE) \
	-define=XTPARAMS=$(XTENSA_PARAMS) \
	-include=$(INC)

clean :
	-$(RM) $(EXE) xtsc.log mapfile

.PHONY : all run clean


%.exe : %.c
	$(CC) $(CFLAGS) -o $@ -mlsp=$(LSP) $^ $(MAP)

