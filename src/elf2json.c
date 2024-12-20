/*

  elf2json.c


  Copyright (C) 2024  olikraus@gmail.com

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <libelf.h>
#include <gelf.h>
#include <string.h>

struct _elf_translate_struct
{
  size_t n;                     // number
  char *m;        // macro
  char *d;        // description
};
typedef struct _elf_translate_struct elf_translate_struct;


#define ET(c,d)  { c, #c, d }
#define ETNONE() { 0, NULL, NULL }

elf_translate_struct et_elf_class[] = {
  ET(ELFCLASSNONE, "Invalid class"),
  ET(ELFCLASS32, "32-bit objects"),
  ET(ELFCLASS64, "64-bit objects"),
  ETNONE()
};

elf_translate_struct et_elf_osabi[] = {
  ET( ELFOSABI_SYSV, "UNIX System V ABI "),
  ET( ELFOSABI_HPUX, "HP-UX "),
  ET( ELFOSABI_NETBSD, "NetBSD.  "),
  ET( ELFOSABI_GNU, "Object uses GNU ELF extensions.  "),
  ET( ELFOSABI_SOLARIS, "Sun Solaris.  "),
  ET( ELFOSABI_AIX, "IBM AIX.  "),
  ET( ELFOSABI_IRIX, "SGI Irix.  "),
  ET( ELFOSABI_FREEBSD, "FreeBSD.  "),
  ET( ELFOSABI_TRU64, "Compaq TRU64 UNIX.  "),
  ET( ELFOSABI_MODESTO, "Novell Modesto.  "),
  ET( ELFOSABI_OPENBSD, "OpenBSD.  "),
  ET( ELFOSABI_ARM_AEABI, "ARM EABI "),
  ET( ELFOSABI_ARM, "ARM "),
  ET( ELFOSABI_STANDALONE, "Standalone (embedded) application "),
  ETNONE()
};

elf_translate_struct et_e_type[] = {
  ET( ET_NONE, "No file type "),
  ET( ET_REL, "Relocatable file "),
  ET( ET_EXEC, "Executable file "),
  ET( ET_DYN, "Shared object file "),
  ET( ET_CORE, "Core file "),
  ETNONE()
};


elf_translate_struct et_e_machine[] = {
  ET( EM_NONE	, "No machine"),
  ET( EM_M32  , "AT&T WE 32100"),
  ET( EM_SPARC	 , "SUN SPARC"),
  ET( EM_386     , "Intel 80386"),
  ET( EM_68K	, "Motorola m68k family"),
  ET( EM_88K	, "Motorola m88k family"),
  ET( EM_IAMCU	 , "Intel MCU"),
  ET( EM_860	, "Intel 80860"),
  ET( EM_MIPS	, "MIPS R3000 big-endian"),
  ET( EM_S370	, "IBM System/370"),
  ET( EM_MIPS_RS3_LE, "MIPS R3000 little-endian"),
  ET( EM_PARISC, "HPPA"),
  ET( EM_VPP500, "Fujitsu VPP500"),
  ET( EM_SPARC32PLUS, "Sun's 'v8plus'"),
  ET( EM_960	, "Intel 80960"),
  ET( EM_PPC	, "PowerPC"),
  ET( EM_PPC64, "PowerPC 64-bit"),
  ET( EM_S390	, "IBM S390"),
  ET( EM_SPU	, "IBM SPU/SPC"),
  ET( EM_V800	, "NEC V800 series"),
  ET( EM_FR20	, "Fujitsu FR20"),
  ET( EM_RH32	, "TRW RH-32"),
  ET( EM_RCE	, "Motorola RCE"),
  ET( EM_ARM	, "ARM"),
  ET( EM_FAKE_ALPHA, "Digital Alpha"),
  ET( EM_SH	, "Hitachi SH"),
  ET( EM_SPARCV9, "SPARC v9 64-bit"),
  ET( EM_TRICORE, "Siemens Tricore"),
  ET( EM_ARC	, "Argonaut RISC Core"),
  ET( EM_H8_300, "Hitachi H8/300"),
  ET( EM_H8_300H, "Hitachi H8/300H"),
  ET( EM_H8S	, "Hitachi H8S"),
  ET( EM_H8_500, "Hitachi H8/500"),
  ET( EM_IA_64, "Intel Merced"),
  ET( EM_MIPS_X, "Stanford MIPS-X"),
  ET( EM_COLDFIRE, "Motorola Coldfire"),
  ET( EM_68HC12, "Motorola M68HC12"),
  ET( EM_MMA	, "Fujitsu MMA Multimedia Accelerator"),
  ET( EM_PCP	, "Siemens PCP"),
  ET( EM_NCPU	, "Sony nCPU embedded RISC"),
  ET( EM_NDR1	, "Denso NDR1 microprocessor"),
  ET( EM_STARCORE, "Motorola Start*Core processor"),
  ET( EM_ME16	, "Toyota ME16 processor"),
  ET( EM_ST100, "STMicroelectronic ST100 processor"),
  ET( EM_TINYJ, "Advanced Logic Corp. Tinyj emb.fam"),
  ET( EM_X86_64, "AMD x86-64 architecture"),
  ET( EM_PDSP	, "Sony DSP Processor"),
  ET( EM_PDP10, "Digital PDP-10"),
  ET( EM_PDP11, "Digital PDP-11"),
  ET( EM_FX66	, "Siemens FX66 microcontroller"),
  ET( EM_ST9PLUS, "STMicroelectronics ST9+ 8/16 mc"),
  ET( EM_ST7	, "STmicroelectronics ST7 8 bit mc"),
  ET( EM_68HC16, "Motorola MC68HC16 microcontroller"),
  ET( EM_68HC11, "Motorola MC68HC11 microcontroller"),
  ET( EM_68HC08, "Motorola MC68HC08 microcontroller"),
  ET( EM_68HC05, "Motorola MC68HC05 microcontroller"),
  ET( EM_SVX	, "Silicon Graphics SVx"),
  ET( EM_ST19	, "STMicroelectronics ST19 8 bit mc"),
  ET( EM_VAX	, "Digital VAX"),
  ET( EM_CRIS	, "Axis Communications 32-bit emb.proc"),
  ET( EM_JAVELIN, "Infineon Technologies 32-bit emb.proc"),
  ET( EM_FIREPATH, "Element 14 64-bit DSP Processor"),
  ET( EM_ZSP	, "LSI Logic 16-bit DSP Processor"),
  ET( EM_MMIX	, "Donald Knuth's educational 64-bit proc"),
  ET( EM_HUANY, "Harvard University machine-independent object files"),
  ET( EM_PRISM, "SiTera Prism"),
  ET( EM_AVR	, "Atmel AVR 8-bit microcontroller"),
  ET( EM_FR30	, "Fujitsu FR30"),
  ET( EM_D10V	, "Mitsubishi D10V"),
  ET( EM_D30V	, "Mitsubishi D30V"),
  ET( EM_V850	, "NEC v850"),
  ET( EM_M32R	, "Mitsubishi M32R"),
  ET( EM_MN10300, "Matsushita MN10300"),
  ET( EM_MN10200, "Matsushita MN10200"),
  ET( EM_PJ	, "picoJava"),
  ET( EM_OPENRISC, "OpenRISC 32-bit embedded processor"),
  ET( EM_ARC_COMPACT, "ARC International ARCompact"),
  ET( EM_XTENSA, "Tensilica Xtensa Architecture"),
  ET( EM_VIDEOCORE, "Alphamosaic VideoCore"),
  ET( EM_TMM_GPP, "Thompson Multimedia General Purpose Proc"),
  ET( EM_NS32K, "National Semi. 32000"),
  ET( EM_TPC	, "Tenor Network TPC"),
  ET( EM_SNP1K, "Trebia SNP 1000"),
  ET( EM_ST200, "STMicroelectronics ST200"),
  ET( EM_IP2K	, "Ubicom IP2xxx"),
  ET( EM_MAX	, "MAX processor"),
  ET( EM_CR	, "National Semi. CompactRISC"),
  ET( EM_F2MC16, "Fujitsu F2MC16"),
  ET( EM_MSP430, "Texas Instruments msp430"),
  ET( EM_BLACKFIN, "Analog Devices Blackfin DSP"),
  ET( EM_SE_C33, "Seiko Epson S1C33 family"),
  ET( EM_SEP	, "Sharp embedded microprocessor"),
  ET( EM_ARCA	, "Arca RISC"),
  ET( EM_UNICORE, "PKU-Unity & MPRC Peking Uni. mc series"),
  ET( EM_EXCESS, "eXcess configurable cpu"),
  ET( EM_DXP	, "Icera Semi. Deep Execution Processor"),
  ET( EM_ALTERA_NIOS2, "Altera Nios II"),
  ET( EM_CRX	, "National Semi. CompactRISC CRX"),
  ET( EM_XGATE, "Motorola XGATE"),
  ET( EM_C166	, "Infineon C16x/XC16x"),
  ET( EM_M16C	, "Renesas M16C"),
  ET( EM_DSPIC30F, "Microchip Technology dsPIC30F"),
  ET( EM_CE	, "Freescale Communication Engine RISC"),
  ET( EM_M32C	, "Renesas M32C"),
  ET( EM_TSK3000, "Altium TSK3000"),
  ET( EM_RS08	, "Freescale RS08"),
  ET( EM_SHARC, "Analog Devices SHARC family"),
  ET( EM_ECOG2, "Cyan Technology eCOG2"),
  ET( EM_SCORE7, "Sunplus S+core7 RISC"),
  ET( EM_DSP24, "New Japan Radio (NJR) 24-bit DSP"),
  ET( EM_VIDEOCORE3, "Broadcom VideoCore III"),
  ET( EM_LATTICEMICO32, "RISC for Lattice FPGA"),
  ET( EM_SE_C17, "Seiko Epson C17"),
  ET( EM_TI_C6000, "Texas Instruments TMS320C6000 DSP"),
  ET( EM_TI_C2000, "Texas Instruments TMS320C2000 DSP"),
  ET( EM_TI_C5500, "Texas Instruments TMS320C55x DSP"),
  ET( EM_TI_ARP32, "Texas Instruments App. Specific RISC"),
  ET( EM_TI_PRU, "Texas Instruments Prog. Realtime Unit"),
  ET( EM_MMDSP_PLUS, "STMicroelectronics 64bit VLIW DSP"),
  ET( EM_CYPRESS_M8C, "Cypress M8C"),
  ET( EM_R32C	, "Renesas R32C"),
  ET( EM_TRIMEDIA, "NXP Semi. TriMedia"),
  ET( EM_QDSP6, "QUALCOMM DSP6"),
  ET( EM_8051	, "Intel 8051 and variants"),
  ET( EM_STXP7X, "STMicroelectronics STxP7x"),
  ET( EM_NDS32, "Andes Tech. compact code emb. RISC"),
  ET( EM_ECOG1X, "Cyan Technology eCOG1X"),
  ET( EM_MAXQ30, "Dallas Semi. MAXQ30 mc"),
  ET( EM_XIMO16, "New Japan Radio (NJR) 16-bit DSP"),
  ET( EM_MANIK, "M2000 Reconfigurable RISC"),
  ET( EM_CRAYNV2, "Cray NV2 vector architecture"),
  ET( EM_RX	, "Renesas RX"),
  ET( EM_METAG, "Imagination Tech. META"),
  ET( EM_MCST_ELBRUS, "MCST Elbrus"),
  ET( EM_ECOG16, "Cyan Technology eCOG16"),
  ET( EM_CR16	, "National Semi. CompactRISC CR16"),
  ET( EM_ETPU	, "Freescale Extended Time Processing Unit"),
  ET( EM_SLE9X, "Infineon Tech. SLE9X"),
  ET( EM_L10M	, "Intel L10M"),
  ET( EM_K10M	, "Intel K10M"),
  ET( EM_AARCH64, "ARM AARCH64"),
  ET( EM_AVR32, "Amtel 32-bit microprocessor"),
  ET( EM_STM8	, "STMicroelectronics STM8"),
  ET( EM_TILE64, "Tilera TILE64"),
  ET( EM_TILEPRO, "Tilera TILEPro"),
  ET( EM_MICROBLAZE, "Xilinx MicroBlaze"),
  ET( EM_CUDA	, "NVIDIA CUDA"),
  ET( EM_TILEGX, "Tilera TILE-Gx"),
  ET( EM_CLOUDSHIELD, "CloudShield"),
  ET( EM_COREA_1ST, "KIPO-KAIST Core-A 1st gen."),
  ET( EM_COREA_2ND, "KIPO-KAIST Core-A 2nd gen."),
  ET( EM_ARCV2, "Synopsys ARCv2 ISA. "),
  ET( EM_OPEN8, "Open8 RISC"),
  ET( EM_RL78	, "Renesas RL78"),
  ET( EM_VIDEOCORE5, "Broadcom VideoCore V"),
  ET( EM_78KOR, "Renesas 78KOR"),
  ET( EM_56800EX, "Freescale 56800EX DSC"),
  ET( EM_BA1	, "Beyond BA1"),
  ET( EM_BA2	, "Beyond BA2"),
  ET( EM_XCORE, "XMOS xCORE"),
  ET( EM_MCHP_PIC, "Microchip 8-bit PIC(r)"),
  ET( EM_INTELGT, "Intel Graphics Technology"),
  ET( EM_KM32	, "KM211 KM32"),
  ET( EM_KMX32, "KM211 KMX32"),
  ET( EM_EMX16, "KM211 KMX16"),
  ET( EM_EMX8	, "KM211 KMX8"),
  ET( EM_KVARC, "KM211 KVARC"),
  ET( EM_CDP	, "Paneve CDP"),
  ET( EM_COGE	, "Cognitive Smart Memory Processor"),
  ET( EM_COOL	, "Bluechip CoolEngine"),
  ET( EM_NORC	, "Nanoradio Optimized RISC"),
  ET( EM_CSR_KALIMBA, "CSR Kalimba"),
  ET( EM_Z80	, "Zilog Z80"),
  ET( EM_VISIUM, "Controls and Data Services VISIUMcore"),
  ET( EM_FT32	, "FTDI Chip FT32"),
  ET( EM_MOXIE, "Moxie processor"),
  ET( EM_AMDGPU, "AMD GPU"),
  ET( EM_RISCV, "RISC-V"),
  ET( EM_BPF	, "Linux BPF -- in-kernel virtual machine"),
  ET( EM_CSKY	    , "C-SKY"),
  ET( EM_LOONGARCH, "LoongArch"),
  ETNONE()
};

elf_translate_struct et_phdr_type[] = {
 ET(PT_NULL, "Program header table entry unused"),
 ET(PT_LOAD, "Loadable program segment"),
 ET(PT_DYNAMIC, "Dynamic linking information"),
 ET(PT_INTERP, "Program interpreter"),
 ET(PT_NOTE, "Auxiliary information"),
 ET(PT_SHLIB, "Reserved"),
 ET(PT_PHDR, "Entry for header table itself"),
 ET(PT_TLS, "Thread-local storage segment"),
 ETNONE()
};

elf_translate_struct et_phdr_flags[] = {
 ET(PF_X, "Segment is executable"),
 ET(PF_W, "Segment is writable"),
 ET(PF_R, "Segment is readable"),
 ETNONE()
};


elf_translate_struct et_sh_type[] = {
  ET(SHT_NULL, "Section header table entry unused"),
  ET(SHT_PROGBITS	, "Program data"),
  ET(SHT_SYMTAB, "Symbol table"),
  ET(SHT_STRTAB, "String table"),
  ET(SHT_RELA	, "Relocation entries with addends"),
  ET(SHT_HASH	 , "Symbol hash table"),
  ET(SHT_DYNAMIC, "Dynamic linking information"),
  ET(SHT_NOTE	, "Notes"),
  ET(SHT_NOBITS, "Program space with no data (bss)"),
  ET(SHT_REL, "Relocation entries, no addends"),
  ET(SHT_SHLIB	, "Reserved"),
  ET(SHT_DYNSYM, "Dynamic linker symbol table"),
  ET(SHT_INIT_ARRAY	 , "Array of constructors"),
  ET(SHT_FINI_ARRAY	 , "Array of destructors"),
  ET(SHT_PREINIT_ARRAY, "Array of pre-constructors"),
  ET(SHT_GROUP, "Section group"),
  ET(SHT_SYMTAB_SHNDX , "Extended section indices"),
  ET(SHT_RELR, "RELR relative relocations"),
  ETNONE()
};


elf_translate_struct et_sh_flags[] = {
  ET(SHF_WRITE, "Writable"),
  ET(SHF_ALLOC, "Occupies memory during execution"),
  ET(SHF_EXECINSTR, "Executable"),
  ET(SHF_MERGE, "Might be merged"),
  ET(SHF_STRINGS, "Contains nul-terminated strings"),
  ET(SHF_INFO_LINK, "`sh_info' contains SHT index"),
  ET(SHF_LINK_ORDER, "Preserve order after combining"),
  ET(SHF_OS_NONCONFORMING, "Non-standard OS specific handling required"),
  ET(SHF_GROUP, "Section is member of a group. "),
  ET(SHF_TLS, "Section hold thread-local data. "),
  ET(SHF_COMPRESSED, "Section with compressed data."),
  ETNONE()
};

/* GELF_ST_TYPE(st_bind) */
elf_translate_struct et_st_bind[] = {
  ET(STB_LOCAL, "Local symbol"),
  ET(STB_GLOBAL, "Global symbol"),
  ET(STB_WEAK, "Weak symbol"),
  ET(STB_NUM, "Number of defined types."),
  ET(STB_GNU_UNIQUE, "Unique symbol."),
  ETNONE()
};

