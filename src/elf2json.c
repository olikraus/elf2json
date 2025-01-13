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


  Notes:
    - functions objects change if they move in memory
    - function objects don't have rela entries
    - function objects change check: equal size and: within 8 bytes max 4 differences

  Get index of section.
    extern size_t elf_ndxscn(Elf_Scn *__scn);

  Get section at INDEX.
    Elf_Scn *elf_getscn (Elf *__elf, size_t __index);
    
  
  objcopy -O ihex input.elf output.hex
  objcopy --info                list all supported formats

*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <libelf.h>
#include <gelf.h>
#include <string.h>
#include <assert.h>

/*==========================================*/
/* Target System Special Code */

#if defined(__MINGW32__) || defined(__MINGW64__)
// seems to be missing on mingw
unsigned long __stack_chk_guard = 0xaa55;
void __attribute__ ((noreturn)) __stack_chk_fail (void)
{
	exit(0);
}
#endif


/*==========================================*/
/* CRC32 from https://datatracker.ietf.org/doc/html/rfc1952#section-8 */
/* updated names and data-types */

/* Table of CRCs of all 8-bit messages. */
unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
int is_crc_table_computed = 0;

/* Make the table for a fast CRC. */
void compute_crc_table(void)
{
  unsigned long c;
  int n, k;
  for (n = 0; n < 256; n++) 
  {
    c = (unsigned long) n;
    for (k = 0; k < 8; k++) 
    {
      if (c & 1) 
      {
        c = 0xedb88320L ^ (c >> 1);
      } 
      else 
      {
        c = c >> 1;
      }
    }
    crc_table[n] = c;
  }
  is_crc_table_computed = 1;
}

/*
 Update a running crc with the bytes buf[0..len-1] and return
 the updated crc. The crc should be initialized to zero. Pre- and
 post-conditioning (one's complement) is performed within this
 function so it shouldn't be done by the caller. Usage example:

   unsigned long crc = 0L;

   while (read_buffer(buffer, length) != EOF) {
     crc = update_crc(crc, buffer, length);
   }
   if (crc != original_crc) error();
*/
unsigned long update_crc(unsigned long crc, unsigned char *buf, size_t len)
{
  unsigned long c = crc ^ 0xffffffffL;
  size_t n;

  if (!is_crc_table_computed)
    compute_crc_table();
  for (n = 0; n < len; n++) 
  {
    c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c ^ 0xffffffffL;
}

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long get_crc(unsigned char *buf, size_t len)
{
  return update_crc(0L, buf, len);
}
      

/*==========================================*/

/* return an SHF_ALLOC section which is most close to the given addess */
/* addr will be updated */
Elf_Scn  *get_section_by_address(Elf * e, Elf64_Addr *addr)
{
  Elf64_Addr min_delta;
  Elf_Scn  *min_scn  = NULL;
  Elf64_Addr new_addr;
  
  GElf_Shdr shdr;
  Elf_Scn  *scn  = NULL;
  /* loop over all sections */
  scn = NULL;
  while (( scn = elf_nextscn(e, scn)) != NULL ) 
  {
    if ( gelf_getshdr( scn, &shdr ) != &shdr )
      return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), NULL;
    if ( (shdr.sh_flags & SHF_ALLOC) != 0 && shdr.sh_size > 0 )
    {
      if ( shdr.sh_addr >= *addr )
      {
        if ( min_delta > shdr.sh_addr -  *addr )
        {
          min_delta = shdr.sh_addr -  *addr;
          min_scn = scn;
          new_addr = shdr.sh_addr + shdr.sh_size;
        }
      }
    }
  }
  if ( min_scn != NULL )
  {
    /*
    if ( gelf_getshdr( min_scn, &shdr ) != &shdr )
      return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), NULL;
    fprintf(stderr, "%09lx: %09lx %09lx\n", *addr, shdr.sh_addr, shdr.sh_size);
    */
    *addr = new_addr;
  }
  return min_scn;
}

/* return symbel with index sym_idx from section with the number scn_idx */
const GElf_Sym *get_symbol(Elf *elf, size_t scn_idx, int sym_idx)
{
  static GElf_Sym symbol;
  
  Elf_Scn *scn = elf_getscn(elf, scn_idx);
  if ( scn != NULL )
  {
    // Elf_Data *elf_getdata (Elf_Scn *__scn, Elf_Data *__data);
    Elf_Data *data = NULL;
    size_t symbols_per_data;
    GElf_Shdr shdr;
    if ( gelf_getshdr( scn, &shdr ) != &shdr )
      return NULL;
    
    for(;;)
    {
      data = elf_getdata(scn , data);     // if data==NULL return first data, otherwise return next data
      if ( data == NULL )
        return NULL;
      symbols_per_data = data->d_size / shdr.sh_entsize;
      assert(shdr.sh_entsize*symbols_per_data == data->d_size);
      if ( sym_idx < symbols_per_data )
        break;
      sym_idx -= data->d_size / shdr.sh_entsize;
    }
    return gelf_getsym(data, sym_idx, &symbol);
  }
  return NULL;
}

