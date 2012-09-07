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
    SceUID fd = sceIoOpen (filename, PSP2_O_RDONLY, PSP2_STM_RU);
    if (fd < 0)
    {
        LOG ("Cannot open executable file for reading.");
        return -1;
    }

    IF_DEBUG LOG ("Reading magic number.");
    if (sceIoRead (fd, magic, MAGIC_LEN) < MAGIC_LEN)
    {
        LOG ("Cannot read magic number.");
        return -1;
    }

     if (magic[0] == ELFMAG0 && magic[1] == ELFMAG1 && magic[2] == ELFMAG2 && magic[3] == ELFMAG3)
     {
            IF_DEBUG LOG ("Found a ELF, loading.");
            return uvl_load_elf (fd, 0, entry);
     }

     if (magic[0] == SCEMAG0 && magic[1] == SCEMAG1 && magic[2] == SCEMAG2 && magic[3] == SCEMAG3)
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
uvl_load_elf (SceUID fd,            ///< File descriptor for ELF
              SceOff start_offset,  ///< Starting position of the ELF in file
                void **entry)       ///< Returned pointer to entry pointer
{
    Elf32_Ehdr_t elf_hdr;
    module_info_t mod_info;
    Elf32_Phdr_t prog_hdr;
    void *base_address = (void*)(u32_t)-1;
    module_imports_t *import;
    int i;

    *entry = NULL;
    // get headers
    IF_DEBUG LOG ("Reading headers.");
    if (sceIoRead (fd, &elf_hdr, sizeof (Elf32_Ehdr_t)) < sizeof (Elf32_Ehdr_t))
    {
        LOG ("Error reading ELF header.");
        return -1;
    }
    IF_DEBUG LOG ("Checking headers.");
    if (uvl_check_elf_header (&elf_hdr) < 0)
    {
        LOG ("Check header failed.");
        return -1;
    }
    // get mod_info
    IF_DEBUG LOG ("Getting module info.");
    if (uvl_get_module_info (fd, start_offset, &elf_hdr, &mod_info) < 0)
    {
        LOG ("Cannot find module info section.");
        return -1;
    }
    // actually load the ELF
    IF_DEBUG LOG ("Found %u program sections.", elf_hdr.e_phnum);
    for (i = 0; i < elf_hdr.e_phnum; i++)
    {
        IF_DEBUG LOG ("Seeking program header #%u.", i);
        if (sceIoLseek (fd, start_offset + elf_hdr.e_phoff + i * elf_hdr.e_phentsize, PSP2_SEEK_SET) < 0)
        {
            LOG ("Error seeking program header.");
            return -1;
        }
        IF_DEBUG LOG ("Reading program header.");
        if (sceIoRead (fd, &prog_hdr, elf_hdr.e_phentsize) < elf_hdr.e_phentsize)
        {
            LOG ("Error reading program header.");
            return -1;
        }
        if (prog_hdr.p_vaddr < base_address) // lowest number is base address
        {
            IF_DEBUG LOG ("Possible base address: 0x%X.", (u32_t)base_address);
            base_address = prog_hdr.p_vaddr;
        }
        // TODO: Alloc memory to load and set permissions
        IF_DEBUG LOG ("Seeking to program.");
        if (sceIoLseek (fd, start_offset + prog_hdr.p_offset, PSP2_SEEK_SET) < 0)
        {
            LOG ("Error seeking to section %u.", i);
            return -1;
        }
        IF_DEBUG LOG ("Loading program to memory at 0x%X.", (u32_t)prog_hdr.p_vaddr);
        if (sceIoRead (fd, prog_hdr.p_vaddr, prog_hdr.p_filesz) < 0)
        {
            LOG ("Error reading program section %u.", i);
            return -1;
        }
        if (prog_hdr.p_memsz > prog_hdr.p_filesz)
        {
            // specs say we have to zero out extra bytes
            IF_DEBUG LOG ("Adding %u bytes of padding.", prog_hdr.p_memsz - prog_hdr.p_filesz);
            memset ((char*)prog_hdr.p_vaddr + prog_hdr.p_filesz, 0, prog_hdr.p_memsz - prog_hdr.p_filesz);
        }
    }
    // resolve NIDs
    IF_DEBUG LOG ("Resolving NID imports.");
    for (import = (module_imports_t*)((char*)base_address + mod_info.stub_top); (void*)import < (void*)((char*)base_address + mod_info.stub_end); import++)
    {
        IF_DEBUG LOG ("Loading required module %s.", import->lib_name);
        if (uvl_load_module (import->lib_name) < 0)
        {
            LOG ("Error loading required module: %s", import->lib_name);
            return -1;
        }
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
uvl_check_elf_header (Elf32_Ehdr_t *hdr) ///< ELF header to check
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
uvl_get_module_info (SceUID fd,             ///< File descriptor for the ELF
                     SceOff start_offset,   ///< Starting position of the ELF in file
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
    name_idx = strstr (strings, SEC_MODINFO) - strings;
    if (name_idx <= 0)
    {
        LOG ("Cannot find section %s in string table.", SEC_MODINFO);
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