/* GELF_ST_TYPE(st_info) */
elf_translate_struct et_st_type[] = {
  ET(STT_NOTYPE, "Symbol type is unspecified"),
  ET(STT_OBJECT, "Symbol is a data object"),
  ET(STT_FUNC, "Symbol is a code object"),
  ET(STT_SECTION, "Symbol associated with a section"),
  ET(STT_FILE, "Symbol's name is file name"),
  ET(STT_COMMON, "Symbol is a common data object"),
  ET(STT_TLS, "Symbol is thread-local data object"),
  ET(STT_NUM, "Number of defined types. "),
  ET(STT_GNU_IFUNC, "Symbol is indirect code object"),
  ETNONE()
};

/* GELF_ST_VISIBILITY(st_other) */
elf_translate_struct et_st_visibility[] = {
  ET(STV_DEFAULT, "Default symbol visibility rules"),
  ET(STV_INTERNAL, "Processor specific hidden class"),
  ET(STV_HIDDEN, "Sym unavailable in other modules"),
  ET(STV_PROTECTED, "Not preemptible, not exported"),
  ETNONE()
};
  
/* d_type member of the Elf_Data struct (libelf.h) */
elf_translate_struct et_d_type[] = {
  ET(ELF_T_BYTE,                   "unsigned char"),
  ET(ELF_T_ADDR,                   "Elf32_Addr, Elf64_Addr, ..."),
  ET(ELF_T_DYN,                    "Dynamic section record."),
  ET(ELF_T_EHDR,                   "ELF header."),
  ET(ELF_T_HALF,                   "Elf32_Half, Elf64_Half, ..."),
  ET(ELF_T_OFF,                    "Elf32_Off, Elf64_Off, ..."),
  ET(ELF_T_PHDR,                   "Program header."),
  ET(ELF_T_RELA,                   "Relocation entry with addend."),
  ET(ELF_T_REL,                    "Relocation entry."),
  ET(ELF_T_SHDR,                   "Section header."),
  ET(ELF_T_SWORD,                  "Elf32_Sword, Elf64_Sword, ..."),
  ET(ELF_T_SYM,                    "Symbol record."),
  ET(ELF_T_WORD,                   "Elf32_Word, Elf64_Word, ..."),
  ET(ELF_T_XWORD,                  "Elf32_Xword, Elf64_Xword, ..."),
  ET(ELF_T_SXWORD,                 "Elf32_Sxword, Elf64_Sxword, ..."),
  ET(ELF_T_VDEF,                   "Elf32_Verdef, Elf64_Verdef, ..."),
  ET(ELF_T_VDAUX,                  "Elf32_Verdaux, Elf64_Verdaux, ..."),
  ET(ELF_T_VNEED,                  "Elf32_Verneed, Elf64_Verneed, ..."),
  ET(ELF_T_VNAUX,                  "Elf32_Vernaux, Elf64_Vernaux, ..."),
  ET(ELF_T_NHDR,                   "Elf32_Nhdr, Elf64_Nhdr, ..."),
  ET(ELF_T_SYMINFO,		"Elf32_Syminfo, Elf64_Syminfo, ..."),
  ET(ELF_T_MOVE,			"Elf32_Move, Elf64_Move, ..."),
  ET(ELF_T_LIB,			"Elf32_Lib, Elf64_Lib, ..."),
  ET(ELF_T_GNUHASH,		"GNU-style hash section. "),
  ET(ELF_T_AUXV,			"Elf32_auxv_t, Elf64_auxv_t, ..."),
  ET(ELF_T_CHDR,			"Compressed, Elf32_Chdr, Elf64_Chdr, ..."),
  ET(ELF_T_NHDR8,			"Special GNU Properties note.  Same as Nhdr, except padding."),
  ET(ELF_T_RELR,			"Relative relocation entry."),
  ETNONE()
};

