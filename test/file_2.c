// gcc -g file_1.c file_2.c -o file.elf && dwarfdump file.elf > file.out

extern int file_2_b;
int file_2_b = 7;

void  file_2_fn(void)
{
  file_2_b = 8;
}

