#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    uint64_t addr;
} sym_t;

/*
 * Writes a Mach-O object file for arm64 macOS with the specified code and symbols.
 *
 * @param obj_path  Output file name
 * @param code      Byte array containing the JIT-generated code
 * @param code_sz   Size of the code in bytes (up to 16 KB)
 * @param syms      Array of symbols to include in the symbol table
 * @param num_syms  Number of symbols in the array
 */
void write_macho(const char *obj_path, const uint8_t *code, size_t code_sz, const sym_t *syms,
                 size_t num_syms) {
    // Validate code size
    if (code_sz > 0x4000) { // 16 KB
        fprintf(stderr, "Code size exceeds 16 KB\n");
        exit(1);
    }

    // Calculate sizes and offsets
    const size_t hdr_sz        = sizeof(struct mach_header_64);
    const size_t seg_cmd_sz    = sizeof(struct segment_command_64) + sizeof(struct section_64);
    const size_t symtab_cmd_sz = sizeof(struct symtab_command);
    const size_t total_lc_sz   = seg_cmd_sz + symtab_cmd_sz;
    const uint64_t sect_off    = hdr_sz + total_lc_sz;
    const uint64_t symtab_off  = sect_off + code_sz;
    const size_t symtab_sz     = num_syms * sizeof(struct nlist_64);

    // Calculate string table size: 4 null bytes + sum of name lengths including null terminators
    size_t strtab_sz = 4; // Initial 4 null bytes
    for (size_t i = 0; i < num_syms; i++) {
        strtab_sz += strlen(syms[i].name) + 1;
    }
    const uint64_t strtab_offset = symtab_off + symtab_sz;

    const size_t total_sz = strtab_offset + strtab_sz;

    if (total_sz > 0x4000) { // 16 KB
        fprintf(stderr, "Total size exceeds 16 KB\n");
        exit(1);
    }

    // Mach-O header
    const struct mach_header_64 hdr = {
        .magic      = MH_MAGIC_64,           // 64-bit Mach-O magic number
        .cputype    = CPU_TYPE_ARM64,        // ARM64 architecture
        .cpusubtype = CPU_SUBTYPE_ARM64_ALL, // Generic ARM64 subtype
        .filetype   = MH_OBJECT,             // Object file type
        .ncmds      = 2,                     // Number of load commands
        .sizeofcmds = total_lc_sz,           // Total size of load commands
        .flags      = 0                      // No special flags
    };

    // Segment command for __TEXT
    const struct segment_command_64 seg_cmd = {
        .cmd      = LC_SEGMENT_64,                  // 64-bit segment command
        .cmdsize  = seg_cmd_sz,                     // Size including section header
        .segname  = "__TEXT",                       // Segment name
        .vmaddr   = 0x100000000,                    // VM address, 16 KB aligned
        .vmsize   = 0x4000,                         // VM size: 16 KB
        .fileoff  = sect_off,                       // File offset of section data
        .filesize = code_sz,                        // File size matches code size
        .maxprot  = VM_PROT_READ | VM_PROT_EXECUTE, // Maximum permissions
        .initprot = VM_PROT_READ | VM_PROT_EXECUTE, // Initial permissions
        .nsects   = 1,                              // One section
        .flags    = 0                               // No flags
    };

    // Section header for __text
    struct section_64 text_sect = {
        .sectname = "__text",    // Section name
        .segname  = "__TEXT",    // Segment name
        .addr     = 0x100000000, // VM address matches segment
        .size     = code_sz,     // Size of the code
        .offset   = sect_off,    // File offset matches segment
        .align    = 2,           // 2^2 = 4-byte alignment
        .reloff   = 0,           // No relocations
        .nreloc   = 0,           // Number of relocations: 0
        .flags = S_REGULAR | S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS // Executable code
    };

    // Build string table
    char *strtab = malloc(strtab_sz);
    if (!strtab) {
        perror("malloc strtab");
        exit(1);
    }
    memset(strtab, 'A', strtab_sz);
    memset(strtab, 0, 4); // First 4 bytes are null?? not just first?

    // Build symbol table and populate string table
    struct nlist_64 *symtab = malloc(symtab_sz);
    if (!symtab) {
        perror("malloc symtab");
        free(strtab);
        exit(1);
    }
    uint32_t strtab_off = 4; // Start after initial 4 null bytes
    for (size_t i = 0; i < num_syms; i++) {
        strcpy(strtab + strtab_off, syms[i].name);
        symtab[i].n_un.n_strx = strtab_off;     // String table index
        symtab[i].n_type      = N_SECT | N_EXT; // Defined, external symbol
        symtab[i].n_sect      = 1;              // Section number (1-based)
        symtab[i].n_desc      = 0;              // No special description
        symtab[i].n_value     = syms[i].addr;   // Symbol address
        strtab_off += strlen(syms[i].name) + 1;
    }

    // Symbol table command
    const struct symtab_command symtab_cmd = {
        .cmd     = LC_SYMTAB,     // Symbol table command
        .cmdsize = symtab_cmd_sz, // Size of the command
        .symoff  = symtab_off,    // File offset of symbol table
        .nsyms   = num_syms,      // Number of syms
        .stroff  = strtab_off,    // File offset of string table
        .strsize = strtab_sz      // Size of string table
    };

    // Write the Mach-O file
    FILE *fp = fopen(obj_path, "wb");
    if (!fp) {
        perror("fopen");
        free(strtab);
        free(symtab);
        exit(1);
    }

    fwrite(&hdr, sizeof(hdr), 1, fp);               // Header
    fwrite(&seg_cmd, sizeof(seg_cmd), 1, fp);       // Segment command
    fwrite(&text_sect, sizeof(text_sect), 1, fp);   // Section header
    fwrite(&symtab_cmd, sizeof(symtab_cmd), 1, fp); // Symbol table command
    fwrite(code, code_sz, 1, fp);                   // Code data
    fwrite(symtab, symtab_sz, 1, fp);               // Symbol table
    fwrite(strtab, strtab_sz, 1, fp);               // String table

    fclose(fp);
    free(strtab);
    free(symtab);
}

#if 1
// Example usage
int main(void) {
    // Sample JIT-generated code (replace with actual code)
    uint8_t code[] = {
        0x00, 0x00, 0x80, 0xd2, // mov x0, #0
        0xc0, 0x03, 0x5f, 0xd6  // ret
    };
    size_t code_size = sizeof(code);

    // Define symbols
    sym_t syms[] = {
        {"_start", 0x100000000}, // Entry point
        {"_ret", 0x100000004}    // Return instruction
    };
    size_t num_syms = sizeof(syms) / sizeof(syms[0]);

    write_macho("output.o", code, code_size, syms, num_syms);
    printf("Mach-O object file 'output.o' written successfully.\n");
    return 0;
}
#endif