/* d_tag member of Elf64_Dyn */
elf_translate_struct et_d_tag[] = {
  ET(  DT_NULL		        , "Marks end of dynamic section"),
  ET(  DT_NEEDED	        , "Name of needed library"),      // string table offset
  ET(  DT_PLTRELSZ	        , "Size in bytes of PLT relocs"),
  ET(  DT_PLTGOT	        , "Processor defined value"),
  ET(  DT_HASH	        , "Address of symbol hash table"),
  ET(  DT_STRTAB	        , "Address of string table"),
  ET(  DT_SYMTAB	        , "Address of symbol table"),
  ET(  DT_RELA		        , "Address of Rela relocs"),
  ET(  DT_RELASZ	        , "Total size of Rela relocs"),
  ET(  DT_RELAENT	        , "Size of one Rela reloc"),
  ET(  DT_STRSZ	        , "Size of string table"),
  ET(  DT_SYMENT	        , "Size of one symbol table entry"),
  ET(  DT_INIT		        , "Address of init function"),
  ET(  DT_FINI		        , "Address of termination function"),
  ET(  DT_SONAME	        , "Name of shared object"),
  ET(  DT_RPATH	        , "Library search path (deprecated)"),
  ET(  DT_SYMBOLIC	        , "Start symbol search here"),
  ET(  DT_REL		        , "Address of Rel relocs"),
  ET(  DT_RELSZ	        , "Total size of Rel relocs"),
  ET(  DT_RELENT	        , "Size of one Rel reloc"),
  ET(  DT_PLTREL	        , "Type of reloc in PLT"),
  ET(  DT_DEBUG	        , "For debugging; unspecified"),
  ET(  DT_TEXTREL	        , "Reloc might modify .text"),
  ET(  DT_JMPREL	        , "Address of PLT relocs"),
  ET(  DT_BIND_NOW	, "Process relocations of object"),
  ET(  DT_INIT_ARRAY	, "Array with addresses of init fct"),
  ET(  DT_FINI_ARRAY	, "Array with addresses of fini fct"),
  ET(  DT_INIT_ARRAYSZ	, "Size in bytes of DT_INIT_ARRAY"),
  ET(  DT_FINI_ARRAYSZ	, "Size in bytes of DT_FINI_ARRAY"),
  ET(  DT_RUNPATH	        , "Library search path"),
  ET(  DT_FLAGS	        , "Flags for the object being loaded"),
  ET(  DT_ENCODING	, "Start of encoded range"),
  ET(  DT_PREINIT_ARRAY , "Array with addresses of preinit fct"),
  ET(  DT_PREINIT_ARRAYSZ , "size in bytes of DT_PREINIT_ARRAY"),
  ET(  DT_SYMTAB_SHNDX , "Address of SYMTAB_SHNDX section"),
  ET(  DT_RELRSZ	        , "Total size of RELR relative relocations"),
  ET(  DT_RELR		        , "Address of RELR relative relocations"),
  ET(  DT_RELRENT	        , "Size of one RELR relative relocaction"),
  ETNONE()
};


