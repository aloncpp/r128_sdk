#ifndef MEM_API_H
#define MEM_API_H

int sram_dbus_cpu_write_512k(uint32_t start_addr);
int psram_assembly_write(uint32_t start_addr, uint32_t len);
int psram_assembly_read(uint32_t start_addr, uint32_t len);

#endif
