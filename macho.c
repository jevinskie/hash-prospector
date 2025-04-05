#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    uint64_t addr;
} symbol_t;

/**
 * Writes a Mach-O object file for arm64 macOS with the specified code and symbols.
 *
 * @param obj_path   Output file name
 * @param code       Byte array containing the JIT-generated code
 * @param code_size  Size of the code in bytes (up to 16 KB)
 * @param symbols    Array of symbols to include in the symbol table
 * @param num_symbols Number of symbols in the array
 */
void write_macho(const char *obj_path, const uint8_t *code, size_t code_size,
                 const symbol_t *symbols, size_t num_symbols) {
    /* Validate code size */
    if (code_size > 0x4000) { /* 16 KB */
        fprintf(stderr, "Code size exceeds 16 KB\n");
        exit(1);
    }

    /* Calculate sizes and offsets */
    const size_t header_size     = sizeof(struct mach_header_64);
    const size_t seg_cmd_size    = sizeof(struct segment_command_64) + sizeof(struct section_64);
    const size_t symtab_cmd_size = sizeof(struct symtab_command);
    const size_t total_load_commands_size = seg_cmd_size + symtab_cmd_size;
    const uint64_t section_offset         = header_size + total_load_commands_size;
    const uint64_t symtab_offset          = section_offset + code_size;

    /* Calculate string table size: 4 null bytes + sum of name lengths including null terminators */
    size_t strtab_size = 4; /* Initial 4 null bytes */
    for (size_t i = 0; i < num_symbols; i++) {
        strtab_size += strlen(symbols[i].name) + 1;
    }
    const uint64_t strtab_offset = symtab_offset + num_symbols * sizeof(struct nlist_64);

    /* Mach-O header */
    const struct mach_header_64 header = {
        .magic      = MH_MAGIC_64,              /* 64-bit Mach-O magic number */
        .cputype    = CPU_TYPE_ARM64,           /* ARM64 architecture */
        .cpusubtype = CPU_SUBTYPE_ARM64_ALL,    /* Generic ARM64 subtype */
        .filetype   = MH_OBJECT,                /* Object file type */
        .ncmds      = 2,                        /* Number of load commands */
        .sizeofcmds = total_load_commands_size, /* Total size of load commands */
        .flags      = 0                         /* No special flags */
    };

    /* Segment command for __TEXT */
    const struct segment_command_64 seg_cmd = {
        .cmd      = LC_SEGMENT_64,                  /* 64-bit segment command */
        .cmdsize  = seg_cmd_size,                   /* Size including section header */
        .segname  = "__TEXT",                       /* Segment name */
        .vmaddr   = 0x100000000,                    /* VM address, 16 KB aligned */
        .vmsize   = 0x4000,                         /* VM size: 16 KB */
        .fileoff  = section_offset,                 /* File offset of section data */
        .filesize = code_size,                      /* File size matches code size */
        .maxprot  = VM_PROT_READ | VM_PROT_EXECUTE, /* Maximum permissions */
        .initprot = VM_PROT_READ | VM_PROT_EXECUTE, /* Initial permissions */
        .nsects   = 1,                              /* One section */
        .flags    = 0                               /* No flags */
    };

    /* Section header for __text */
    struct section_64 text_section = {
        .sectname = "__text",       /* Section name */
        .segname  = "__TEXT",       /* Segment name */
        .addr     = 0x100000000,    /* VM address matches segment */
        .size     = code_size,      /* Size of the code */
        .offset   = section_offset, /* File offset matches segment */
        .align    = 2,              /* 2^2 = 4-byte alignment */
        .reloff   = 0,              /* No relocations */
        .nreloc   = 0,              /* Number of relocations: 0 */
        .flags =
            S_REGULAR | S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS /* Executable code */
    };

    /* Build string table */
    char *strtab = malloc(strtab_size);
    if (!strtab) {
        perror("malloc strtab");
        exit(1);
    }
    memset(strtab, 'A', strtab_size);
    memset(strtab, 0, 4); /* First 4 bytes are null?? not just first? */

    /* Build symbol table and populate string table */
    struct nlist_64 *symtab = malloc(num_symbols * sizeof(struct nlist_64));
    if (!symtab) {
        perror("malloc symtab");
        free(strtab);
        exit(1);
    }
    uint32_t strtab_off = 4; /* Start after initial 4 null bytes */
    for (size_t i = 0; i < num_symbols; i++) {
        strcpy(strtab + strtab_off, symbols[i].name);
        symtab[i].n_un.n_strx = strtab_off;         /* String table index */
        symtab[i].n_type      = N_SECT | N_EXT;     /* Defined, external symbol */
        symtab[i].n_sect      = 1;                  /* Section number (1-based) */
        symtab[i].n_desc      = 0;                  /* No special description */
        symtab[i].n_value     = symbols[i].address; /* Symbol address */
        strtab_off += strlen(symbols[i].name) + 1;
    }

    /* Symbol table command */
    const struct symtab_command symtab_cmd = {
        .cmd     = LC_SYMTAB,       /* Symbol table command */
        .cmdsize = symtab_cmd_size, /* Size of the command */
        .symoff  = symtab_offset,   /* File offset of symbol table */
        .nsyms   = num_symbols,     /* Number of symbols */
        .stroff  = strtab_offset,   /* File offset of string table */
        .strsize = strtab_size      /* Size of string table */
    };

    /* Write the Mach-O file */
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen");
        free(strtab);
        free(symtab);
        exit(1);
    }

    fwrite(&header, sizeof(header), 1, fp);                       /* Header */
    fwrite(&seg_cmd, sizeof(seg_cmd), 1, fp);                     /* Segment command */
    fwrite(&text_section, sizeof(text_section), 1, fp);           /* Section header */
    fwrite(&symtab_cmd, sizeof(symtab_cmd), 1, fp);               /* Symbol table command */
    fwrite(code, code_size, 1, fp);                               /* Code data */
    fwrite(symtab, num_symbols * sizeof(struct nlist_64), 1, fp); /* Symbol table */
    fwrite(strtab, strtab_size, 1, fp);                           /* String table */

    fclose(fp);
    free(strtab);
    free(symtab);
}

/* Example usage */
int main(void) {
    /* Sample JIT-generated code (replace with actual code) */
    uint8_t code[] = {
        0x00, 0x00, 0x80, 0xd2, /* mov x0, #0 */
        0xc0, 0x03, 0x5f, 0xd6  /* ret */
    };
    size_t code_size = sizeof(code);

    /* Define symbols */
    symbol_t symbols[] = {
        {"_start", 0x100000000}, /* Entry point */
        {"_ret", 0x100000004}    /* Return instruction */
    };
    size_t num_symbols = sizeof(symbols) / sizeof(symbols[0]);

    write_macho_file("output.o", code, code_size, symbols, num_symbols);
    printf("Mach-O object file 'output.o' written successfully.\n");
    return 0;
}