const char *get_symbol_name(Elf *elf, size_t scn_idx, int sym_idx)
{
  const GElf_Sym *sym = get_symbol(elf, scn_idx, sym_idx);
  if ( sym != NULL )
  {
    Elf_Scn *scn = elf_getscn(elf, scn_idx);
    GElf_Shdr shdr;
    if ( gelf_getshdr( scn, &shdr ) != &shdr )
      return NULL;

    return elf_strptr(elf, shdr.sh_link, sym->st_name );
  }
  return NULL;
}




/*==========================================*/
/* ELF value, macro names and comments */

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
#ifdef ELFOSABI_SYSV
  ET( ELFOSABI_SYSV, "UNIX System V ABI "),
#endif
#ifdef ELFOSABI_HPUX
  ET( ELFOSABI_HPUX, "HP-UX "),
#endif
#ifdef ELFOSABI_NETBSD
  ET( ELFOSABI_NETBSD, "NetBSD.  "),
#endif
#ifdef ELFOSABI_GNU
  ET( ELFOSABI_GNU, "Object uses GNU ELF extensions.  "),
#endif
#ifdef ELFOSABI_SOLARIS
  ET( ELFOSABI_SOLARIS, "Sun Solaris.  "),
#endif
#ifdef ELFOSABI_AIX
  ET( ELFOSABI_AIX, "IBM AIX.  "),
#endif
#ifdef ELFOSABI_IRIX
  ET( ELFOSABI_IRIX, "SGI Irix.  "),
#endif
#ifdef ELFOSABI_FREEBSD
  ET( ELFOSABI_FREEBSD, "FreeBSD.  "),
#endif
#ifdef ELFOSABI_TRU64
  ET( ELFOSABI_TRU64, "Compaq TRU64 UNIX.  "),
#endif
#ifdef ELFOSABI_MODESTO
  ET( ELFOSABI_MODESTO, "Novell Modesto.  "),
#endif
#ifdef ELFOSABI_OPENBSD
  ET( ELFOSABI_OPENBSD, "OpenBSD.  "),
#endif
#ifdef ELFOSABI_ARM_AEABI
  ET( ELFOSABI_ARM_AEABI, "ARM EABI "),
#endif
#ifdef ELFOSABI_ARM
  ET( ELFOSABI_ARM, "ARM "),
#endif
#ifdef ELFOSABI_STANDALONE
  ET( ELFOSABI_STANDALONE, "Standalone (embedded) application "),
#endif
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
#ifdef EM_M32
  ET( EM_M32  , "AT&T WE 32100"),
#endif
#ifdef EM_SPARC
  ET( EM_SPARC	 , "SUN SPARC"),
#endif
#ifdef EM_386
  ET( EM_386     , "Intel 80386"),
#endif
#ifdef EM_68K
  ET( EM_68K	, "Motorola m68k family"),
#endif
#ifdef EM_88K
  ET( EM_88K	, "Motorola m88k family"),
#endif
#ifdef EM_IAMCU
  //ET( EM_IAMCU	 , "Intel MCU"),				// not in mingw
#endif
#ifdef EM_860
  ET( EM_860	, "Intel 80860"),
#endif
#ifdef EM_MIPS
  ET( EM_MIPS	, "MIPS R3000 big-endian"),
#endif
#ifdef EM_S370
  ET( EM_S370	, "IBM System/370"),
#endif
#ifdef EM_MIPS_RS3_LE
  ET( EM_MIPS_RS3_LE, "MIPS R3000 little-endian"),
#endif
#ifdef EM_PARISC
  ET( EM_PARISC, "HPPA"),
#endif
#ifdef EM_VPP500
  ET( EM_VPP500, "Fujitsu VPP500"),
#endif
#ifdef EM_SPARC32PLUS
  ET( EM_SPARC32PLUS, "Sun's 'v8plus'"),
#endif
#ifdef EM_960
  ET( EM_960	, "Intel 80960"),
#endif
#ifdef EM_PPC
  ET( EM_PPC	, "PowerPC"),
#endif
#ifdef EM_PPC64
  ET( EM_PPC64, "PowerPC 64-bit"),
#endif
#ifdef EM_S390
  ET( EM_S390	, "IBM S390"),
#endif
#ifdef EM_SPU
  ET( EM_SPU	, "IBM SPU/SPC"),					// not in mingw
#endif
#ifdef EM_V800
  ET( EM_V800	, "NEC V800 series"),
#endif
#ifdef EM_FR20
  ET( EM_FR20	, "Fujitsu FR20"),
#endif
#ifdef EM_RH32
  ET( EM_RH32	, "TRW RH-32"),
#endif
#ifdef EM_RCE
  ET( EM_RCE	, "Motorola RCE"),
#endif
#ifdef EM_ARM
  ET( EM_ARM	, "ARM"),
#endif
#ifdef EM_FAKE_ALPHA
  ET( EM_FAKE_ALPHA, "Digital Alpha"),			// not in mingw
#endif
#ifdef EM_SH
  ET( EM_SH	, "Hitachi SH"),
#endif
#ifdef EM_SPARCV9
  ET( EM_SPARCV9, "SPARC v9 64-bit"),
#endif
#ifdef EM_TRICORE
  ET( EM_TRICORE, "Siemens Tricore"),
#endif
#ifdef EM_ARC
  ET( EM_ARC	, "Argonaut RISC Core"),
