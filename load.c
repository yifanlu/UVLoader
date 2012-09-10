#include "load.h"
#include "resolve.h"
#include "scefuncs.h"
#include "utils.h"

/********************************************//**
 *  \brief Loads an supported executable
 *  
 *  This function identifies and loads a 
 *  executable at the given file.
 *  Currently supports ELF and SCE executable.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int
uvl_load_exe (const char *filename, ///< Absolute path to executable
                    void **entry)   ///< Returned pointer to entry pointer
{
    char magic[4];

    *entry = NULL;
    IF_DEBUG LOG ("Opening %s for reading.", filename);
    PsvUID fd = sceIoOpen (filename, PSP2_O_RDONLY, PSP2_STM_RU);
    if (fd < 0)
    {
        LOG ("Cannot open executable file for loading.");
        return -1;
    }

    IF_DEBUG LOG ("Reading magic number.");
    if (sceIoRead (fd, magic, MAGIC_LEN) < MAGIC_LEN)
    {
        LOG ("Cannot read magic number.");
        return -1;
    }

    if (magic[0] == ELFMAG0)
    {
        if (magic[1] == ELFMAG1 && magic[2] == ELFMAG2 && magic[3] == ELFMAG3)
        {
            IF_DEBUG LOG ("Found a ELF, loading.");
            return uvl_load_elf (fd, 0, entry);
        }
    }
    else if (magic[0] == SCEMAG0)
    {
        if (magic[1] == SCEMAG1 && magic[2] == SCEMAG2 && magic[3] == SCEMAG3)
        {
            IF_DEBUG LOG ("Found a SCE, seeking to ELF.");
            if (sceIoLseek (fd, SCEHDR_LEN, PSP2_SEEK_SET) < 0)
            {
                LOG ("Cannot skip SCE header.");
                return -1;
            }
            IF_DEBUG LOG ("Loading ELF.");
            return uvl_load_elf (fd, SCEHDR_LEN, entry);
        }
    }

    // close file
    IF_DEBUG LOG ("Closing executable file.");
    if (sceIoClose (fd) < 0)
    {
        LOG ("Cannot close executable %s after loading.", filename);
        return -1;
    }

    LOG ("Invalid magic.");
    return -1;
}

/********************************************//**
 *  \brief Loads an ELF file
 *  
 *  Performs both loading and resolving NIDs
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_load_elf (PsvUID fd,            ///< File descriptor for ELF
              PsvOff start_offset,  ///< Starting position of the ELF in file
                void **entry)       ///< Returned pointer to entry pointer
{
    Elf32_Ehdr_t elf_hdr;
    u32_t i;
    void *base_address = NULL;
    *entry = NULL;

    // get headers
    IF_DEBUG LOG ("Reading headers.");
    if (sceIoRead (fd, &elf_hdr, sizeof (Elf32_Ehdr_t)) < sizeof (Elf32_Ehdr_t))
    {
        LOG ("Error reading ELF header.");
        return -1;
    }
    IF_DEBUG LOG ("Checking headers.");
    if (uvl_elf_check_header (&elf_hdr) < 0)
    {
        LOG ("Check header failed.");
        return -1;
    }

    // get mod_info
    module_info_t mod_info;
    IF_DEBUG LOG ("Getting module info.");
    if (uvl_elf_get_module_info (fd, start_offset, &elf_hdr, &mod_info) < 0)
    {
        LOG ("Cannot find module info section.");
        return -1;
    }

    // free memory
    Elf32_Phdr_t prog_hdrs[elf_hdr.e_phnum];
    IF_DEBUG LOG ("Seeking program headers.");
    if (sceIoLseek (fd, start_offset + elf_hdr.e_phoff, PSP2_SEEK_SET) < 0)
    {
        LOG ("Error seeking program header.");
        return -1;
    }
    IF_DEBUG LOG ("Reading program headers.");
    if (sceIoRead (fd, prog_hdrs, elf_hdr.e_phnum * elf_hdr.e_phentsize) < 0)
    {
        LOG ("Error reading program header.");
        return -1;
    }
    IF_DEBUG LOG ("Freeing memory.");
    if (uvl_elf_free_memory (prog_hdrs, elf_hdr.e_phnum) < 0)
    {
        LOG ("Error freeing memory.");
        return -1;
    }

    // actually load the ELF
    PsvUID memblock;
    void *blockaddr;
    u32_t length;
    IF_DEBUG LOG ("Loading %u program sections.", elf_hdr.e_phnum);
    for (i = 0; i < elf_hdr.e_phnum; i++)
    {
        if (prog_hdrs[i].p_type != PT_LOAD)
        {
            IF_DEBUG LOG ("Section %u is not loadable. Skipping.", i);
            continue;
        }
        length = prog_hdrs[i].p_memsz;
        length = (length + 0xFFFFF) & ~0xFFFFF; // Align to 1MB
        if (prog_hdrs[i].p_flags & PF_X == PF_X) // executable section
        {
            if (base_address == NULL)
            {
                base_address = prog_hdrs[i].p_vaddr; // set this to first exe section
                // TODO: better way to find baseaddr
            }
            memblock = sceKernelAllocCodeMemBlock ("UVLHomebrew", length);
        }
        else // data section
        {
            memblock = sceKernelAllocMemBlock ("UVLHomebrew", 0xC20D060, length, NULL);
        }
        if (memblock < 0)
        {
            LOG ("Error allocating memory.");
            return -1;
        }
        if (sceKernelGetMemBlockBase (memblock, &blockaddr) < 0)
        {
            LOG ("Error getting memory block address.");
        }
        if ((u32_t)blockaddr != (u32_t)prog_hdrs[i].p_vaddr)
        {
            LOG ("Error, section %u wants to be loaded to 0x%08X but we allocated 0x%08X", i, (u32_t)prog_hdrs[i].p_vaddr, (u32_t)blockaddr);
        }
        IF_DEBUG LOG ("Allocated memory at 0x%08X, attempting to load section %u.", (u32_t)blockaddr, i);
        IF_DEBUG LOG ("Seeking program section.");
        if (sceIoLseek (fd, start_offset + prog_hdrs[i].p_offset, PSP2_SEEK_SET) < 0)
        {
            LOG ("Error seeking program section.");
            return -1;
        }
        IF_DEBUG LOG ("Reading program section.");
        if (sceIoRead (fd, blockaddr, prog_hdrs[i].p_filesz) < 0)
        {
            LOG ("Error reading program section.");
            return -1;
        }
        IF_DEBUG LOG ("Zeroing %u remainder of memory.", prog_hdrs[i].p_memsz - prog_hdrs[i].p_filesz);
        memset ((void*)((u32_t)blockaddr + prog_hdrs[i].p_filesz), 0, prog_hdrs[i].p_memsz - prog_hdrs[i].p_filesz);
    }

    // resolve NIDs
    module_imports_t *import;
    if (base_address == NULL)
    {
        LOG ("Cannot find base address for executable.");
        return -1;
    }
    IF_DEBUG LOG ("Resolving NID imports.");
    for (import = (module_imports_t*)((char*)base_address + mod_info.stub_top); (void*)import < (void*)((char*)base_address + mod_info.stub_end); import++)
    {
        IF_DEBUG LOG ("Loading required module %s.", import->lib_name);
        if (uvl_load_module (import->lib_name) < 0)
        {
            LOG ("Error loading required module: %s", import->lib_name);
            return -1;
        }
        for (i = 0; i < import->num_functions; i++)
        {
            IF_DEBUG LOG ("Trying to resolve function NID: 0x%08X from %s", import->func_nid_table[i], import->lib_name);
            if (uvl_resolve_stub (import->func_nid_table[i], import->func_entry_table[i], import->lib_name) < 0)
            {
                LOG ("Cannot resolve NID: 0x%08X. Continuing.", import->func_nid_table[i]);
                continue;
            }
        }
        for (i = 0; i < import->num_vars; i++)
        {
            IF_DEBUG LOG ("Trying to resolve variable NID: 0x%08X from %s", import->var_nid_table[i], import->lib_name);
            if (uvl_resolve_stub (import->var_nid_table[i], import->var_entry_table[i], import->lib_name) < 0)
            {
                LOG ("Cannot resolve NID: 0x%08X. Continuing.", import->var_nid_table[i]);
                continue;
            }
        }
        for (i = 0; i < import->num_tls_vars; i++)
        {
            IF_DEBUG LOG ("Trying to resolve tls NID: 0x%08X from %s", import->tls_nid_table[i], import->lib_name);
            if (uvl_resolve_stub (import->tls_nid_table[i], import->tls_entry_table[i], import->lib_name) < 0)
            {
                LOG ("Cannot resolve NID: 0x%08X. Continuing.", import->tls_nid_table[i]);
                continue;
            }
        }
    }
// old resolve code
#if 0
        IF_DEBUG LOG ("Queuing required importes for module.");
        if (uvl_add_unresolved_imports (import) < 0)
        {
            LOG ("Failed to add imports for module: %s", import->lib_name);
            return -1;
        }
    }
    IF_DEBUG LOG ("Adding modules' exports to resolve table.");
    if (uvl_resolve_all_loaded_modules (RESOLVE_MOD_EXPS) < 0)
    {
        LOG ("Failed to resolve module exports.");
        return -1;
    }
    IF_DEBUG LOG ("Attempting to resolve homebrew.");
    if (uvl_resolve_all_unresolved () < 0)
    {
        LOG ("Failed to resolve app imports.");
        return -1;
    }
#endif
    *entry = (void*)((u32_t)base_address + (u32_t)elf_hdr.e_entry);
    return 0;
}

/********************************************//**
 *  \brief Loads a system module by name
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_load_module (char *name) ///< Name of module to load
{
    // TODO: Get filename for mod name and load module
}

/********************************************//**
 *  \brief Validates ELF header
 *  
 *  Makes sure the ELF is recognized by the 
 *  Vita's architecture.
 *  \returns Zero if valid, otherwise invalid
 ***********************************************/
