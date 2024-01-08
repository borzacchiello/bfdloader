#include <stdlib.h>
#include <stdio.h>

#include "bfdloader.h"

static void usage(const char* progname)
{
    printf("USAGE: %s <binpath>\n", progname);
    exit(1);
}

static const char* symtype_to_string(SymbolType ty)
{
    switch (ty) {
        case BFDLOADER_SYMBOL_FUNCTION:
            return "funct";
        case BFDLOADER_SYMBOL_GLOBAL:
            return "globl";
        case BFDLOADER_SYMBOL_LOCAL:
            return "local";
        default:
            break;
    }
    return "unknw";
}

static const char* reloctype_to_string(RelocationType ty)
{
    switch (ty) {
        case BFDLOADER_RELOC_FUNCTION:
            return "func";
        case BFDLOADER_RELOC_DATA:
            return "data";
        default:
            break;
    }
    return "unkn";
}

static const char* perm_to_string(uint8_t perm)
{
    static char permstr[4] = {0};
    if (perm & BFDLOADER_PERM_READ)
        permstr[0] = 'r';
    else
        permstr[0] = '-';
    if (perm & BFDLOADER_PERM_WRITE)
        permstr[1] = 'w';
    else
        permstr[1] = '-';
    if (perm & BFDLOADER_PERM_EXEC)
        permstr[2] = 'x';
    else
        permstr[2] = '-';
    return permstr;
}

int main(int argc, char** argv)
{
    if (argc < 2)
        usage(argv[0]);

    LoadedBinary ctx;
    if (bfdloader_load(&ctx, argv[1]) != 0) {
        printf("!Err: unable to process the file\n");
        return 1;
    }

    printf("File \"%s\"\n", argv[1]);
    printf("  Binary Type:  %s\n", ctx.bintype);
    printf("  Architecture: %s\n", ctx.arch);
    printf("  Entrypoint:   0x%llx\n", (unsigned long long)ctx.entrypoint);
    printf("  Sections:\n");
    for (uint64_t i = 0; i < ctx.n_sections; ++i) {
        Section* sec = &ctx.sections[i];
        printf("    %08llx %s %s [%llu]\n", (unsigned long long)sec->addr,
               perm_to_string(sec->perm), sec->name,
               (unsigned long long)sec->size);
    }
    printf("  Symbols:\n");
    for (uint64_t i = 0; i < ctx.n_symbols; ++i) {
        Symbol* sym = &ctx.symbols[i];
        printf("    %08llx %s %s\n", (unsigned long long)sym->addr,
               symtype_to_string(sym->ty), sym->name);
    }
    printf("  Relocations:\n");
    for (uint64_t i = 0; i < ctx.n_relocations; ++i) {
        Relocation* reloc = &ctx.relocations[i];
        printf("    %08llx %s %s\n", (unsigned long long)reloc->addr,
               reloctype_to_string(reloc->ty), reloc->name);
    }

    bfdloader_destroy(&ctx);
    return 0;
}