/* read only elf */
struct _relf_struct
{
  int fd;
  Elf *elf;                                     // elf object, returned from elf_begin
  
  size_t section_header_total;               // shdrnum, total number of section headers (each section has a section header, so this is the same as the total number of sectios)
  size_t section_header_string_table_index;  // shdrstrndx, the index of the section where we find the strings for the the section header names;
  size_t program_header_total;               // phdrnum, total number of program headers
  
  GElf_Ehdr elf_file_header;                    // ehdr elf file header
  GElf_Shdr section_header;                     // shdr section header
  
  size_t symtab_section_index;             // section header index of the ".symtab" section, 0 if not found
  size_t strtab_section_index;               // section header index of the ".strtab" section, this contains the strings for the symbols from .symtab, 0 if not found
  size_t dynsym_section_index;          // section header index of the ".dynsym" section, 0 if not found
  size_t dynstr_section_index;          // section header index of the ".dynstr" section, 0 if not found
  
  
};
typedef struct _relf_struct relf_struct;


#if defined(__MINGW32__) || defined(__MINGW64__)
// seems to be missing on mingw
unsigned long __stack_chk_guard = 0xaa55;
void __attribute__ ((noreturn)) __stack_chk_fail (void)
{
	exit(0);
}
#endif

const char *et_get_macro(elf_translate_struct *et, size_t n)
{
  size_t i = 0;
  for(;;)
  {
    if ( et[i].m == NULL )
      break;
    if ( et[i].n == n )
      return et[i].m;
    i++;
  }
  return "";
}

const char *et_get_description(elf_translate_struct *et, size_t n)
{
  size_t i = 0;
  for(;;)
  {
    if ( et[i].m == NULL )
      break;
    if ( et[i].n == n )
      return et[i].d;
    i++;
  }
  return "";
}

// returns NULL if not found
Elf_Scn *relf_find_scn_by_name(relf_struct *relf, const char *name)
{
  Elf_Scn  *scn;        // section descriptor
  GElf_Shdr shdr;
  const char *section_name;
  /* loop over all sections */
  scn = elf_nextscn(relf->elf, NULL);
  while ( scn != NULL ) 
  {
    if ( gelf_getshdr( scn, &shdr ) != &shdr )
        return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), NULL;
    section_name = elf_strptr(relf->elf, relf->section_header_string_table_index, shdr.sh_name );
    if ( section_name == NULL )
      return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), NULL;
    if ( strcmp(section_name, name) == 0 )
      return scn;
    scn = elf_nextscn(relf->elf, scn);
  }
  return NULL; /* section not found */
}

// returns 0 if not found
size_t relf_find_section_index_by_name(relf_struct *relf, const char *name)
{
  Elf_Scn *scn = relf_find_scn_by_name(relf, name);
  if ( scn == NULL )
    return 0;
  return elf_ndxscn( scn ); // returns SHN_UNDEF in case of error, SHN_UNDEF is zero  (elf.h)
}

int relf_init(relf_struct *relf, const char *elf_filename)
{
  memset(relf, 0, sizeof(relf_struct));
    
  if ( elf_version( EV_CURRENT ) == EV_NONE )
    return fprintf(stderr, "Incorrect libelf version: %s\n", elf_errmsg(-1) ), 0;

  relf->fd = open( elf_filename , O_RDONLY , 0);
  if ( relf->fd >= 0 )
  {
    if (( relf->elf = elf_begin( relf->fd , ELF_C_READ, NULL )) != NULL )
    {
      if ( elf_kind( relf->elf ) == ELF_K_ELF )
      {
        if ( gelf_getehdr(relf->elf, &(relf->elf_file_header) ) != NULL )
        {
          if ( elf_getshdrnum(relf->elf, &(relf->section_header_total) ) == 0 )
          {
            if ( elf_getshdrstrndx(relf->elf, &(relf->section_header_string_table_index) ) == 0 )
            {
              if ( elf_getphdrnum(relf->elf, &(relf->program_header_total) ) == 0 )
              {
                /* section index is 0 if not found */
                relf->symtab_section_index = relf_find_section_index_by_name(relf, ".symtab"); 
                relf->strtab_section_index = relf_find_section_index_by_name(relf, ".strtab"); 
                relf->dynsym_section_index = relf_find_section_index_by_name(relf, ".dynsym"); 
                relf->dynstr_section_index = relf_find_section_index_by_name(relf, ".dynstr"); 
                
                return 1;
              }
              else
              {
                fprintf(stderr, "elf_getphdrnum failed: %s\n", elf_errmsg(-1));
              }
            }
            else
            {
              fprintf(stderr, "elf_getshdrstrndx failed: %s\n", elf_errmsg(-1));
            }
          }
          else
          {
            fprintf(stderr, "elf_getshdrnum failed: %s\n", elf_errmsg(-1));
          }
        }
        else
        {
          fprintf(stderr, "Unable to get elf file header: %s\n", elf_errmsg(-1));
        }
      }
      else
      {
        fprintf(stderr, "Not an elf file (found kind %d)\n", elf_kind( relf->elf ));
      }
      elf_end(relf->elf);
    }
    else
    {
      fprintf(stderr, "elf_begin failed: %s\n", elf_errmsg(-1));
    }
    close(relf->fd);
  }
  else
  {
    perror(elf_filename);
  }
  
  memset(relf, 0, sizeof(relf_struct));
  return 0;
}


void relf_destroy(relf_struct *relf)
{
  elf_end(relf->elf);  
  close(relf->fd);  
}