int 
uvl_elf_check_header (Elf32_Ehdr_t *hdr) ///< ELF header to check
{
    // magic number
    if (!(hdr->e_ident[EI_MAG0] == ELFMAG0 && hdr->e_ident[EI_MAG1] == ELFMAG1 && hdr->e_ident[EI_MAG2] == ELFMAG2 && hdr->e_ident[EI_MAG3] == ELFMAG3))
    {
        LOG ("Invalid ELF magic number.");
        return -1;
    }
    // class
    if (!(hdr->e_ident[EI_CLASS] == ELFCLASS32))
    {
        LOG ("Not a 32bit executable.");
        return -1;
    }
    // data
    if (!(hdr->e_ident[EI_DATA] == ELFDATA2LSB))
    {
        LOG ("Not a valid ARM executable.");
        return -1;
    }
    // version
    if (!(hdr->e_ident[EI_VERSION] == EV_CURRENT))
    {
        LOG ("Unsupported ELF version.");
        return -1;
    }
    // type
    if (!(hdr->e_type == ET_EXEC))
    {
        LOG ("Only ET_EXEC files can be loaded currently.");
        return -1;
    }
    // machine
    if (!(hdr->e_machine == EM_ARM))
    {
        LOG ("Not an ARM executable.");
        return -1;
    }
    // version
    if (!(hdr->e_version == EV_CURRENT))
    {
        LOG ("Unsupported ELF version.");
        return -1;
    }
    // contains headers
    if (!(hdr->e_shoff > 0 && hdr->e_phoff > 0))
    {
        LOG ("Missing table header(s).");
        return -1;
    }
    // contains strings
    if (!(hdr->e_shstrndx > 0))
    {
        LOG ("Missing strings table.");
        return -1;
    }
    return 0;
}

