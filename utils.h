#ifndef UVL_UTILS
#define UVL_UTILS

void* memcpy (void *destination, const void *source, u32_t num);
char* strcpy (char *destination, const char *source);
int memcmp (const void *ptr1, const void *ptr2, u32_t num);
int strcmp (const char *str1, const char *str2);
void* memset (void *ptr, int value, u32_t num);
u32_t strlen (const char *str);
char* memstr (char *needle, int n_length, char *haystack, int h_length);

#endif
