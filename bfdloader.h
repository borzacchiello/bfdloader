#ifndef BFDLOADER_H
#define BFDLOADER_H

#include <stdint.h>

#define BFDLOADER_ERR_INVALID_PATH        1
#define BFDLOADER_ERR_UNEXPECTED_BINARY   2
#define BFDLOADER_ERR_UNSUPPORTED_BINTYPE 3

#define BFDLOADER_PERM_READ  1
#define BFDLOADER_PERM_WRITE 2
#define BFDLOADER_PERM_EXEC  4

typedef enum SymbolType {
    BFDLOADER_SYMBOL_UNKNOWN  = 0,
    BFDLOADER_SYMBOL_FUNCTION = 1,
    BFDLOADER_SYMBOL_LOCAL    = 2,
    BFDLOADER_SYMBOL_GLOBAL   = 3
} SymbolType;

typedef enum RelocationType {
    BFDLOADER_RELOC_UNKNOWN  = 0,
    BFDLOADER_RELOC_FUNCTION = 1,
    BFDLOADER_RELOC_DATA     = 2
} RelocationType;

typedef struct Section {
    char*    name;
    uint8_t  perm;
    uint64_t addr;
    uint8_t* data;
    uint64_t size;
} Section;

typedef struct Symbol {
    uint64_t   addr;
    char*      name;
    SymbolType ty;
} Symbol;

typedef struct Relocation {
    uint64_t       addr;
    char*          name;
    RelocationType ty;
} Relocation;

typedef struct LoadedBinary {
    void*    bfd_ctx;
    char*    arch;
    char*    bintype;
    uint64_t entrypoint;

    Section*    sections;
    uint64_t    n_sections;
    Symbol*     symbols;
    uint64_t    n_symbols;
    Relocation* relocations;
    uint64_t    n_relocations;

    const char* last_error;
} LoadedBinary;

int  bfdloader_load(LoadedBinary* lb, const char* path);
void bfdloader_destroy(LoadedBinary* lb);

#endif