void relf_show_flag_list(elf_translate_struct *et, size_t flags)
{
  int is_first = 1;
  size_t i = 0;
  printf("[");
  for(;;)
  {
    if ( et[i].m == NULL )
      break;
    if ( (et[i].n & flags) != 0 )
    {
      if ( is_first )   
        is_first = 0;
      else
        printf(", ");
      printf( "\"%s\"", et[i].m);
    }
    i++;
  }
  printf("]");
}


void relf_member(const char *s)
{
  printf("\"%s\":", s);
}

/*
  Show the value of a variable
  The value is a number with a C-definition and a description.
  This triple (number, c-def and description) is stored in the 
  elements of the 'elf_translate_struct' array (first arg)
*/
void relf_show_et_value(elf_translate_struct *et, const char *variable, size_t n)
{
  relf_member(variable);
  printf("[%zu, \"%s\", \"%s\"]", n, et_get_macro(et, n), et_get_description(et, n));
}

void relf_show_pure_value(const char *variable, long long unsigned n)
{
  relf_member(variable);  
  printf("[%llu, \"0x%08llx\"]", n, n);
}

void relf_show_flag_value_list(elf_translate_struct *et, const char *variable, long long unsigned n)
{
  relf_member(variable);
  printf("[%llu, ", n);
  relf_show_flag_list(et, n);
  printf("]");
}

void relf_show_string_value(const char *variable, const char *value)
{
  printf("\"%s\": \"%s\"", variable, value);
}


void relf_show_memory(const char *variable, unsigned char *ptr, size_t cnt)
{
  int i;
  relf_member(variable);
  printf("[");
  for( i = 0; i < cnt; i++ )
  {
      if ( i > 0 )
        printf(",");
      printf("%u", (int)ptr[i]);
  }
  printf("]");
}



void relf_indent(int n)
{
  while( n > 0 )
  {
    printf("    ");
    n--;
  }
}

void relf_n()
{
  printf("\n");
}

void relf_c()
{
  printf(",");
}

void relf_cn()
{
  relf_c();
  relf_n();
}

void relf_oo()  // open object
{
  printf("{\n");
}

void relf_co()  // close object
{
  printf("}");
}

void relf_oa()  // open array
{
  printf("[\n");
}

void relf_ca()  // close array
{
  printf("]");
}



void relf_show_elf_header(relf_struct *relf)
{
  int indent = 1;
  char *ident = elf_getident(relf->elf , NULL);
  // EI_OSABI
  // ident[EI_ABIVERSION]
  relf_indent(indent);
  relf_show_et_value(et_elf_class, "EI_CLASS", gelf_getclass( relf->elf ));
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("EI_DATA", ident[EI_DATA]);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("EI_VERSION", ident[EI_VERSION]);
  relf_cn();
  relf_indent(indent);
  relf_show_et_value(et_elf_osabi, "EI_OSABI", ident[EI_OSABI]);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("EI_ABIVERSION", ident[EI_ABIVERSION]);
  relf_cn();
  relf_indent(indent);
  relf_show_et_value(et_e_type, "e_type", relf->elf_file_header.e_type );
  relf_cn();
  relf_indent(indent);
  relf_show_et_value(et_e_machine, "e_machine", relf->elf_file_header.e_machine );
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("e_version", relf->elf_file_header.e_version);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("e_entry", relf->elf_file_header.e_entry);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("e_phoff", relf->elf_file_header.e_phoff);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("e_shoff", relf->elf_file_header.e_shoff);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("e_flags", relf->elf_file_header.e_flags);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("e_ehsize", relf->elf_file_header.e_ehsize);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("e_phentsize", relf->elf_file_header.e_phentsize);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("e_phnum", relf->elf_file_header.e_phnum);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("e_shentsize", relf->elf_file_header.e_shentsize);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("e_shnum", relf->elf_file_header.e_shnum);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("e_shstrndx", relf->elf_file_header.e_shstrndx);

  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("program_header_total", relf->program_header_total);

  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("symtab_section_index", relf->symtab_section_index);

  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("strtab_section_index", relf->strtab_section_index);
  
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("dynsym_section_index", relf->dynsym_section_index);

  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("dynstr_section_index", relf->dynstr_section_index);
}

/*
  phdr member
  Elf64_Word	p_type;		Segment type
  Elf64_Word	p_flags;		Segment flags
  Elf64_Off	p_offset;		Segment file offset
  Elf64_Addr	p_vaddr;		Segment virtual address
  Elf64_Addr	p_paddr;		Segment physical address
  Elf64_Xword	p_filesz;		Segment size in file
  Elf64_Xword	p_memsz;	Segment size in memory
  Elf64_Xword	p_align;		Segment alignment
*/

int relf_show_program_header(relf_struct *relf, GElf_Phdr *phdr)
{
  // et_phdr_type
// et_phdr_flags
  int indent = 3;
  
  relf_indent(indent-1);
  relf_oo();
  
  //relf_indent(indent);
  //relf_show_pure_value("section_index", (long long unsigned)elf_ndxscn(scn));   // get the "official" section index
  //relf_cn();    
  
  //relf_indent(indent);
  //relf_show_string_value("sh_name", section_name);
  //relf_cn();  
  
  relf_indent(indent);
  relf_show_et_value(et_phdr_type, "p_type", phdr->p_type);
  relf_cn();
  relf_indent(indent);
  relf_show_flag_value_list(et_phdr_flags, "p_flags", phdr->p_flags);
  relf_cn();

  relf_indent(indent);
  relf_show_pure_value("p_offset", phdr->p_offset);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("p_vaddr", phdr->p_vaddr);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("p_paddr", phdr->p_paddr);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("p_filesz", phdr->p_filesz);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("p_memsz", phdr->p_memsz);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("p_align", phdr->p_align);
  relf_n();
  
  relf_indent(indent-1);
  relf_co();    // close object
  //printf("Section %04lu %-18s type=%10lu size=%5ld entsize=%5ld\n", (unsigned long)elf_ndxscn(scn), section_name, (unsigned long)shdr.sh_type, (unsigned long)shdr.sh_size, (unsigned long)shdr.sh_entsize);
  return 1;


  
}

int relf_show_program_header_list(relf_struct *relf)
{
  int indent = 1;
  int i;
  GElf_Phdr phdr;
  int is_first = 1;
  
  /* loop over all program headers */
  relf_indent(indent);
  relf_member("program_header_list");
  relf_n();
  relf_indent(indent);
  relf_oa();            // open array
  for( i = 0; i < relf->program_header_total; i++ )
  {
    if ( gelf_getphdr(relf->elf, i, &phdr) == NULL )
      break;
    
    if ( is_first )
      is_first = 0;
    else
      relf_cn();                // comma + new line
      
    if ( relf_show_program_header(relf, &phdr) == 0 )
    {
      relf_ca();
      relf_n();
      return 0;
    }
  }
  relf_n();
  relf_indent(indent);
  relf_ca();    // close array
  return 1;
}

