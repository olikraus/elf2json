/*

  elf2bin.c


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

  Write the section content to a binary file, similar to
    objcopy -O binary input.elf output.bin
  
*/

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
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


int elf2bin(Elf *elf, const char *outfile)
{
  Elf_Scn  *scn  = NULL;
  GElf_Shdr shdr;
  size_t block_addr = 0;
  
  if ( elf_kind( elf ) != ELF_K_ELF )
  {
    fprintf(stderr, "Not an elf file (found kind %d)\n", elf_kind( elf ));
    return 0;
  }

  // S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
  int fd = creat(outfile, S_IRWXU);
  if ( fd < 0 )
  {
    perror(outfile);
    return 0;
  }
  
  printf("address   size      starting values...\n");

  /* loop over all sections */
  while (( scn = elf_nextscn(elf, scn)) != NULL ) 
  {
    if ( gelf_getshdr( scn, &shdr ) != &shdr )
      return fprintf(stderr, "libelf: %s\n", elf_errmsg(-1)), 0;
    if ( (shdr.sh_flags & SHF_ALLOC) != 0 && shdr.sh_size > 0 )
    {
      /* loop over the data blocks of the section */
      Elf_Data *data = NULL;
      for(;;)
      {
        data = elf_getdata(scn , data);     // if data==NULL return first data, otherwise return next data
        if ( data == NULL )
          break;
        if ( data->d_buf == NULL )
          break;
        
        block_addr = shdr.sh_addr + data->d_off;    // calculate the address of this data in the target system, not 100% sure whether this is correct
        /* data->d_size contains the size of the block, data->d_buf a ptr to the internal memory */
        {
          size_t i;
          size_t cnt = data->d_size;
          if ( cnt > 16 )
            cnt = 16;
          /* output address and size to stdout */
          printf("%08lx: %08lx ", (unsigned long)block_addr, (unsigned long)data->d_size);

          /* output first 16 data values of the section */
          for( i = 0; i < cnt; i++ )
          {
            printf(" %02x", ((unsigned char *)data->d_buf)[i]);
          }
          printf("\n");          
        }
        
        lseek(fd, block_addr, SEEK_SET);
        write(fd, data->d_buf, data->d_size);        
      }
    }
  }
  close(fd);
  return 1;
}

int main(int argc, char **argv)
{
  int fd = -1;
  Elf *elf = NULL;
  char *elf_filename = NULL;
  if ( argc < 3 )
  {
    printf("%s <input.elf> <output.bin>\n", argv[0]);
    return 1;
  }

  if ( elf_version( EV_CURRENT ) == EV_NONE )
    return fprintf(stderr, "Incorrect libelf version: %s\n", elf_errmsg(-1) ), 0;
  
  elf_filename = argv[1];
  fd = open( elf_filename, O_RDONLY | O_BINARY, 0);
  if ( fd >= 0 )
  {
    if (( elf = elf_begin( fd , ELF_C_READ, NULL )) != NULL )
    {
      if ( elf2bin(elf, argv[2]) )
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

