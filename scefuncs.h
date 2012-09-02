#ifndef UVL_SCEFUNCS
#define UVL_SCEFUNCS

#ifdef GENERATE_STUBS
#define STUB_FUNCTION (type, name) \
    type __attribute__((naked, aligned(16))) name (...) \
    { \
        __asm__ ("mov r12, #0\n" \
                 "movt r12, #0\n" \
                 "bx r12\n" \
                 "nop"); \
    }
#else
#define STUB_FUNCTION (name) void __attribute__((naked)) name (...)
#endif

STUB_FUNCTION (int, sceIoOpen);
STUB_FUNCTION (int, sceIoClose);

#endif
