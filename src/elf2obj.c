/*

  elf2obj.c


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

  List objects from the symbol table
  
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <libelf.h>
#include <gelf.h>
#include <string.h>
#include <assert.h>

/*
        {
            "section_index":[34, "0x00000022"],
            "sh_name": ".symtab",
            "sh_type":[2, "SHT_SYMTAB", "Symbol table"],
            "sh_flags":[0, []],
            "sh_addr":[0, "0x00000000"],
            "sh_offset":[77616, "0x00012f30"],
            "sh_size":[3048, "0x00000be8"],
            "sh_link":[35, "0x00000023"],
            "sh_info":[21, "0x00000015"],
            "sh_addralign":[8, "0x00000008"],
            "sh_entsize":[24, "0x00000018"],
            "data_list":
            [
                {
                    "d_type":[11, "ELF_T_SYM", "Symbol record."],
                    "d_size":[3048, "0x00000be8"],
                    "d_off":[0, "0x00000000"],
                    "d_align":[8, "0x00000008"],
                    "symbol_list":
                    [

                        {
                            "st_name": "get_section_by_address",
                            "st_value":[22129, "0x00005671"],
                            "st_size":[324, "0x00000144"],
                            "st_shndx":[16, "0x00000010"],
                            "st_info":[18, "0x00000012"],
                            "ST_BIND":[1, "STB_GLOBAL", "Global symbol"],
                            "ST_TYPE":[2, "STT_FUNC", "Symbol is a code object"],
                            "st_other":[0, "0x00000000"],
                            "ST_VISIBILITY":[0, "STV_DEFAULT", "Default symbol visibility rules"],
                            "obj_crc":[2219958429, "0x8451e09d"],
                            "obj_data":[243,15,30,250,85,72,137,229,72,131,196,128,72,137,125,136,72,137,117,128,100,72,139,4,37,40,0,0,0,72,137,69,248,49,192,72,199,69,152,0,0,0,0,72,199,69,168,0,0,0,0,72,199,69,168,0,0,0,0,233,182,0,0,0]
                        },

*/

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


/* 
  returns a pointer to a memory location of y symbol 
  if NULL is returned, then there is either an error or the destination memory doesn't exist (BSS area).  
*/
void *get_symbol_mem_ptr(Elf *elf, GElf_Sym *symbol)
{
  GElf_Shdr shdr;
  Elf_Scn *scn;
  Elf_Data *data = NULL;
  size_t block_addr = 0;
  
  size_t section_index = symbol->st_shndx;
  size_t addr = symbol->st_value;
  
  if ( section_index == 0 || section_index > 0x0fff0 )
    return NULL;
  
  scn = elf_getscn(elf,  section_index);  
  if ( scn == NULL )
    return fprintf(stderr, "libelf: %s, section_index=%llu \n", elf_errmsg(-1), (long long unsigned)section_index), NULL;
  if ( gelf_getshdr( scn, &shdr ) != &shdr )
    return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), NULL;

  /*
    shdr.sh_addr                contains the base address for the target memory
    shdr.sh_size                 contains the size of the section
  */
  
  if ( shdr.sh_size == 0 )
  {
    // fprintf(stderr, "get_symbol_mem_ptr: shdr.sh_size == 0\n");
    return NULL;
  }
  
  for(;;)
  {
    data = elf_getdata(scn , data);     // if data==NULL return first data, otherwise return next data
    if ( data == NULL )
      break;
    if ( data->d_buf == NULL )
    {
      //fprintf(stderr, "get_symbol_mem_ptr: no buffer\n");
      break;    // probably this is a BSS area
    }
    block_addr = shdr.sh_addr + data->d_off;    // calculate the address of this data in the target system, not 100% sure whether this is correct
    //printf("block_addr=%08lx addr=%08lx d_buf=%p\n", block_addr, addr, data->d_buf);
    if ( addr >= block_addr && addr < block_addr+data->d_size )  // check if the requested addr is inside the current block
    {
      return data->d_buf + addr - block_addr;   // found
    }
    else
    {
      // fprintf(stderr, "get_symbol_mem_ptr: block_addr=%lu - %lu addr=%lu\n", block_addr, block_addr+data->d_size, addr);      
    }
  }  
  return NULL;
}



