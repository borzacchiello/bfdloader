#define PACKAGE         ""
#define PACKAGE_VERSION ""

#include <bfd.h>
#include <stdlib.h>
#include "bfdloader.h"

static void bfd_error_handler(const char* msg, va_list lst)
{
    // do not print on stdout
    (void)msg;
    (void)lst;
    return;
}

static void load_sections(LoadedBinary* ctx)
{
    bfd* bfdctx = (bfd*)ctx->bfd_ctx;

    ctx->n_sections = 0;
    for (asection* bfd_sec = bfdctx->sections; bfd_sec != (asection*)NULL;
         bfd_sec           = bfd_sec->next) {
        if (bfd_section_size(bfd_sec) == 0 || bfd_section_vma(bfd_sec) == 0)
            continue;

        ctx->n_sections += 1;
    }

    ctx->sections  = malloc(sizeof(Section) * ctx->n_sections);
    uint64_t secid = 0;
    for (asection* bfd_sec = bfdctx->sections; bfd_sec != (asection*)NULL;
         bfd_sec           = bfd_sec->next) {

        if (bfd_section_size(bfd_sec) == 0 || bfd_section_vma(bfd_sec) == 0)
            continue;

        uint8_t perm = BFDLOADER_PERM_READ;
        if (!(bfd_section_flags(bfd_sec) & SEC_READONLY))
            perm |= BFDLOADER_PERM_WRITE;
        if (bfd_section_flags(bfd_sec) & EXEC_P)
            perm |= BFDLOADER_PERM_EXEC;

        char*    secname = strdup(bfd_section_name(bfd_sec));
        uint64_t secsize = bfd_section_size(bfd_sec);
        uint64_t addr    = bfd_section_vma(bfd_sec);

        uint8_t* secdata = malloc(secsize);
        bfd_simple_get_relocated_section_contents(bfdctx, bfd_sec, secdata,
                                                  NULL);

        ctx->sections[secid].addr = addr;
        ctx->sections[secid].data = secdata;
        ctx->sections[secid].size = secsize;
        ctx->sections[secid].name = secname;
        ctx->sections[secid].perm = perm;
        secid++;
    }
}

static void process_symtable(LoadedBinary* ctx, asymbol* symbol_table[],
                             size_t number_of_symbols)
{
    uint64_t symoff;
    if (ctx->symbols == NULL) {
        symoff         = 0;
        ctx->symbols   = malloc(sizeof(Symbol) * number_of_symbols);
        ctx->n_symbols = number_of_symbols;
    } else {
        symoff = ctx->n_symbols;
        ctx->symbols =
            realloc(ctx->symbols, sizeof(Symbol) *
                                      (ctx->n_symbols + number_of_symbols));
        ctx->n_symbols += number_of_symbols;
    }

    symbol_info symbolinfo;
    for (int i = 0; i < number_of_symbols; i++) {
        asymbol* symbol = symbol_table[i];

        bfd_symbol_info(symbol_table[i], &symbolinfo);
        if (symbolinfo.value == 0) {
            ctx->n_symbols--;
            continue;
        }

        char*             name = strdup(symbolinfo.name);
        SymbolType type = BFDLOADER_SYMBOL_UNKNOWN;
        if (symbol->flags & BSF_FUNCTION)
            type = BFDLOADER_SYMBOL_FUNCTION;
        else if (symbol->flags & BSF_LOCAL)
            type = BFDLOADER_SYMBOL_LOCAL;
        else if (symbol->flags & BSF_GLOBAL)
            type = BFDLOADER_SYMBOL_GLOBAL;

        Symbol* sym = &ctx->symbols[symoff];
        sym->name          = name;
        sym->ty            = type;
        sym->addr          = symbolinfo.value;

        symoff++;
    }
}

static int load_symtab(LoadedBinary* ctx)
{
    bfd* bfdctx = (bfd*)ctx->bfd_ctx;

    long storage_needed = bfd_get_symtab_upper_bound(bfdctx);
    if (storage_needed < 0)
        return 1;
    if (storage_needed == 0)
        return 0;

    asymbol* symbol_table[storage_needed];
    long     number_of_symbols = bfd_canonicalize_symtab(bfdctx, symbol_table);
    if (number_of_symbols < 0)
        return 1;
    process_symtable(ctx, symbol_table, number_of_symbols);
    return 0;
}

static int load_dyn_symtab(LoadedBinary* ctx)
{
    bfd* bfdctx = (bfd*)ctx->bfd_ctx;

    long storage_needed = bfd_get_dynamic_symtab_upper_bound(bfdctx);
    if (storage_needed < 0)
        return 1;
    if (storage_needed == 0)
        return 0;

    asymbol* symbol_table[storage_needed];
    long     number_of_symbols = bfd_canonicalize_symtab(bfdctx, symbol_table);
    if (number_of_symbols < 0)
        return 1;
    process_symtable(ctx, symbol_table, number_of_symbols);
    return 0;
}