#endif
#ifdef EM_H8_300
  ET( EM_H8_300, "Hitachi H8/300"),
#endif
#ifdef EM_H8_300H
  ET( EM_H8_300H, "Hitachi H8/300H"),
#endif
#ifdef EM_H8S
  ET( EM_H8S	, "Hitachi H8S"),
#endif
#ifdef EM_H8_500
  ET( EM_H8_500, "Hitachi H8/500"),
#endif
#ifdef EM_IA_64
  ET( EM_IA_64, "Intel Merced"),
#endif
#ifdef EM_MIPS_X
  ET( EM_MIPS_X, "Stanford MIPS-X"),
#endif
#ifdef EM_COLDFIRE
  ET( EM_COLDFIRE, "Motorola Coldfire"),
#endif
#ifdef EM_68HC12
  ET( EM_68HC12, "Motorola M68HC12"),
#endif
#ifdef EM_MMA
  ET( EM_MMA	, "Fujitsu MMA Multimedia Accelerator"),
#endif
#ifdef EM_PCP
  ET( EM_PCP	, "Siemens PCP"),
#endif
#ifdef EM_NCPU
  ET( EM_NCPU	, "Sony nCPU embedded RISC"),
#endif
#ifdef EM_NDR1
  ET( EM_NDR1	, "Denso NDR1 microprocessor"),
#endif
#ifdef EM_STARCORE
  ET( EM_STARCORE, "Motorola Start*Core processor"),
#endif
#ifdef EM_ME16
  ET( EM_ME16	, "Toyota ME16 processor"),
#endif
#ifdef EM_ST100
  ET( EM_ST100, "STMicroelectronic ST100 processor"),
#endif
#ifdef EM_TINYJ
  ET( EM_TINYJ, "Advanced Logic Corp. Tinyj emb.fam"),
#endif
#ifdef EM_X86_64
  ET( EM_X86_64, "AMD x86-64 architecture"),
#endif
#ifdef EM_PDSP
  ET( EM_PDSP	, "Sony DSP Processor"),
#endif
#ifdef EM_PDP10
  ET( EM_PDP10, "Digital PDP-10"),				// not in mingw
#endif
#ifdef EM_PDP11
  ET( EM_PDP11, "Digital PDP-11"),				// not in mingw
#endif
#ifdef EM_FX66
  ET( EM_FX66	, "Siemens FX66 microcontroller"),
#endif
#ifdef EM_ST9PLUS
  ET( EM_ST9PLUS, "STMicroelectronics ST9+ 8/16 mc"),
#endif
#ifdef EM_ST7
  ET( EM_ST7	, "STmicroelectronics ST7 8 bit mc"),
#endif
#ifdef EM_68HC16
  ET( EM_68HC16, "Motorola MC68HC16 microcontroller"),
#endif
#ifdef EM_68HC11
  ET( EM_68HC11, "Motorola MC68HC11 microcontroller"),
#endif
#ifdef EM_68HC08
  ET( EM_68HC08, "Motorola MC68HC08 microcontroller"),
#endif
#ifdef EM_68HC05
  ET( EM_68HC05, "Motorola MC68HC05 microcontroller"),
#endif
#ifdef EM_SVX
  ET( EM_SVX	, "Silicon Graphics SVx"),
#endif
#ifdef EM_ST19
  ET( EM_ST19	, "STMicroelectronics ST19 8 bit mc"),
#endif
#ifdef EM_VAX
  ET( EM_VAX	, "Digital VAX"),
#endif
#ifdef EM_CRIS
  ET( EM_CRIS	, "Axis Communications 32-bit emb.proc"),
#endif
#ifdef EM_JAVELIN
  ET( EM_JAVELIN, "Infineon Technologies 32-bit emb.proc"),
#endif
#ifdef EM_FIREPATH
  ET( EM_FIREPATH, "Element 14 64-bit DSP Processor"),
#endif
#ifdef EM_ZSP
  ET( EM_ZSP	, "LSI Logic 16-bit DSP Processor"),
#endif
#ifdef EM_MMIX
  ET( EM_MMIX	, "Donald Knuth's educational 64-bit proc"),
#endif
#ifdef EM_HUANY
  ET( EM_HUANY, "Harvard University machine-independent object files"),
#endif
#ifdef EM_PRISM
  ET( EM_PRISM, "SiTera Prism"),
#endif
#ifdef EM_AVR
  ET( EM_AVR	, "Atmel AVR 8-bit microcontroller"),
#endif
#ifdef EM_FR30
  ET( EM_FR30	, "Fujitsu FR30"),
#endif
#ifdef EM_D10V
  ET( EM_D10V	, "Mitsubishi D10V"),
#endif
#ifdef EM_D30V
  ET( EM_D30V	, "Mitsubishi D30V"),
#endif
#ifdef EM_V850
  ET( EM_V850	, "NEC v850"),
#endif
#ifdef EM_M32R
  ET( EM_M32R	, "Mitsubishi M32R"),
#endif
#ifdef EM_MN10300
  ET( EM_MN10300, "Matsushita MN10300"),
#endif
#ifdef EM_MN10200
  ET( EM_MN10200, "Matsushita MN10200"),