int elf2obj(Elf *elf)
{
  Elf_Scn  *scn  = NULL;
  GElf_Shdr shdr;
  size_t section_header_string_table_index = 0;
  char *section_name = NULL;
  char *symbol_name = NULL;
  unsigned symbol_bind = 0;
  unsigned symbol_type = 0;
  
  
  if ( elf_kind( elf ) != ELF_K_ELF )
  {
    fprintf(stderr, "Not an elf file (found kind %d)\n", elf_kind( elf ));
    return 0;
  }

  if ( elf_getshdrstrndx(elf, &(section_header_string_table_index) ) < 0 )
  {
    fprintf(stderr, "Sectionheader string table not found libelf: %s\n", elf_errmsg(-1));
    return 0;
  }
  
  
  /* loop over all sections */
  while (( scn = elf_nextscn(elf, scn)) != NULL ) 
  {
    if ( gelf_getshdr( scn, &shdr ) != &shdr )
      return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;

    if ( shdr.sh_type == SHT_SYMTAB )  
    {
      /* shdr.sh_link contains the section number of the corresponding string table */
      
      section_name = elf_strptr(elf, section_header_string_table_index, shdr.sh_name );
      
      /* loop over the data blocks of the section */
      Elf_Data *data = NULL;
      for(;;)
      {
        data = elf_getdata(scn , data);     // if data==NULL return first data, otherwise return next data
        if ( data == NULL )
          break;
        if ( data->d_buf == NULL )
          break;
        
        if ( data->d_type == ELF_T_SYM )
        {
          int i = 0;
          GElf_Sym symbol;
          
          while( gelf_getsym(data, i, &symbol) == &symbol )
          {
            symbol_name = elf_strptr(elf, shdr.sh_link, symbol.st_name);
            symbol_bind = GELF_ST_BIND(symbol.st_info);
            symbol_type = GELF_ST_TYPE(symbol.st_info);
            if ( symbol_bind == STB_GLOBAL )
            {
              if ( symbol_type == STT_OBJECT || symbol_type == STT_FUNC )
              {
                if ( symbol.st_value > 0 && symbol.st_size > 0 ) // the symbol should exist and should have a size other than zero
                {
                  // symbol.st_shndx            section where the symbol content is located
                  // symbol.st_value            address of the symbol on the host
                  void *sym_ptr = get_symbol_mem_ptr(elf, &symbol);   // return value could be NULL
                  printf("section %s, data blk size %llu, type %c, val %8llu, size %8llu, ptr %8p, scn %3d, symbol %s\n", 
						section_name, 
						(long long unsigned)data->d_size, 
						symbol_type == STT_FUNC ? 'F' : 'O', 
						(long long unsigned)symbol.st_value, 
						(long long unsigned)symbol.st_size, 
						sym_ptr, symbol.st_shndx, symbol_name);
                } // existing & none-empty symbols
              } // symbol is function or object
            } // symbol with bind == STB_GLOBAL
            i++;
          } // loop over all symbols          
        } // data block type is ELF_T_SYM
      } // loop over data blocks of a section      
    } // section type is SHT_SYMTAB
  } // loop over all sections
  
  return 1;
}





int main(int argc, char **argv)
{
  int fd = -1;
  Elf *elf = NULL;
  char *elf_filename = NULL;
  if ( argc < 2 )
  {
    printf("%s <input.elf>\n", argv[0]);
    return 1;
  }

  if ( elf_version( EV_CURRENT ) == EV_NONE )
    return fprintf(stderr, "Incorrect libelf version: %s\n", elf_errmsg(-1) ), 0;
  
  elf_filename = argv[1];
  fd = open( elf_filename, O_RDONLY  | O_BINARY , 0);
  if ( fd >= 0 )
  {
    if (( elf = elf_begin( fd , ELF_C_READ, NULL )) != NULL )
    {
      if ( elf2obj(elf) )
      {
        elf_end(elf); 
        close(fd);  
        return 0;
      }
      else
      {
        fprintf(stderr, "Conversion failed\n");
      }
      elf_end(elf);
    }
    else
    {
      fprintf(stderr, "elf_begin failed: %s\n", elf_errmsg(-1));
    }
    close(fd);
  }
  else
  {
    perror(elf_filename);
  }
  return 0;
}