static int load_dyn_relocs(LoadedBinary* ctx)
{
    bfd* bfdctx = (bfd*)ctx->bfd_ctx;

    long reloc_size = bfd_get_dynamic_reloc_upper_bound(bfdctx);
    if (reloc_size < 0)
        return 1;

    if (reloc_size == 0)
        return 0;

    long dyn_sym_size = bfd_get_dynamic_symtab_upper_bound(bfdctx);
    if (dyn_sym_size < 0)
        return 1;
    if (dyn_sym_size == 0)
        return 0;

    asymbol* dyn_symbols[dyn_sym_size];
    long num_dyn_symbols = bfd_canonicalize_dynamic_symtab(bfdctx, dyn_symbols);
    if (num_dyn_symbols < 0)
        return 1;

    arelent* relpp[reloc_size];
    long relcount = bfd_canonicalize_dynamic_reloc(bfdctx, relpp, dyn_symbols);
    if (relcount < 0)
        return 1;

    ctx->relocations   = malloc(sizeof(Relocation) * relcount);
    ctx->n_relocations = (uint64_t)relcount;
    uint64_t reloff    = 0;

    for (long i = 0; i < relcount; ++i) {
        arelent*           reloc = relpp[i];
        Relocation* r     = &ctx->relocations[reloff];
        if ((*reloc->sym_ptr_ptr)->flags & BSF_FUNCTION) {
            r->name = strdup((*reloc->sym_ptr_ptr)->name);
            r->addr = reloc->address;
            r->ty   = BFDLOADER_RELOC_FUNCTION;
            reloff++;
        } else if ((*reloc->sym_ptr_ptr)->flags & BSF_GLOBAL) {
            r->name = strdup((*reloc->sym_ptr_ptr)->name);
            r->addr = reloc->address;
            r->ty   = BFDLOADER_RELOC_DATA;
            reloff++;
        } else {
            ctx->n_relocations--;
        }
    }
    return 0;
}

int bfdloader_load(LoadedBinary* ctx, const char* path)
{
    bfd_set_error_handler(bfd_error_handler);

    bfd* bfdctx = bfd_openr(path, NULL);
    if (bfdctx == NULL)
        return BFDLOADER_ERR_INVALID_PATH;

    if (!bfd_check_format(bfdctx, bfd_object)) {
        bfd_close(bfdctx);
        return BFDLOADER_ERR_UNEXPECTED_BINARY;
    }
    bfd_set_error(bfd_error_no_error);

    ctx->bfd_ctx    = (void*)bfdctx;
    ctx->last_error = "";
    ctx->entrypoint = bfd_get_start_address(bfdctx);

    const bfd_arch_info_type* bfd_arch_info = bfd_get_arch_info(bfdctx);
    ctx->arch = strdup(bfd_arch_info->printable_name);

    enum bfd_flavour flavour = bfd_get_flavour(bfdctx);
    if (bfd_get_flavour(bfdctx) == bfd_target_unknown_flavour) {
        bfd_close(bfdctx);
        return BFDLOADER_ERR_UNSUPPORTED_BINTYPE;
    }
    ctx->bintype     = strdup(bfd_flavour_name(flavour));
    ctx->sections    = NULL;
    ctx->symbols     = NULL;
    ctx->relocations = NULL;
    ctx->n_sections = ctx->n_symbols = ctx->n_relocations = 0;

    load_sections(ctx);
    if (load_symtab(ctx) != 0)
        ctx->last_error = "unable to load symtab";
    if (load_dyn_symtab(ctx) != 0)
        ctx->last_error = "unable to load dyn symtab";
    if (load_dyn_relocs(ctx) != 0)
        ctx->last_error = "unable to load dyn relocs";
    return 0;
}

void bfdloader_destroy(LoadedBinary* ctx)
{
    bfd* bfdctx = (bfd*)ctx->bfd_ctx;

    for (uint64_t i = 0; i < ctx->n_sections; ++i) {
        Section* sec = &ctx->sections[i];
        free(sec->name);
        free(sec->data);
    }
    free(ctx->sections);

    for (uint64_t i = 0; i < ctx->n_symbols; ++i) {
        Symbol* sym = &ctx->symbols[i];
        free(sym->name);
    }
    free(ctx->symbols);

    for (uint64_t i = 0; i < ctx->n_relocations; ++i) {
        Relocation* reloc = &ctx->relocations[i];
        free(reloc->name);
    }
    free(ctx->relocations);

    free(ctx->arch);
    free(ctx->bintype);
    bfd_close(bfdctx);
}
