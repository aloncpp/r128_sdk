#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <csr.h>
#include <console.h>

int cmd_csr_read(int argc, char **argv)
{
    unsigned long csr_value = 0;
    if (argc < 2)
    {
        printf("Usage: csr_read csr\n");
        return -1;
    }

    if (!strcmp(argv[1], "mstatus"))
    {
        csr_value = csr_read(mstatus);
    }
    else if (!strcmp(argv[1], "mepc"))
    {
        csr_value = csr_read(mepc);
    }
    else if (!strcmp(argv[1], "mtvec"))
    {
        csr_value = csr_read(mtvec);
    }
    else if (!strcmp(argv[1], "mcause"))
    {
        csr_value = csr_read(mcause);
    }
    else if (!strcmp(argv[1], "mie"))
    {
        csr_value = csr_read(mie);
    }
    else if (!strcmp(argv[1], "mip"))
    {
        csr_value = csr_read(mip);
    }
    else if (!strcmp(argv[1], "mtval"))
    {
        csr_value = csr_read(mtval);
    }
    else if (!strcmp(argv[1], "mscratch"))
    {
        csr_value = csr_read(mscratch);
    }
    else if (!strcmp(argv[1], "mhcr"))
    {
        csr_value = csr_read(mhcr);
    }
	else if (!strcmp(argv[1], "sstatus"))
    {
        csr_value = csr_read(sstatus);
    }
    else if (!strcmp(argv[1], "sepc"))
    {
        csr_value = csr_read(sepc);
    }
    else if (!strcmp(argv[1], "stvec"))
    {
        csr_value = csr_read(stvec);
    }
    else if (!strcmp(argv[1], "scause"))
    {
        csr_value = csr_read(scause);
    }
    else if (!strcmp(argv[1], "sie"))
    {
        csr_value = csr_read(sie);
    }
    else if (!strcmp(argv[1], "sip"))
    {
        csr_value = csr_read(sip);
    }
    else if (!strcmp(argv[1], "stval"))
    {
        csr_value = csr_read(stval);
    }
    else if (!strcmp(argv[1], "sscratch"))
    {
        csr_value = csr_read(sscratch);
    }
    else
    {
        printf("can not support the csr\n");
        return -1;
    }

    printf("%s:0x%lx\n", argv[1], csr_value);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_csr_read, csr_read, read csr);

int cmd_csr_write(int argc, char **argv)
{
    unsigned long csr_value = 0;
    char *end = NULL;

    if (argc < 3)
    {
        printf("Usage: csr_write csr value\n");
        return -1;
    }

    csr_value = strtoul(argv[2], &end, 0);

    if (!strcmp(argv[1], "mstatus"))
    {
        csr_write(mstatus, csr_value);
    }
    else if (!strcmp(argv[1], "mepc"))
    {
        csr_write(mepc, csr_value);
    }
    else if (!strcmp(argv[1], "mtvec"))
    {
        csr_write(mtvec, csr_value);
    }
    else if (!strcmp(argv[1], "mcause"))
    {
        csr_write(mcause, csr_value);
    }
    else if (!strcmp(argv[1], "mie"))
    {
        csr_write(mie, csr_value);
    }
    else if (!strcmp(argv[1], "mip"))
    {
        csr_write(mip, csr_value);
    }
    else if (!strcmp(argv[1], "mtval"))
    {
        csr_write(mtval, csr_value);
    }
    else if (!strcmp(argv[1], "mscratch"))
    {
        csr_write(mscratch, csr_value);
    }
    else if (!strcmp(argv[1], "sstatus"))
    {
        csr_write(sstatus, csr_value);
    }
    else if (!strcmp(argv[1], "sepc"))
    {
        csr_write(sepc, csr_value);
    }
    else if (!strcmp(argv[1], "stvec"))
    {
        csr_write(stvec, csr_value);
    }
    else if (!strcmp(argv[1], "scause"))
    {
        csr_write(scause, csr_value);
    }
    else if (!strcmp(argv[1], "sie"))
    {
        csr_write(sie, csr_value);
    }
    else if (!strcmp(argv[1], "sip"))
    {
        csr_write(sip, csr_value);
    }
    else if (!strcmp(argv[1], "stval"))
    {
        csr_write(stval, csr_value);
    }
    else if (!strcmp(argv[1], "sscratch"))
    {
        csr_write(sscratch, csr_value);
    }
    else
    {
        printf("can not support the csr %s\n", argv[1]);
        return -1;
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_csr_write, csr_write, write csr);