/* returns a pointer to a memory location within a section */
void *relf_get_mem_ptr(relf_struct *relf, size_t section_index, size_t addr)
{
  GElf_Shdr shdr;
  Elf_Scn *scn;
  Elf_Data *data = NULL;
  size_t block_addr = 0;
  
  if ( section_index == 0 || section_index > 0x0fff0 )
    return NULL;
  
  scn = elf_getscn (relf->elf,  section_index);  
  if ( scn == NULL )
    return fprintf(stderr, "libelf: %s, section_index=%ld \n", elf_errmsg(-1), section_index), NULL;
  if ( gelf_getshdr( scn, &shdr ) != &shdr )
    return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), NULL;

  /*
    shdr.sh_addr                contains the base address for the target memory
    shdr.sh_size                 contains the size of the section
  */
  
  if ( shdr.sh_size == 0 )
    return NULL;
  
  for(;;)
  {
    data = elf_getdata(scn , data);     // if data==NULL return first data, otherwise return next data
    if ( data == NULL )
      break;
    if ( data->d_buf == NULL )
      break;
    block_addr = shdr.sh_addr + data->d_off;    // calculate the address of this data in the target system, not 100% sure whether this is correct
    //printf("block_addr=%08lx addr=%08lx d_buf=%p\n", block_addr, addr, data->d_buf);
    if ( addr >= block_addr && addr < block_addr+data->d_size )  // check if the requested addr is inside the current block
    {
      return data->d_buf + addr - block_addr;   // found
    }
  }  
  return NULL;
}

int relf_show_symbol_data(relf_struct *relf, Elf_Scn  *scn, Elf_Data *data, int corresponding_section_string_table_index)
{
  int i = 0;
  
  /*
    GElf_Sym contains the following members:  
      st_name;		Symbol name (string tbl index) 
      st_value;		Symbol value 
      st_size;		Symbol size
      st_info;		Symbol type and binding         GELF_ST_BIND(st_info), GELF_ST_TYPE(st_info)
      st_other;		Symbol visibility               GELF_ST_VISIBILITY(st_other)
      st_shndx;		Section index
                                    https://refspecs.linuxbase.org/elf/gabi4+/ch4.symtab.html
                                    Every symbol table entry is defined in relation to some section.
                                    This member holds the relevant section header table index. 
                                    As the sh_link and sh_info interpretation table and the related text describe, some section indexes indicate special meanings.
                                    If this member contains SHN_XINDEX, then the actual section header index is too large to fit in this field. 
                                    The actual value is contained in the associated section of type SHT_SYMTAB_SHNDX.   
  
                                    The interpretation of the st_value field depends on the st_shndx value:
                                    SHN_UNDEF	        0x0000          Undefined section 
                                        This section table index means the symbol is undefined. 
                                        When the link editor combines this object file with another that defines the indicated symbol, this file's references to the symbol will be linked to the actual definition. 
                                    SHN_ABS		0xfff1	        Associated symbol is absolute
                                        The symbol has an absolute value that will not change because of relocation. 
                                    SHN_COMMON	0xfff2		Associated symbol is common
                                        The symbol labels a common block that has not yet been allocated. 
                                        The symbol's value gives alignment constraints, similar to a section's sh_addralign member. 
                                        The link editor will allocate the storage for the symbol at an address that is a multiple of st_value. 
                                        The symbol's size tells how many bytes are required. Symbols with section index SHN_COMMON may appear only in relocatable objects. 
                                    SHN_XINDEX	        0xffff		Index is in extra table.
                                        This value is an escape value. 
                                        It indicates that the symbol refers to a specific location within a section, 
                                        but that the section header index for that section is too large to be represented directly in the symbol table entry. 
                                        The actual section header index is found in the associated SHT_SYMTAB_SHNDX section. 
                                        The entries in that section correspond one to one with the entries in the symbol table. 
                                        Only those entries in SHT_SYMTAB_SHNDX that correspond to symbol table entries with SHN_XINDEX will hold valid section header indexes; all other entries will have value 0. 

  
  */
  GElf_Sym symbol;
  
  int indent = 6;
  int is_first = 1;
  const char *symbol_name;


  relf_cn();
  relf_indent(indent-1);
  relf_member("symbol_list");
  relf_n();
  relf_indent(indent-1);
  relf_oa();    // open array

  
  /*
    There is no function to get the total number of symbols, so start with index zero and loop
    until gelf_getsym() returns an error,
  
    Note: It seems that then total number is equal to the data block sized / sizeof(symbol struct), 
      but sizeof(symbol struct) depends on the type of the elf file (32 or 64 bit).
      
    if st_shndx has the value SHN_XINDEX then
     GElf_Sym *gelf_getsymshndx(Elf_Data *symdata, Elf_Data *xndxdata, int ndx, GElf_Sym *sym, Elf32_Word *xndxptr);
    will calculate the correct section index in xndxptr. xndxdata must be a section of type SHT_SYMTAB_SHNDX
  */
  while( gelf_getsym(data, i, &symbol) != NULL )
  {
    if ( is_first ) 
      is_first = 0;
    else
      relf_cn();

    symbol_name = elf_strptr(relf->elf, corresponding_section_string_table_index, symbol.st_name );
    if ( symbol_name == NULL )
      return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;

    
    relf_indent(indent);
    relf_oo();

    relf_indent(indent+1);
    relf_show_string_value("st_name", symbol_name);
    relf_cn();    

    relf_indent(indent+1);
    relf_show_pure_value("st_value", symbol.st_value);
    relf_cn();    
    relf_indent(indent+1);
    relf_show_pure_value("st_size", symbol.st_size);
    relf_cn();    
    relf_indent(indent+1);
    relf_show_pure_value("st_shndx", symbol.st_shndx);
    relf_cn();    

    relf_indent(indent+1);
    relf_show_pure_value("st_info", symbol.st_info);
    relf_cn();    
    relf_indent(indent+1);
    relf_show_et_value(et_st_bind, "ST_BIND", GELF_ST_BIND(symbol.st_info));
    relf_cn();
    relf_indent(indent+1);
    relf_show_et_value(et_st_type, "ST_TYPE", GELF_ST_TYPE(symbol.st_info));
    relf_cn();
    relf_indent(indent+1);
    relf_show_pure_value("st_other", symbol.st_other);
    relf_cn();    
    relf_indent(indent+1);
    relf_show_et_value(et_st_visibility, "ST_VISIBILITY", GELF_ST_VISIBILITY(symbol.st_other));
    //relf_cn();
    
    if ( symbol.st_shndx > 0 )
    {
      unsigned char * ptr = (unsigned char *)relf_get_mem_ptr(relf, symbol.st_shndx, symbol.st_value);
      if ( ptr != NULL )
      {
        relf_cn();    
        relf_indent(indent+1);
        
        relf_show_memory("target_memory", ptr, symbol.st_size > 16 ? 16 : symbol.st_size);
      }
    }

    
    relf_n();
    relf_indent(indent);
    relf_co();
    
    
    i++;
  }
  relf_n();
  relf_indent(indent-1);
  relf_ca(); // close array
  return 1;
}

int relf_show_dyn_data(relf_struct *relf, Elf_Scn  *scn, Elf_Data *data)
{
  int i = 0;
/*
      typedef struct
      {
        Elf64_Sxword	d_tag;			// Dynamic entry type
        union
          {
            Elf64_Xword d_val;		// Integer value
            Elf64_Addr d_ptr;			// Address value 
          } d_un;
      } Elf64_Dyn;
*/
  GElf_Dyn dynamic;
  int indent = 6;
  int is_first = 1;


  relf_cn();
  relf_indent(indent-1);
  relf_member("dynamic_list");
  relf_n();
  relf_indent(indent-1);
  relf_oa();    // open array
  
  /* extern GElf_Dyn *gelf_getdyn (Elf_Data *__data, int __ndx, GElf_Dyn *__dst); */
  while( gelf_getdyn(data, i, &dynamic) != NULL )
  {
    if ( is_first ) 
      is_first = 0;
    else
      relf_cn();
    
    relf_indent(indent);
    relf_oo();
    
    relf_indent(indent+1);
    relf_show_et_value(et_d_tag, "d_tag", dynamic.d_tag);
    relf_cn();
    relf_indent(indent+1);
    relf_show_pure_value("d_val", dynamic.d_un.d_val);
    
    /*
    relf_cn();
    relf_indent(indent+1);
    relf_show_pure_value("d_ptr", dynamic.d_un.d_ptr);
    */
 
    if ( dynamic.d_tag == DT_NEEDED )
    {
      relf_cn();
      relf_indent(indent+1);
      relf_show_string_value("lib_name", elf_strptr(relf->elf, relf->dynstr_section_index, dynamic.d_un.d_val ));
    }
    
    relf_n();
    relf_indent(indent);
    relf_co();
        
    i++;
  }
  relf_n();
  relf_indent(indent-1);
  relf_ca(); // close array
  return 1;
}

