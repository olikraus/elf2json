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
  
  size_t strtab_section_header_index;               // section header index of the ".strtab" section, this contains the strings for the symbols from .symtab
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

void relf_show_et_value(elf_translate_struct *et, const char *variable, size_t n)
{
  printf("%s: [%zu, \"%s\", \"%s\"]", variable, n, et_get_macro(et, n), et_get_description(et, n));
}

void relf_show_pure_value(const char *variable, long long unsigned n)
{
  printf("%s: %llu", variable, n);
}

void relf_n()
{
  printf("\n");
}

void relf_c()
{
  printf(",");
}


void relf_show_elf_header(relf_struct *relf)
{
  char *ident = elf_getident(relf->elf , NULL);
  // EI_OSABI
  // ident[EI_ABIVERSION]
  relf_show_et_value(et_elf_class, "EI_CLASS", gelf_getclass( relf->elf ));
  relf_c(); relf_n();
  relf_show_pure_value("EI_DATA", ident[EI_DATA]);
  relf_c(); relf_n();
  relf_show_pure_value("EI_VERSION", ident[EI_VERSION]);
  relf_c(); relf_n();
  relf_show_et_value(et_elf_osabi, "EI_OSABI", ident[EI_OSABI]);
  relf_c(); relf_n();
  relf_show_pure_value("EI_ABIVERSION", ident[EI_ABIVERSION]);
  relf_c(); relf_n();
  relf_show_et_value(et_e_type, "e_type", relf->elf_file_header.e_type );
  relf_c(); relf_n();
  relf_show_et_value(et_e_machine, "e_machine", relf->elf_file_header.e_machine );
  relf_c(); relf_n();
  relf_show_pure_value("e_version", relf->elf_file_header.e_version);
  relf_c(); relf_n();
  relf_show_pure_value("e_entry", relf->elf_file_header.e_entry);
  relf_c(); relf_n();
  relf_show_pure_value("e_phoff", relf->elf_file_header.e_phoff);
  relf_c(); relf_n();
  relf_show_pure_value("e_shoff", relf->elf_file_header.e_shoff);
  relf_c(); relf_n();
  relf_show_pure_value("e_flags", relf->elf_file_header.e_flags);
  relf_c(); relf_n();
  relf_show_pure_value("e_ehsize", relf->elf_file_header.e_ehsize);
  relf_c(); relf_n();
  relf_show_pure_value("e_phentsize", relf->elf_file_header.e_phentsize);
  relf_c(); relf_n();
  relf_show_pure_value("e_phnum", relf->elf_file_header.e_phnum);
  relf_c(); relf_n();
  relf_show_pure_value("e_shentsize", relf->elf_file_header.e_shentsize);
  relf_c(); relf_n();
  relf_show_pure_value("e_shnum", relf->elf_file_header.e_shnum);
  relf_c(); relf_n();
  relf_show_pure_value("e_shstrndx", relf->elf_file_header.e_shstrndx);
  relf_n();
}

int relf_show_sections(relf_struct *relf)
{
  Elf_Scn  *scn;        // section descriptor
  GElf_Shdr shdr;
  const char *section_name;
  /* loop over all sections */
  scn = NULL;
  while (( scn = elf_nextscn(relf->elf, scn)) != NULL ) 
  {
    if ( gelf_getshdr( scn, &shdr ) != &shdr )
      return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;
    section_name = elf_strptr(relf->elf, relf->section_header_string_table_index, shdr.sh_name );
    if ( section_name == NULL )
      return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;
    printf("Section %04lu %-18s type=%10lu size=%5ld entsize=%5ld\n", (unsigned long)elf_ndxscn(scn), section_name, (unsigned long)shdr.sh_type, (unsigned long)shdr.sh_size, (unsigned long)shdr.sh_entsize);
  }
  return 1;
}

int main( int argc , char ** argv )
{
  relf_struct relf;
  
  if ( argc < 2)
    return 0;
  
  relf_init(&relf, argv[1]);
  relf_show_elf_header(&relf);
  relf_show_sections(&relf);
  relf_destroy(&relf);
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