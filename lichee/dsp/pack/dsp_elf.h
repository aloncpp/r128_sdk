#ifndef __DSP_ELF_H
#define __DSP_ELF_H

#define EI_NIDENT	16		/* Size of e_ident[] */
/* These typedefs need to be handled better */
typedef uint32_t	Elf32_Addr;	/* Unsigned program address */
typedef uint32_t	Elf32_Off;	/* Unsigned file offset */
typedef int32_t		Elf32_Sword;	/* Signed large integer */
typedef uint32_t	Elf32_Word;	/* Unsigned large integer */
typedef uint16_t	Elf32_Half;	/* Unsigned medium integer */
/* ELF Header */
typedef struct {
	unsigned char	e_ident[EI_NIDENT]; /* ELF Identification */
	Elf32_Half	e_type;		/* object file type */
	Elf32_Half	e_machine;	/* machine */
	Elf32_Word	e_version;	/* object file version */
	Elf32_Addr	e_entry;	/* virtual entry point */
	Elf32_Off	e_phoff;	/* program header table offset */
	Elf32_Off	e_shoff;	/* section header table offset */
	Elf32_Word	e_flags;	/* processor-specific flags */
	Elf32_Half	e_ehsize;	/* ELF header size */
	Elf32_Half	e_phentsize;	/* program header entry size */
	Elf32_Half	e_phnum;	/* number of program header entries */
	Elf32_Half	e_shentsize;	/* section header entry size */
	Elf32_Half	e_shnum;	/* number of section header entries */
	Elf32_Half	e_shstrndx;	/* section header table's "section
					   header string table" entry offset */
} Elf32_Ehdr;

/* Program Header */
typedef struct {
	Elf32_Word	p_type;		/* segment type */
	Elf32_Off	p_offset;	/* segment offset */
	Elf32_Addr	p_vaddr;	/* virtual address of segment */
	Elf32_Addr	p_paddr;	/* physical address of segment */
	Elf32_Word	p_filesz;	/* number of bytes in file for seg */
	Elf32_Word	p_memsz;	/* number of bytes in mem. for seg */
	Elf32_Word	p_flags;	/* flags */
	Elf32_Word	p_align;	/* memory alignment */
} Elf32_Phdr;

/* Section Header */
typedef struct {
	Elf32_Word	sh_name;	/* name - index into section header
					   string table section */
	Elf32_Word	sh_type;	/* type */
	Elf32_Word	sh_flags;	/* flags */
	Elf32_Addr	sh_addr;	/* address */
	Elf32_Off	sh_offset;	/* file offset */
	Elf32_Word	sh_size;	/* section size */
	Elf32_Word	sh_link;	/* section header table index link */
	Elf32_Word	sh_info;	/* extra information */
	Elf32_Word	sh_addralign;	/* address alignment */
	Elf32_Word	sh_entsize;	/* section entry size */
} Elf32_Shdr;



/* Section Attribute Flags - sh_flags */
#define SHF_WRITE	0x1		/* Writable */
#define SHF_ALLOC	0x2		/* occupies memory */
#define SHF_EXECINSTR	0x4		/* executable */
#define SHF_MERGE	0x10		/* Might be merged */
#define SHF_STRINGS	0x20		/* Contains NULL terminated strings */
#define SHF_INFO_LINK	0x40		/* sh_info contains SHT index */
#define SHF_LINK_ORDER	0x80		/* Preserve order after combining*/
#define SHF_OS_NONCONFORMING 0x100	/* Non-standard OS specific handling */
#define SHF_GROUP	0x200		/* Member of section group */
#define SHF_TLS		0x400		/* Thread local storage */
#define SHF_MASKOS	0x0ff00000	/* OS specific */
#define SHF_MASKPROC	0xf0000000	/* reserved bits for processor */

/* sh_type */
#define SHT_NULL	0		/* inactive */
#define SHT_PROGBITS	1		/* program defined information */
#define SHT_SYMTAB	2		/* symbol table section */
#define SHT_STRTAB	3		/* string table section */
#define SHT_RELA	4		/* relocation section with addends*/
#define SHT_HASH	5		/* symbol hash table section */
#define SHT_DYNAMIC	6		/* dynamic section */
#define SHT_NOTE	7		/* note section */
#define SHT_NOBITS	8		/* no space section */
#define SHT_REL		9		/* relation section without addends */
#define SHT_SHLIB	10		/* reserved - purpose unknown */
#define SHT_DYNSYM	11		/* dynamic symbol table section */
#define SHT_INIT_ARRAY	14		/* Array of constructors */
#define SHT_FINI_ARRAY	15		/* Array of destructors */
#define SHT_PREINIT_ARRAY 16		/* Array of pre-constructors */
#define SHT_GROUP	17		/* Section group */
#define SHT_SYMTAB_SHNDX 18		/* Extended section indeces */
#define SHT_NUM		19		/* number of section types */
#define SHT_LOOS	0x60000000	/* Start OS-specific */
#define SHT_HIOS	0x6fffffff	/* End OS-specific */
#define SHT_LOPROC	0x70000000	/* reserved range for processor */
#define SHT_HIPROC	0x7fffffff	/* specific section header types */
#define SHT_LOUSER	0x80000000	/* reserved range for application */
#define SHT_HIUSER	0xffffffff	/* specific indexes */

#endif /* __SUNXI_PLATFORM_H */