/********************************************//**
 *  \brief Finds SCE module info
 *  
 *  This function locates the strings table 
 *  and finds the section where the module 
 *  information resides. Then it reads the 
 *  module information. This function will 
 *  move the pointer in the file descriptor.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_elf_get_module_info (PsvUID fd,             ///< File descriptor for the ELF
                         PsvOff start_offset,   ///< Starting position of the ELF in file
                   Elf32_Ehdr_t *elf_hdr,       ///< ELF header
                  module_info_t *mod_info)      ///< Where to read information to
{
    Elf32_Shdr_t sec_hdr;
    // find strings table
    IF_DEBUG LOG ("Seeking to strings table.");
    if (sceIoLseek (fd, start_offset + elf_hdr->e_shoff + elf_hdr->e_shstrndx * elf_hdr->e_shentsize, PSP2_SEEK_SET) < 0)
    {
        LOG ("Error seeking strings table.");
        return -1;
    }
    IF_DEBUG LOG ("Reading strings table header.");
    if (sceIoRead (fd, &sec_hdr, sizeof (Elf32_Shdr_t)) < sizeof (Elf32_Shdr_t))
    {
        LOG ("Error reading strings table header.");
        return -1;
    }
    char strings[sec_hdr.sh_size];
    int name_idx;
    IF_DEBUG LOG ("Seeking strings table.");
    if (sceIoLseek (fd, start_offset + sec_hdr.sh_offset, PSP2_SEEK_SET) < 0)
    {
        LOG ("Error seeking strings table.");
        return -1;
    }
    IF_DEBUG LOG ("Reading strings table.");
    if (sceIoRead (fd, strings, sec_hdr.sh_size) < 0)
    {
        LOG ("Error reading strings table.");
        return -1;
    }
    name_idx = strstr (strings, UVL_SEC_MODINFO) - strings;
    if (name_idx <= 0)
    {
        LOG ("Cannot find section %s in string table.", UVL_SEC_MODINFO);
        return -1;
    }
    IF_DEBUG LOG ("Found section %s.", name_idx);
    // find sceModuleInfo section
    int i;
    IF_DEBUG LOG ("Seeking sections table.");
    if (sceIoLseek (fd, start_offset + elf_hdr->e_shoff, PSP2_SEEK_SET) < 0)
    {
        LOG ("Error seeking section table.");
        return -1;
    }
    IF_DEBUG LOG ("Reading %u sections.", elf_hdr->e_shnum);
    for (i = 0; i < elf_hdr->e_shnum; i++)
    {
        IF_DEBUG LOG ("Reading section header.");
        if (sceIoRead (fd, &sec_hdr, sizeof (Elf32_Shdr_t)) < sizeof (Elf32_Shdr_t))
        {
            LOG ("Error reading section header.");
            return -1;
        }
        if (sec_hdr.sh_name == name_idx) // we want this section
        {
            IF_DEBUG LOG ("Found requested section %s.", name_idx);
            IF_DEBUG LOG ("Seeking to section.");
            if (sceIoLseek (fd, start_offset + sec_hdr.sh_offset, PSP2_SEEK_SET) < 0)
            {
                LOG ("Error seeking sceModuleInfo table.");
                return -1;
            }
            IF_DEBUG LOG ("Reading section.");
            if (sceIoRead (fd, mod_info, sec_hdr.sh_size) < 0)
            {
                LOG ("Error reading sceModuleInfo.");
                return -1;
            }
            return 0;
        }
    }
    return -1;
}

/********************************************//**
 *  \brief Frees memory of where we want to load
 *  
 *  Finds the max and min addresses we want to
 *  load to using program headers and frees 
 *  any module taking up those spaces.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int
uvl_elf_free_memory (Elf32_Phdr_t *prog_hdrs,   ///< Array of program headers
                              int count)        ///< Number of program headers
{
    void *min_addr = (void*)0x00000000;
    void *max_addr = (void*)0xFFFFFFFF;
    loaded_module_info_t m_mod_info;
    PsvUID mod_list[MAX_LOADED_MODS];
    u32_t num_loaded = MAX_LOADED_MODS;
    int i, j;
    u32_t length;
    int temp[2];

    IF_DEBUG LOG ("Reading %u program headers.", count);
    for (i = 0; i < count; i++)
    {
        if (prog_hdrs[i].p_vaddr > min_addr)
        {
            min_addr = prog_hdrs[i].p_vaddr;
        }
        if ((u32_t)prog_hdrs[i].p_vaddr + prog_hdrs[i].p_memsz < (u32_t)max_addr)
        {
            max_addr = (void*)((u32_t)prog_hdrs[i].p_vaddr + prog_hdrs[i].p_memsz);
        }
    }
    IF_DEBUG LOG ("Lowest load address: 0x%08X, highest: 0x%08X", (u32_t)min_addr, (u32_t)max_addr);\

    IF_DEBUG LOG ("Getting list of loaded modules.");
    if (sceKernelGetModuleList (0xFF, mod_list, &num_loaded) < 0)
    {
        LOG ("Failed to get module list.");
        return -1;
    }
    IF_DEBUG LOG ("Found %u loaded modules.", num_loaded);
    for (i = 0; i < num_loaded; i++)
    {
        m_mod_info.size = sizeof (loaded_module_info_t); // should be 440
        IF_DEBUG LOG ("Getting information for module #%u, UID: 0x%X.", i, mod_list[i]);
        if (sceKernelGetModuleInfo (mod_list[i], &m_mod_info) < 0)
        {
            LOG ("Error getting info for mod 0x%08X, continuing", mod_list[i]);
            continue;
        }
        for (j = 0; j < 3; j++)
        {
            if (m_mod_info.segments[j].vaddr > min_addr || (u32_t)m_mod_info.segments[j].vaddr + m_mod_info.segments[j].memsz > (u32_t)min_addr)
            {
                IF_DEBUG LOG ("Module %s segment %u (0x%08X, size %u) is in our address space. Attempting to unload.", m_mod_info.module_name, j, (u32_t)m_mod_info.segments[j].vaddr, m_mod_info.segments[j].memsz);
                if (sceKernelStopUnloadModule (mod_list[i], 0, 0, 0, &temp[0], &temp[1]) < 0)
                {
                    LOG ("Error unloading %s.", m_mod_info.module_name);
                    return -1;
                }
                break;
            }
        }
    }
    return 0;
}
