# elf2json

## Links

libelf by Example: https://atakua.org/old-wp/wp-content/uploads/2015/03/libelf-by-example-20100112.pdf

ELF Format Cheatsheet: https://gist.github.com/x0nu11byt3/bcb35c3de461e5fb66173071a2379779


Tool Interface Standard (TIS) Executable and Linking:
https://refspecs.linuxfoundation.org/elf/elf.pdf


https://codebrowser.dev/linux/include/elf.h.html

https://man7.org/linux/man-pages/man5/elf.5.html

ELF man pages: https://docs.oracle.com/cd/E19253-01/816-5172/6mbb7btor/index.html

Alternative Tool: https://github.com/m4b/elf2json

## Background

This tool will use libelf, which is based on `elf.h` types.
On linux it might be required to install libelf-dev.

## Notes on the elf2json output

 * A valid `section_index` is always greater or equal to one (because `SHN_UNDEF` is defined as 0).
 * The section index values `symtab_section_index`, `strtab_section_index`, `dynsym_section_index` and `dynstr_section_index` are added to the root object of the JSON output.
  These section index values might be zero, if the corresponding section doesn't exist. The section index values are **not** the index value into the JSON section list, instead the section index
  will match the value of the `section_index` member of the section.
 * Strings are always resolved: The string is printed as JSON value instead of the string index (section name, symbol name, etc).
 * For a data object symbel, the `target_memory` member will show  the first view bytes of the data object (for example this will show the init value of a global variable).
 