#ifndef UVL_SCEFUNCS
#define UVL_SCEFUNCS

#include "types.h"

/** \name SCE IO Constants
 *  @{
 */

/* flags */
#define SCE_FREAD       (0x0001)  /* readable */
#define SCE_FWRITE      (0x0002)  /* writable */
#define SCE_FNBLOCK     (0x0004)  /*   Reserved: non-blocking reads */
#define SCE_FDIRO       (0x0008)  /* internal use for dopen */
#define SCE_FRLOCK      (0x0010)  /*   Reserved: read locked (non-shared) */
#define SCE_FWLOCK      (0x0020)  /*   Reserved: write locked (non-shared) */
#define SCE_FAPPEND     (0x0100)  /* append on each write */
#define SCE_FCREAT      (0x0200)  /* create if nonexistant */
#define SCE_FTRUNC      (0x0400)  /* truncate to zero length */
#define SCE_EXCL        (0x0800)  /* exclusive create */
#define SCE_FSCAN       (0x1000)  /*   Reserved: scan type */
#define SCE_FRCOM       (0x2000)  /*   Reserved: remote command entry */
#define SCE_FNBUF       (0x4000)  /* no device buffer */
#define SCE_FASYNC      (0x8000)  /*   Reserved: asyncronous i/o */
#define SCE_FFDEXCL     (0x01000000)        /* exclusive access */
#define SCE_FGAMEDATA   (0x40000000)

/* Flag for sceIoOpen() */
#define SCE_O_RDONLY    (SCE_FREAD)
#define SCE_O_WRONLY    (SCE_FWRITE)
#define SCE_O_RDWR      (SCE_FREAD|SCE_FWRITE)
#define SCE_O_NBLOCK    (SCE_FNBLOCK) /*   Reserved: Non-Blocking I/O */
#define SCE_O_APPEND    (SCE_FAPPEND) /* append (writes guaranteed at the end) */
#define SCE_O_CREAT     (SCE_FCREAT)  /* open with file create */
#define SCE_O_TRUNC     (SCE_FTRUNC)  /* open with truncation */
#define SCE_O_EXCL      (SCE_EXCL)    /* exclusive create */
#define SCE_O_NOBUF     (SCE_FNBUF)   /* no device buffer */
#define SCE_O_NOWAIT    (SCE_FASYNC)  /*   Reserved: asyncronous i/o */
#define SCE_O_FDEXCL    (SCE_FFDEXCL) /* exclusive access */
#define SCE_O_PWLOCK    (SCE_FPWLOCK) /* power control lock */
#define SCE_O_FGAMEDATA (SCE_FGAMEDATA)

/* Flag for sceIoLseek() */
#define SCE_SEEK_SET    (0)
#define SCE_SEEK_CUR    (1)
#define SCE_SEEK_END    (2)

/* Filetypes and Protection bits */
#define SCE_STM_FMT         (0xf  <<  12)
#define SCE_STM_FLNK        (0x4  <<  12)   /* symbolic link */
#define SCE_STM_FREG        (0x2  <<  12)   /* regular file */
#define SCE_STM_FDIR        (0x1  <<  12)   /* directory */
#define SCE_STM_ISLNK(m)    (((m) & SCE_STM_FMT) == SCE_STM_FLNK)
#define SCE_STM_ISREG(m)    (((m) & SCE_STM_FMT) == SCE_STM_FREG)
#define SCE_STM_ISDIR(m)    (((m) & SCE_STM_FMT) == SCE_STM_FDIR)

#define SCE_STM_RWXU        00700
#define SCE_STM_RUSR        00400
#define SCE_STM_WUSR        00200
#define SCE_STM_XUSR        00100
#define SCE_STM_RWU         (SCE_STM_RUSR|SCE_STM_WUSR)
#define SCE_STM_RU          (SCE_STM_RUSR)
/** @}*/

#ifdef GENERATE_STUBS
#define STUB_FUNCTION(type, name) \
    type __attribute__((naked, aligned(16))) name () \
    { \
        __asm__ ("mov r12, #0\n" \
                 "movt r12, #0\n" \
                 "bx r12\n" \
                 "nop"); \
    }
#else
#define STUB_FUNCTION(type, name) type __attribute__((naked)) name ()
#endif

STUB_FUNCTION(SceUID, sceIoOpen);
STUB_FUNCTION(int, sceIoClose);
STUB_FUNCTION(SceOff, sceIoLseek);
STUB_FUNCTION(SceOff, sceIoRead);
STUB_FUNCTION(int, sceKernelGetModuleList);
STUB_FUNCTION(int, sceKernelGetModuleInfo);
STUB_FUNCTION(SceUID, sceKernelCreateThread);
STUB_FUNCTION(int, sceKernelStartThread);
STUB_FUNCTION(int, sceKernelExitDeleteThread);
STUB_FUNCTION(int, sceKernelStopUnloadModule);

void uvl_scefuncs_resolve_all ();

#endif