#endif
#ifdef EM_PJ
  ET( EM_PJ	, "picoJava"),
#endif
#ifdef EM_OPENRISC
  ET( EM_OPENRISC, "OpenRISC 32-bit embedded processor"),
#endif
#ifdef EM_ARC_COMPACT
  ET( EM_ARC_COMPACT, "ARC International ARCompact"),		// not in mingw
#endif
#ifdef EM_XTENSA
  ET( EM_XTENSA, "Tensilica Xtensa Architecture"),
#endif
#ifdef EM_VIDEOCORE
  ET( EM_VIDEOCORE, "Alphamosaic VideoCore"),
#endif
#ifdef EM_TMM_GPP
  ET( EM_TMM_GPP, "Thompson Multimedia General Purpose Proc"),
#endif
#ifdef EM_NS32K
  ET( EM_NS32K, "National Semi. 32000"),
#endif
#ifdef EM_TPC
  ET( EM_TPC	, "Tenor Network TPC"),
#endif
#ifdef EM_SNP1K
  ET( EM_SNP1K, "Trebia SNP 1000"),
#endif
#ifdef EM_ST200
  ET( EM_ST200, "STMicroelectronics ST200"),
#endif
#ifdef EM_IP2K
  ET( EM_IP2K	, "Ubicom IP2xxx"),
#endif
#ifdef EM_MAX
  ET( EM_MAX	, "MAX processor"),
#endif
#ifdef EM_CR
  ET( EM_CR	, "National Semi. CompactRISC"),
#endif
#ifdef EM_F2MC16
  ET( EM_F2MC16, "Fujitsu F2MC16"),
#endif
#ifdef EM_MSP430
  ET( EM_MSP430, "Texas Instruments msp430"),
#endif
#ifdef EM_BLACKFIN
  ET( EM_BLACKFIN, "Analog Devices Blackfin DSP"),
#endif
#ifdef EM_SE_C33
  ET( EM_SE_C33, "Seiko Epson S1C33 family"),
#endif
#ifdef EM_SEP
  ET( EM_SEP	, "Sharp embedded microprocessor"),
#endif
#ifdef EM_ARCA
  ET( EM_ARCA	, "Arca RISC"),
#endif
#ifdef EM_UNICORE
  ET( EM_UNICORE, "PKU-Unity & MPRC Peking Uni. mc series"),
#endif
#ifdef EM_EXCESS
  ET( EM_EXCESS, "eXcess configurable cpu"),		// not in mingw
#endif
#ifdef EM_DXP
  ET( EM_DXP	, "Icera Semi. Deep Execution Processor"),		// not in mingw
#endif
#ifdef EM_ALTERA_NIOS2
  ET( EM_ALTERA_NIOS2, "Altera Nios II"),					// not in mingw
#endif
#ifdef EM_CRX
  ET( EM_CRX	, "National Semi. CompactRISC CRX"),				// not in mingw
#endif
#ifdef EM_XGATE
  ET( EM_XGATE, "Motorola XGATE"),							// not in mingw
#endif
#ifdef EM_C166
  ET( EM_C166	, "Infineon C16x/XC16x"),						// not in mingw
#endif
#ifdef EM_M16C
  ET( EM_M16C	, "Renesas M16C"),										// not in mingw
#endif
#ifdef EM_DSPIC30F
  ET( EM_DSPIC30F, "Microchip Technology dsPIC30F"),		// not in mingw
#endif
#ifdef EM_CE
  ET( EM_CE	, "Freescale Communication Engine RISC"),		// not in mingw
#endif
#ifdef EM_M32C
  ET( EM_M32C	, "Renesas M32C"),		// not in mingw
#endif
#ifdef EM_TSK3000
  ET( EM_TSK3000, "Altium TSK3000"),		// not in mingw
#endif
#ifdef EM_RS08
  ET( EM_RS08	, "Freescale RS08"),		// not in mingw
#endif
#ifdef EM_SHARC
  ET( EM_SHARC, "Analog Devices SHARC family"),		// not in mingw
#endif
#ifdef EM_ECOG2
  ET( EM_ECOG2, "Cyan Technology eCOG2"),		// not in mingw
#endif
#ifdef EM_SCORE7
  ET( EM_SCORE7, "Sunplus S+core7 RISC"),		// not in mingw
#endif
#ifdef EM_DSP24
  ET( EM_DSP24, "New Japan Radio (NJR) 24-bit DSP"),		// not in mingw
#endif
#ifdef EM_VIDEOCORE3
  ET( EM_VIDEOCORE3, "Broadcom VideoCore III"),		// not in mingw
#endif
#ifdef EM_LATTICEMICO32
  ET( EM_LATTICEMICO32, "RISC for Lattice FPGA"),		// not in mingw
#endif
#ifdef EM_SE_C17
  ET( EM_SE_C17, "Seiko Epson C17"),		// not in mingw
#endif
#ifdef EM_TI_C6000
  ET( EM_TI_C6000, "Texas Instruments TMS320C6000 DSP"),		// not in mingw
#endif
#ifdef EM_TI_C2000
  ET( EM_TI_C2000, "Texas Instruments TMS320C2000 DSP"),		// not in mingw
