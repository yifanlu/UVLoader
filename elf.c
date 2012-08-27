#include "scefuncs.h"
#include "strings.h"
#include "elf.h"

int uvl_load_exe (const char *filename)
{
    char magic[4];
    SceUID fd = sceIoOpen (filename, SCE_O_RDONLY, SCE_STM_RU);
    if (fd < 0)
    {
        LOG ("Cannot open executable file for loading.");
        return -1;
    }

    if (sceIoRead (fd, magic, MAGIC_LEN) < MAGIC_LEN)
    {
        LOG ("Cannot read magic number.");
        return -1;
    }

    if (magic[0] == ELFMAG0)
    {
        if (magic[1] == ELFMAG1 && magic[2] == ELFMAG2 && magic[3] == ELFMAG3)
        {
            return uvl_load_elf (fd, 0);
        }
    }
    else if (magic[0] == SCEMAG0)
    {
        if (magic[1] == ELFMAG1 && magic[2] == ELFMAG2 && magic[3] == ELFMAG3)
        {
            if (sceIoLseek (fd, SCEHDR_LEN, SCE_SEEK_SET) < 0)
            {
                LOG ("Cannot skip SCE header.");
                return -1;
            }
            return uvl_load_elf (fd, SCEHDR_LEN);
        }
    }

    LOG ("Invalid magic.");
    return -1;
}

int uvl_load_elf (SceUID fd, SceOff start_offset)
{
    Elf32_Ehdr_t elf_hdr;
    module_info_t mod_info;
    int i;
    // get headers
    if (sceIoRead (fd, &elf_hdr, sizeof (Elf32_Ehdr_t)) < sizeof (Elf32_Ehdr_t))
    {
        LOG ("Error reading ELF header.");
        return -1;
    }
    if (uvl_check_elf_header (&elf_hdr) < 0)
    {
        LOG ("Check header failed.");
        return -1;
    }
    // get mod_info
    if (uvl_get_module_info (fd, start_offset, &elf_hdr, &mod_info) < 0)
    {
        LOG ("Cannot find module info section.");
        return -1;
    }
}

int uvl_check_elf_header (Elf32_Ehdr_t *hdr)
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

int uvl_get_module_info (SceUID off, SceOff start_offset, Elf32_Ehdr_t *elf_hdr, module_info_t *mod_info)
{
    Elf32_Shdr_t sec_hdr;
    // find strings table
    if (sceIoLseek (fd, start_offset + elf_hdr->e_shoff + elf_hdr->e_shstrndx * elf_hdr->e_shentsize, SCE_SEEK_SET) < 0)
    {
        LOG ("Error seeking strings table.");
        return -1;
    }
    if (sceIoRead (fd, &sec_hdr, sizeof (Elf32_Shdr_t)) < sizeof (Elf32_Shdr_t))
    {
        LOG ("Error reading strings table header.");
        return -1;
    }
    char strings[sec_hdr.sh_size];
    int name_idx;
    if (sceIoLseek (fd, start_offset + sec_hdr.sh_offset, SCE_SEEK_SET) < 0)
    {
        LOG ("Error seeking strings table.");
        return -1;
    }
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
    // find sceModuleInfo section
    int i;
    if (sceIoLseek (fd, start_offset + elf_hdr->e_shoff, SCE_SEEK_SET) < 0)
    {
        LOG ("Error seeking section table.");
        return -1;
    }
    for (i = 0; i < elf_hdr.e_shnum; i++)
    {
        if (sceIoRead (fd, &sec_hdr, sizeof (Elf32_Shdr_t)) < sizeof (Elf32_Shdr_t))
        {
            LOG ("Error reading section header.");
            return -1;
        }
        if (sec_hdr.sh_name == name_idx) // we want this section
        {
            if (sceIoLseek (fd, start_offset + sec_hdr.sh_offset, SCE_SEEK_SET) < 0)
            {
                LOG ("Error seeking sceModuleInfo table.");
                return -1;
            }
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
