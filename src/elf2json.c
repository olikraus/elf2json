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



int main ( int argc , char ** argv )
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