#endif
#ifdef EM_TI_C5500
  ET( EM_TI_C5500, "Texas Instruments TMS320C55x DSP"),		// not in mingw
#endif
#ifdef EM_TI_ARP32
  ET( EM_TI_ARP32, "Texas Instruments App. Specific RISC"),		// not in mingw
#endif
#ifdef EM_TI_PRU
  ET( EM_TI_PRU, "Texas Instruments Prog. Realtime Unit"),		// not in mingw
#endif
#ifdef EM_MMDSP_PLUS
  ET( EM_MMDSP_PLUS, "STMicroelectronics 64bit VLIW DSP"),		// not in mingw
#endif
#ifdef EM_CYPRESS_M8C
  ET( EM_CYPRESS_M8C, "Cypress M8C"),		// not in mingw
#endif
#ifdef EM_R32C
  ET( EM_R32C	, "Renesas R32C"),		// not in mingw
#endif
#ifdef EM_TRIMEDIA
  ET( EM_TRIMEDIA, "NXP Semi. TriMedia"),		// not in mingw
#endif

#ifdef EM_QDSP6
  ET( EM_QDSP6, "QUALCOMM DSP6"),
#endif
#ifdef EM_8051
  ET( EM_8051	, "Intel 8051 and variants"),
#endif
#ifdef EM_STXP7X
  ET( EM_STXP7X, "STMicroelectronics STxP7x"),
#endif
#ifdef EM_NDS32
  ET( EM_NDS32, "Andes Tech. compact code emb. RISC"),
#endif
#ifdef EM_ECOG1X
  ET( EM_ECOG1X, "Cyan Technology eCOG1X"),
#endif
#ifdef EM_MAXQ30
  ET( EM_MAXQ30, "Dallas Semi. MAXQ30 mc"),
#endif
#ifdef EM_XIMO16
  ET( EM_XIMO16, "New Japan Radio (NJR) 16-bit DSP"),
#endif
#ifdef EM_MANIK
  ET( EM_MANIK, "M2000 Reconfigurable RISC"),
#endif
#ifdef EM_CRAYNV2
  ET( EM_CRAYNV2, "Cray NV2 vector architecture"),
#endif
#ifdef EM_RX
  ET( EM_RX	, "Renesas RX"),
#endif
#ifdef EM_METAG
  ET( EM_METAG, "Imagination Tech. META"),
#endif
#ifdef EM_MCST_ELBRUS
  ET( EM_MCST_ELBRUS, "MCST Elbrus"),
#endif
#ifdef EM_ECOG16
  ET( EM_ECOG16, "Cyan Technology eCOG16"),
#endif
#ifdef EM_CR16
  ET( EM_CR16	, "National Semi. CompactRISC CR16"),
#endif
#ifdef EM_ETPU
  ET( EM_ETPU	, "Freescale Extended Time Processing Unit"),
#endif
#ifdef EM_SLE9X
  ET( EM_SLE9X, "Infineon Tech. SLE9X"),
#endif
#ifdef EM_L10M
  ET( EM_L10M	, "Intel L10M"),
#endif
#ifdef EM_K10M
  ET( EM_K10M	, "Intel K10M"),
#endif
#ifdef EM_AARCH64
  ET( EM_AARCH64, "ARM AARCH64"),
#endif
#ifdef EM_AVR32
  ET( EM_AVR32, "Amtel 32-bit microprocessor"),
#endif
#ifdef EM_STM8
  ET( EM_STM8	, "STMicroelectronics STM8"),
#endif
#ifdef EM_TILE64
  ET( EM_TILE64, "Tilera TILE64"),
#endif
#ifdef EM_TILEPRO
  ET( EM_TILEPRO, "Tilera TILEPro"),
#endif
#ifdef EM_MICROBLAZE
  ET( EM_MICROBLAZE, "Xilinx MicroBlaze"),
#endif
#ifdef EM_CUDA
  ET( EM_CUDA	, "NVIDIA CUDA"),
#endif
#ifdef EM_TILEGX
  ET( EM_TILEGX, "Tilera TILE-Gx"),
#endif
#ifdef EM_CLOUDSHIELD
  ET( EM_CLOUDSHIELD, "CloudShield"),
#endif
#ifdef EM_COREA_1ST
  ET( EM_COREA_1ST, "KIPO-KAIST Core-A 1st gen."),
#endif
#ifdef EM_COREA_2ND
  ET( EM_COREA_2ND, "KIPO-KAIST Core-A 2nd gen."),
#endif
#ifdef EM_ARCV2
  ET( EM_ARCV2, "Synopsys ARCv2 ISA. "),
#endif
#ifdef EM_OPEN8
  ET( EM_OPEN8, "Open8 RISC"),
#endif
#ifdef EM_RL78
  ET( EM_RL78	, "Renesas RL78"),
#endif
#ifdef EM_VIDEOCORE5
  ET( EM_VIDEOCORE5, "Broadcom VideoCore V"),
#endif
#ifdef EM_78KOR
  ET( EM_78KOR, "Renesas 78KOR"),
#endif
#ifdef EM_56800EX
  ET( EM_56800EX, "Freescale 56800EX DSC"),
#endif
#ifdef EM_BA1
  ET( EM_BA1	, "Beyond BA1"),