int relf_show_data(relf_struct *relf, Elf_Scn  *scn, Elf_Data *data, int corresponding_section_string_table_index)
{
  switch(data->d_type)
  {
    case ELF_T_SYM:             // used by SHT_SYMTAB, SHT_DYNSYM
      return relf_show_symbol_data(relf, scn, data, corresponding_section_string_table_index);
    case ELF_T_DYN:             // used by SHT_DYNAMIC
      return relf_show_dyn_data(relf, scn, data);
    default:
      return 1;
  }
  return 1;
}

/*
  Show the list of data blocks for a given section.
  Some sections have a corresponding string table.
  If such a corresponding string table exists, then corresponding_section_string_table_index will contain that number.
    Example is the index of ".strtab" for ".symtab", this means that if scn is ".symtab", 
    then corresponding_section_string_table_index will contain the index of ".strtab"
*/
int relf_show_data_list(relf_struct *relf, Elf_Scn  *scn, int corresponding_section_string_table_index)
{
  int indent = 4;
  GElf_Shdr shdr;
  long long unsigned data_cnt = 0;
  Elf_Data *data = NULL;
  int is_first = 1;

  if ( gelf_getshdr( scn, &shdr ) != &shdr )
    return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;
  
  relf_indent(indent-1);
  relf_member("data_list");
  relf_n();
  relf_indent(indent-1);
  relf_oa();    // open array
  
  for(;;)
  {
    if ( data_cnt >= shdr.sh_size )
      break;
    
    data = elf_getdata(scn , data);     // if data==NULL return first data, otherwise return next data
    if ( data == NULL )
      break;
    
    if ( is_first ) 
      is_first = 0;
    else
      relf_cn();
    relf_indent(indent);
    relf_oo();

    //relf_indent(indent+1);
    //relf_show_pure_value("d_buf", (long long int)(data->d_buf));
    //relf_cn();
    
    relf_indent(indent+1);
    relf_show_et_value(et_d_type, "d_type", data->d_type);
    relf_cn();
    relf_indent(indent+1);
    relf_show_pure_value("d_size", data->d_size);
    relf_cn();
    relf_indent(indent+1);
    relf_show_pure_value("d_off", data->d_off);
    relf_cn();
    relf_indent(indent+1);
    relf_show_pure_value("d_align", data->d_align);
    //printf("  Data block type=%lu size=%lu off=%lu\n", (unsigned long)data->d_type, (unsigned long)data->d_size, (unsigned long)data->d_off);
    
    relf_show_data(relf, scn, data, corresponding_section_string_table_index);
    
    
    data_cnt += data->d_size;
    relf_n();
    relf_indent(indent);
    relf_co();
  } // data (within section) loop
  relf_n();
  relf_indent(indent-1);
  relf_ca(); // close array
  return 1;
}

int relf_show_section(relf_struct *relf, Elf_Scn  *scn)
{
  int indent = 3;
  GElf_Shdr shdr;
  int section_string_table_index = 0;
  const char *section_name;
    if ( gelf_getshdr( scn, &shdr ) != &shdr )
      return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;
    section_name = elf_strptr(relf->elf, relf->section_header_string_table_index, shdr.sh_name );
    if ( section_name == NULL )
      return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;

  /*
    some sections require a string table, for example
      SHT_DYNSYM        requires a ".dynstr" section of type SHT_STRTAB
      SHT_SYMTAB         requires a ".strtab" section of type SHT_STRTAB
    This is checked here and assigned to section_string_table_index.
    The string table index is then passed to the show data procedure
  */
  switch (shdr.sh_type)
  {
    case SHT_DYNSYM:
      section_string_table_index = relf->dynstr_section_index;
      break;
    case SHT_SYMTAB:
      section_string_table_index = relf->strtab_section_index;
      break;
  }
  
    
  relf_indent(indent-1);
  relf_oo();
  /* 
      output the section index, the index is used by 
        char *elf_strptr (Elf *__elf, size_t __index, size_t __offset)
        Elf_Scn *elf_getscn (Elf *__elf, size_t __index);
  */
  relf_indent(indent);
  relf_show_pure_value("section_index", (long long unsigned)elf_ndxscn(scn));   // get the "official" section index
  relf_cn();
    
  relf_indent(indent);
  relf_show_string_value("sh_name", section_name);
  relf_cn();
  relf_indent(indent);
  relf_show_et_value(et_sh_type, "sh_type", shdr.sh_type);
  relf_cn();
  relf_indent(indent);
  relf_show_flag_value_list(et_sh_flags, "sh_flags", shdr.sh_flags);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("sh_addr", shdr.sh_addr);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("sh_offset", shdr.sh_offset);            // this is the file offset inside the elf file, useless in the elf file
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("sh_size", shdr.sh_size);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("sh_link", shdr.sh_link);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("sh_info", shdr.sh_info);
  relf_cn();

  relf_indent(indent);
  relf_show_pure_value("sh_addralign", shdr.sh_addralign);
  relf_cn();
  relf_indent(indent);
  relf_show_pure_value("sh_entsize", shdr.sh_entsize);
  relf_cn();
  
  
  relf_show_data_list(relf, scn, section_string_table_index);
  
  relf_n();
  relf_indent(indent-1);
  relf_co();    // close object
  //printf("Section %04lu %-18s type=%10lu size=%5ld entsize=%5ld\n", (unsigned long)elf_ndxscn(scn), section_name, (unsigned long)shdr.sh_type, (unsigned long)shdr.sh_size, (unsigned long)shdr.sh_entsize);
  return 1;
}


int relf_show_section_list(relf_struct *relf)
{
  int indent = 1;
  Elf_Scn  *scn;        // section descriptor
  int is_first = 1;
  /* loop over all sections */
  relf_indent(indent);
  relf_member("section_list");
  relf_n();
  relf_indent(indent);
  relf_oa();            // open array
  scn = elf_nextscn(relf->elf, NULL);
  while ( scn != NULL ) 
    {
    if ( is_first )
      is_first = 0;
    else
      relf_cn();                // comma + new line
      
    if ( relf_show_section(relf, scn) == 0 )
    {
      relf_ca();
      relf_n();
      return 0;
    }
    scn = elf_nextscn(relf->elf, scn);
  }
  relf_n();
  relf_indent(indent);
  relf_ca();    // close array
  relf_n();
  return 1;
}

int default_return_value = 123;

int main( int argc , char ** argv )
{
  relf_struct relf;
  
  if ( argc < 2)
    return 0;
  
  relf_init(&relf, argv[1]);
  relf_oo();
  relf_show_elf_header(&relf);
  relf_cn();
  
  relf_show_program_header_list(&relf);
  relf_cn();
  
  
  relf_show_section_list(&relf);
  relf_co();
  relf_n();
  
  relf_destroy(&relf);
  
  return default_return_value;
}



