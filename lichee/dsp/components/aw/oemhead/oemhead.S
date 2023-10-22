#include "version.h"

.section .oemhead.text,"ax"
.incbin "components/aw/oemhead/oemhead.b"
.asciz SUB_VER