#endif
#ifdef EM_BA2
  ET( EM_BA2	, "Beyond BA2"),
#endif
#ifdef EM_XCORE
  ET( EM_XCORE, "XMOS xCORE"),
#endif
#ifdef EM_MCHP_PIC
  ET( EM_MCHP_PIC, "Microchip 8-bit PIC(r)"),
#endif
#ifdef EM_INTELGT
  ET( EM_INTELGT, "Intel Graphics Technology"),
#endif
#ifdef EM_KM32
  ET( EM_KM32	, "KM211 KM32"),
#endif
#ifdef EM_KMX32
  ET( EM_KMX32, "KM211 KMX32"),
#endif
#ifdef EM_EMX16
  ET( EM_EMX16, "KM211 KMX16"),
#endif
#ifdef EM_EMX8
  ET( EM_EMX8	, "KM211 KMX8"),
#endif
#ifdef EM_KVARC
  ET( EM_KVARC, "KM211 KVARC"),
#endif
#ifdef EM_CDP
  ET( EM_CDP	, "Paneve CDP"),
#endif
#ifdef EM_COGE
  ET( EM_COGE	, "Cognitive Smart Memory Processor"),
#endif
#ifdef EM_COOL
  ET( EM_COOL	, "Bluechip CoolEngine"),
#endif
#ifdef EM_NORC
  ET( EM_NORC	, "Nanoradio Optimized RISC"),
#endif
#ifdef EM_CSR_KALIMBA
  ET( EM_CSR_KALIMBA, "CSR Kalimba"),
#endif
#ifdef EM_Z80
  ET( EM_Z80	, "Zilog Z80"),
#endif
#ifdef EM_VISIUM
  ET( EM_VISIUM, "Controls and Data Services VISIUMcore"),
#endif
#ifdef EM_FT32
  ET( EM_FT32	, "FTDI Chip FT32"),
#endif
#ifdef EM_MOXIE
  ET( EM_MOXIE, "Moxie processor"),
#endif
#ifdef EM_AMDGPU
  ET( EM_AMDGPU, "AMD GPU"),
#endif
#ifdef EM_RISCV
  ET( EM_RISCV, "RISC-V"),
#endif
#ifdef EM_BPF
  ET( EM_BPF	, "Linux BPF -- in-kernel virtual machine"),
#endif
#ifdef EM_CSKY
  ET( EM_CSKY	    , "C-SKY"),
#endif
#ifdef EM_LOONGARCH
  ET( EM_LOONGARCH, "LoongArch"),
#endif
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
#ifdef SHT_RELR
  ET(SHT_RELR, "RELR relative relocations"),
#endif
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
#ifdef SHF_COMPRESSED
  ET(SHF_COMPRESSED, "Section with compressed data."),
#endif
  ETNONE()
};

/* GELF_ST_BIND(st_bind) */
elf_translate_struct et_st_bind[] = {
  ET(STB_LOCAL, "Local symbol"),
  ET(STB_GLOBAL, "Global symbol"),
  ET(STB_WEAK, "Weak symbol"),
  ET(STB_NUM, "Number of defined types."),
#ifdef STB_GNU_UNIQUE
  ET(STB_GNU_UNIQUE, "Unique symbol."),
#endif
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
#ifdef STT_GNU_IFUNC
  ET(STT_GNU_IFUNC, "Symbol is indirect code object"),
#endif
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
#ifdef ELF_T_VDEF
  ET(ELF_T_VDEF,                   "Elf32_Verdef, Elf64_Verdef, ..."),
#endif
#ifdef ELF_T_VDAUX
  ET(ELF_T_VDAUX,                  "Elf32_Verdaux, Elf64_Verdaux, ..."),
#endif
#ifdef ELF_T_VNEED
  ET(ELF_T_VNEED,                  "Elf32_Verneed, Elf64_Verneed, ..."),
#endif
#ifdef ELF_T_VNAUX
  ET(ELF_T_VNAUX,                  "Elf32_Vernaux, Elf64_Vernaux, ..."),
#endif
#ifdef ELF_T_NHDR
  ET(ELF_T_NHDR,                   "Elf32_Nhdr, Elf64_Nhdr, ..."),
#endif
#ifdef ELF_T_SYMINFO
  ET(ELF_T_SYMINFO,		"Elf32_Syminfo, Elf64_Syminfo, ..."),
#endif
#ifdef ELF_T_MOVE
  ET(ELF_T_MOVE,			"Elf32_Move, Elf64_Move, ..."),
#endif
#ifdef ELF_T_LIB
  ET(ELF_T_LIB,			"Elf32_Lib, Elf64_Lib, ..."),
#endif
#ifdef ELF_T_GNUHASH
  ET(ELF_T_GNUHASH,		"GNU-style hash section. "),
#endif
#ifdef ELF_T_AUXV
  ET(ELF_T_AUXV,			"Elf32_auxv_t, Elf64_auxv_t, ..."),
#endif
#ifdef ELF_T_CHDR
  ET(ELF_T_CHDR,			"Compressed, Elf32_Chdr, Elf64_Chdr, ..."),
#endif
#ifdef ELF_T_NHDR8
  ET(ELF_T_NHDR8,			"Special GNU Properties note.  Same as Nhdr, except padding."),
#endif
#ifdef ELF_T_RELR
  ET(ELF_T_RELR,			"Relative relocation entry."),
#endif
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
#ifdef DT_SYMTAB_SHNDX
  ET(  DT_SYMTAB_SHNDX , "Address of SYMTAB_SHNDX section"),
#endif
#ifdef DT_RELRSZ
  ET(  DT_RELRSZ	        , "Total size of RELR relative relocations"),
#endif
#ifdef DT_RELR
  ET(  DT_RELR		        , "Address of RELR relative relocations"),
#endif
#ifdef DT_RELRENT
  ET(  DT_RELRENT	        , "Size of one RELR relative relocaction"),
#endif
  ETNONE()
};

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