int mainx( int argc , char ** argv )
{
  int fd ;
  Elf * e ;
  char * k ;
  int elfclass;
  char *elfclassstr;
  size_t shdrnum;
  size_t shdrstrndx;
  size_t strtab_section_index; 
  size_t phdrnum;
  
  
  GElf_Ehdr ehdr;
  GElf_Shdr shdr;
  Elf_Scn  *scn;        // section descriptor
  Elf_Data *data;       // data info within a section
  size_t data_cnt;      // total data count so far...             
  const char *section_name;
  
  if ( argc != 2)
    return 0;
  if ( elf_version( EV_CURRENT ) == EV_NONE )
    return fprintf(stderr, "Incorrect libelf version: %s\n", elf_errmsg(-1) ), 0;

  fd = open( argv[1] , O_RDONLY , 0);
  if ( fd < 0)
    return perror(argv[1]), 0;
  
  if (( e = elf_begin( fd , ELF_C_READ, NULL )) == NULL )
    return fprintf(stderr, "elf_begin: %s\n", elf_errmsg(-1)), 0;

  switch ( elf_kind( e ) ) 
  {
    case ELF_K_AR :
      k = "ar" ;
      break ;
    case ELF_K_ELF :
      k = "elf" ;
      break ;
    case ELF_K_COFF :
      k = "coff" ;
      break ;
    case ELF_K_NONE :
    default :
      k = "unknown";
      break;
  }
  
  printf("elf kind: %s\n", k);
  
  if ( gelf_getehdr (e , & ehdr ) == NULL )
    return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;
  
  elfclass = gelf_getclass( e ); 
  switch ( elfclass )
  {
    case ELFCLASS32:
      elfclassstr = "32 bit";
      break;
    case ELFCLASS64:
      elfclassstr = "64 bit";
      break;
    case ELFCLASSNONE:
    default:
      elfclassstr = "unknown";
      break;
  }
  

//  etype
//  #define ET_NONE		0		/* No file type */
//  #define ET_REL		1		/* Relocatable file */
//  #define ET_EXEC		2		/* Executable file */
//  #define ET_DYN		3		/* Shared object file */
//  #define ET_CORE		4		/* Core file */
//  #define	ET_NUM		5		/* Number of defined types */
    
  
  printf("elf class: %s\n", elfclassstr);
  printf("elf header Object file type                   e_type=         %lu\n", (unsigned long)ehdr.e_type);
  printf("elf header Architecture                       e_machine=      %lu\n", (unsigned long)ehdr.e_machine);
  printf("elf header Object file version                e_version=      %lu\n", (unsigned long)ehdr.e_version);
  printf("elf header Entry point virtual address        e_entry=        %lu\n", (unsigned long)ehdr.e_entry);
  printf("elf header Program header table file offset   e_phoff=        %lu\n", (unsigned long)ehdr.e_phoff);
  printf("elf header Section header table file offset   e_shoff=        %lu\n", (unsigned long)ehdr.e_shoff);
  printf("elf header Processor-specific flags           e_flags=        0x%04lx\n", (unsigned long)ehdr.e_flags);
  printf("elf header Program header table entry size    e_phentsize=    %lu\n", (unsigned long)ehdr.e_phentsize);
  printf("elf header Section header table entry size    e_shentsize=    %lu\n", (unsigned long)ehdr.e_shentsize);
  
  
  if ( elf_getshdrnum(e, &shdrnum ) != 0 )
    return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;
  if ( elf_getshdrstrndx(e, &shdrstrndx ) != 0 )
    return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;
  if ( elf_getphdrnum(e, &phdrnum ) != 0 )
    return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;
  
  printf("elf header number of sections               shdrnum=    %lu\n", (unsigned long)shdrnum);
  printf("elf header section name string table index  shdrstrndx= %lu\n", (unsigned long)shdrstrndx);
  printf("elf header program header table entries     phdrnum=    %lu\n", (unsigned long)phdrnum);
  
  /* loop over all sections */
  scn = NULL;
  strtab_section_index = 35;
  while (( scn = elf_nextscn(e, scn)) != NULL ) 
  {
    if ( gelf_getshdr( scn, &shdr ) != &shdr )
      return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;
    section_name = elf_strptr(e, shdrstrndx, shdr.sh_name );
    if ( section_name == NULL )
      return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;
    
//typedef struct
//{
//  Elf32_Word	sh_name;		/* Section name (string tbl index) */
//  Elf32_Word	sh_type;		/* Section type */
//  Elf32_Word	sh_flags;		/* Section flags */
//  Elf32_Addr	sh_addr;		/* Section virtual addr at execution */
//  Elf32_Off	sh_offset;		/* Section file offset */
//  Elf32_Word	sh_size;		/* Section size in bytes */
//  Elf32_Word	sh_link;		/* Link to another section */
//  Elf32_Word	sh_info;		/* Additional section information */
//  Elf32_Word	sh_addralign;		/* Section alignment */
//  Elf32_Word	sh_entsize;		/* Entry size if section holds table */
//} Elf32_Shdr;
    
    printf("Section %04lu %-18s type=%10lu size=%5ld entsize=%5ld\n", (unsigned long)elf_ndxscn(scn), section_name, (unsigned long)shdr.sh_type, (unsigned long)shdr.sh_size, (unsigned long)shdr.sh_entsize);

    if ( shdr.sh_type == SHT_STRTAB )
    {
      // strtab_section_index = elf_ndxscn(scn); // we have to search for .strtab
    }


//typedef struct
//{
//  void *d_buf;			/* Pointer to the actual data.  */
//  Elf_Type d_type;		/* Type of this piece of data. Set to ELF_T_BYTE if unknown, Set to ELF_T_CHDR if compressed, otherwise it might be set to the parent type */
//  unsigned int d_version;	/* ELF version.  */
//  size_t d_size;		/* Size in bytes.  */
//  int64_t d_off;		/* Offset into section.  */
//  size_t d_align;		/* Alignment in section.  */
//} Elf_Data;
    
    data_cnt = 0;
    data = NULL;
    for(;;)
    {
      if ( data_cnt >= shdr.sh_size )
        break;
      data = elf_getdata(scn , data);
      if ( data == NULL )
        break;
      printf("  Data block type=%lu size=%lu off=%lu\n", (unsigned long)data->d_type, (unsigned long)data->d_size, (unsigned long)data->d_off);
      
      if ( shdr.sh_type == SHT_SYMTAB )
      {
        // https://man.freebsd.org/cgi/man.cgi?query=gelf_getsym&sektion=3&apropos=0&manpath=FreeBSD+7.1-RELEASE
        // typedef struct
        // {
        //   Elf64_Word	st_name;		/* Symbol name (string tbl index) */
        //   unsigned char	st_info;		/* Symbol type and binding */
        //   unsigned char st_other;		/* Symbol visibility */
        //   Elf64_Section	st_shndx;		/* Section index */
        //   Elf64_Addr	st_value;		/* Symbol value */
        //   Elf64_Xword	st_size;		/* Symbol size */
        // } Elf64_Sym;        
        GElf_Sym sym;
        int ndx = 0;
        for(;;)
        {
          if ( gelf_getsym(data, ndx, &sym) == NULL )
            break;
          printf("    Symbol %s\n", elf_strptr(e, strtab_section_index, sym.st_name ));
          ndx++;
        }

      }
      
      data_cnt += data->d_size;
    } // data (within section) loop
  } // section loop
  
  elf_end(e);  
  close(fd);
  
  return 1;
}