#ifndef UVL_RESOLVE
#define UVL_RESOLVE

#define RESOLVE_TYPE_UNKNOWN    0
#define RESOLVE_TYPE_FUNCTION   1
#define RESOLVE_TYPE_SYSCALL    2
#define RESOLVE_TYPE_VARIABLE   3

#define INSTRUCTION_UNKNOWN     0
#define INSTRUCTION_MOV         1
#define INSTRUCTION_MOVT        2
#define INSTRUCTION_SYSCALL     3
#define INSTRUCTION_BRANCH      4
#define STUB_FUNC_MAX_LEN       16

typedef struct resolve_entry
{
    u32_t   nid;
    u16_t   type;
    u16_t   resolved;
    union value
    {
        u32_t   value;
        u32_t   ptr;
        u32_t   func_ptr;
        u32_t   syscall;
    };
} resolve_entry_t;

#endif