/*==========================================*/
/* Read-Onle ELF wrapper for the gelf/elf library */


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

/* 54 0x85a53329 */
/* 54 0xe8a996fa  [243,15,30,250,85,72,137,229,72,131,236,16,72,137,125,248,72,137,117,240,72,139,85,240,72,139,69,248,72,137,198,72,141,5,7,77,0,0,72,137,199,184,0,0,0,0,232,192,243,255,255,144,201,195] */
/* 54 0xe332ee3e [243,15,30,250,85,72,137,229,72,131,236,16,72,137,125,248,72,137,117,240,72,139,85,240,72,139,69,248,72,137,198,72,141,5,1,77,0,0,72,137,199,184,0,0,0,0,232,182,243,255,255,144,201,195] */

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
    return fprintf(stderr, "libelf: %s, section_index=%lld \n", elf_errmsg(-1), (long long int)section_index), NULL;
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


int relf_show_symbol_data(relf_struct *relf, Elf_Scn  *scn, Elf_Data *data, int sh_link)
{
  
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
  int i = 0;
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

    symbol_name = elf_strptr(relf->elf, sh_link, symbol.st_name );
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
#ifdef GELF_ST_VISIBILITY
    relf_cn();    
    relf_indent(indent+1);
    relf_show_et_value(et_st_visibility, "ST_VISIBILITY", GELF_ST_VISIBILITY(symbol.st_other));
#endif 
    //relf_cn();
    
    if ( symbol.st_shndx > 0 )
    {
      unsigned char * ptr = (unsigned char *)relf_get_mem_ptr(relf, symbol.st_shndx, symbol.st_value);
      if ( ptr != NULL )
      {
        unsigned long crc = get_crc(ptr, symbol.st_size);
        relf_cn();    
        relf_indent(indent+1);
        relf_show_pure_value("obj_crc", crc);
        relf_cn();    
        relf_indent(indent+1);
        relf_show_memory("obj_data", ptr, symbol.st_size > 64 ? 64 : symbol.st_size);
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


/* Relocation table entry without addend (in section of type SHT_REL).  */
// ELF_T_REL
// typedef Elf64_Rel GElf_Rel;

/* Relocation table entry with addend (in section of type SHT_RELA).  */
// ELF_T_RELA
// typedef Elf64_Rela GElf_Rela;
// extern GElf_Rela *gelf_getrela (Elf_Data *__data, int __ndx, GElf_Rela *__dst);


/* Relative relocation entry (in section of type SHT_RELR).  */
// ELF_T_RELR
// typedef Elf64_Relr GElf_Relr;

//typedef struct
//{
//  Elf64_Addr	r_offset;		/* Address */
//  Elf64_Xword	r_info;			/* Relocation type and symbol index */
//  Elf64_Sxword	r_addend;		/* Addend */
//} Elf64_Rela;


//#define ELF64_R_SYM(i)			((i) >> 32)
//#define ELF64_R_TYPE(i)			((i) & 0xffffffff)
// processor specific reallocation types (ELF64_R_TYPE): https://docs.oracle.com/cd/E19120-01/open.solaris/819-0690/chapter6-26/index.html
// rtype seems to be derived from the machine: https://github.com/bminor/binutils-gdb/blob/d77a3144958c8370a46cf17a87ee3c9081da9038/binutils/readelf.c#L2031
// mapping from e_machine to the rtype set: https://github.com/bminor/binutils-gdb/blob/d77a3144958c8370a46cf17a87ee3c9081da9038/binutils/readelf.c#L2031
// actual realocation might happen here: https://repo.or.cz/glibc.git/blob/HEAD:/elf/dl-reloc.c, makro def is here: https://github.com/lattera/glibc/blob/master/elf/dynamic-link.h
// finally the machine dependent mapping is done here: https://github.com/lattera/glibc/blob/master/sysdeps/x86_64/dl-machine.h
// readelf -l (minus L) prints a mapping between programheaders and sections.





/*
  section:
    sh_link              	The section header index of the associated symbol table.        
    sh_info                      	The section header index of the section to which the relocation applies.
*/

int relf_show_rela_data(relf_struct *relf, Elf_Scn  *scn, Elf_Data *data, int sh_link)
{
  const char *symbol_name;
  int i = 0;
/*
typedef struct
{
  Elf64_Addr	r_offset;		// Address 
  Elf64_Xword	r_info;			// Relocation type and symbol index
  Elf64_Sxword	r_addend;		// Addend
} Elf64_Rela;
  extern GElf_Rela *gelf_getrela (Elf_Data *__data, int __ndx, GElf_Rela *__dst);
*/
  GElf_Rela rela;
  int indent = 6;
  int is_first = 1;
  //char *symbol_name = "(none)";


  relf_cn();
  relf_indent(indent-1);
  relf_member("rela_list");
  relf_n();
  relf_indent(indent-1);
  relf_oa();    // open array
  
  /* extern GElf_Rela *gelf_getrela (Elf_Data *__data, int __ndx, GElf_Rela *__dst); */
  while( gelf_getrela(data, i, &rela) != NULL )
  {
    if ( is_first ) 
      is_first = 0;
    else
      relf_cn();


    if ( sh_link > 0 )
    {
      symbol_name = get_symbol_name(relf->elf, sh_link, GELF_R_SYM(rela.r_info));
    }



    
    relf_indent(indent);
    relf_oo();
    
    relf_indent(indent+1);
    relf_show_pure_value("r_offset", rela.r_offset);
    relf_cn();

    relf_indent(indent+1);
    relf_show_pure_value( "SYM", GELF_R_SYM(rela.r_info));              // this is probably an index into the symbol table
    relf_cn();


    if ( symbol_name != NULL )
    {
      relf_indent(indent+1);
      relf_show_string_value("symbol_name", symbol_name);
      relf_cn();    
    }
    
    relf_indent(indent+1);
    relf_show_pure_value("TYPE", GELF_R_TYPE(rela.r_info));
    relf_cn();
    
    relf_indent(indent+1);
    relf_show_pure_value("r_addend", rela.r_addend);
      
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


/*
  dispatch procedure to show the data for a specific data type
  
  note: corresponding_section_string_table_index is used for ELF_T_SYM only, but is probably obsolete, because this is also aavailable in shdr.sh_link
      see Figure 4-12 in https://refspecs.linuxbase.org/elf/gabi4+/ch4.sheader.html
*/
int relf_show_data(relf_struct *relf, Elf_Scn  *scn, Elf_Data *data, int sh_link)
{
  switch(data->d_type)
  {
    case ELF_T_SYM:             // used by SHT_SYMTAB, SHT_DYNSYM
      return relf_show_symbol_data(relf, scn, data, sh_link);
    case ELF_T_DYN:             // used by SHT_DYNAMIC
      return relf_show_dyn_data(relf, scn, data);
    case ELF_T_RELA:
      return relf_show_rela_data(relf, scn, data, sh_link);
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

  Update: sh_link usually contains the corresponding string table (Figure 4-12 in https://refspecs.linuxbase.org/elf/gabi4+/ch4.sheader.html)
    It is probably better to take it from there if the table is SHT_SYMTAB or SHT_DYNSYM
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
    
    
    //relf_show_data(relf, scn, data, corresponding_section_string_table_index);
    relf_show_data(relf, scn, data, shdr.sh_link);
    
    
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

int relf_show_section(relf_struct *relf, Elf_Scn  *scn, int is_data)
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
      section_string_table_index = relf->dynstr_section_index;  // should be the same as shdr.sh_link, see Figure 4-12 in https://refspecs.linuxbase.org/elf/gabi4+/ch4.sheader.html
      assert(section_string_table_index == shdr.sh_link);
      break;
    case SHT_SYMTAB:
      section_string_table_index = relf->strtab_section_index; // should be the same as shdr.sh_link, see Figure 4-12 in https://refspecs.linuxbase.org/elf/gabi4+/ch4.sheader.html
      assert(section_string_table_index == shdr.sh_link);
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
  if ( is_data )
  {
    relf_cn();
    relf_show_data_list(relf, scn, section_string_table_index);
  }
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
      
    if ( relf_show_section(relf, scn, 1) == 0 )
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
  //relf_n();
  return 1;
}



int relf_show_section_addr_list(relf_struct *relf)
{
  int indent = 1;
  Elf64_Addr addr = 0;
  Elf_Scn  *scn;        // section descriptor
  int is_first = 1;
  /* loop over all sections */
  relf_indent(indent);
  relf_member("section_addr_list");
  relf_n();
  relf_indent(indent);
  relf_oa();            // open array
  scn = elf_nextscn(relf->elf, NULL);
  //while ( scn != NULL ) 
  for(;;)
  {
    scn = get_section_by_address(relf->elf, &addr);
    if ( scn == NULL )
      break;
    
    if ( is_first )
      is_first = 0;
    else
      relf_cn();                // comma + new line
    
    if ( relf_show_section(relf, scn, 0) == 0 )
    {
      relf_ca();
      relf_n();
      return 0;
    }
  }
  relf_n();
  relf_indent(indent);
  relf_ca();    // close array
  //relf_n();
  return 1;
}


int default_return_value = 123;

int main( int argc , char ** argv )
{
  relf_struct relf;
  
  if ( argc < 2)
    return 0;
  
  if ( relf_init(&relf, argv[1]) == 0 )
    return 0;
  relf_oo();
  relf_show_elf_header(&relf);
  relf_cn();
  
  relf_show_program_header_list(&relf);
  relf_cn();
  
  relf_show_section_list(&relf);
  relf_cn();

  relf_show_section_addr_list(&relf);
  relf_n();

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