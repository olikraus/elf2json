// gcc -g file_1.c file_2.c -o file.elf && dwarfdump file.elf > file.out

extern int file_2_b;
void file_2_fn(void);

int file_1_a = 4;


int main()
{
  int file_1_x = 2;
  file_2_fn();
  file_1_x += file_1_a + file_2_b;
  return file_1_x;
}
