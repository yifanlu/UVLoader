#include "utils.h"

// Below is stolen from Android's Bionic

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

 /*
 * sizeof(word) MUST BE A POWER OF TWO
 * SO THAT wmask BELOW IS ALL ONES
 */
typedef long word;      /* "word" used for optimal copy speed */

#define wsize   sizeof(word)
#define wmask   (wsize - 1)

/*
 * Copy a block of memory, handling overlap.
 * This is the routine that actually implements
 * (the portable versions of) bcopy, memcpy, and memmove.
 */
void *
memcpy(void *dst0, const void *src0, size_t length)
{
    char *dst = dst0;
    const char *src = src0;
    size_t t;

    if (length == 0 || dst == src)      /* nothing to do */
        goto done;

    /*
     * Macros: loop-t-times; and loop-t-times, t>0
     */
#define TLOOP(s) if (t) TLOOP1(s)
#define TLOOP1(s) do { s; } while (--t)

    if ((unsigned long)dst < (unsigned long)src) {
        /*
         * Copy forward.
         */
        t = (long)src;  /* only need low bits */
        if ((t | (long)dst) & wmask) {
            /*
             * Try to align operands.  This cannot be done
             * unless the low bits match.
             */
            if ((t ^ (long)dst) & wmask || length < wsize)
                t = length;
            else
                t = wsize - (t & wmask);
            length -= t;
            TLOOP1(*dst++ = *src++);
        }
        /*
         * Copy whole words, then mop up any trailing bytes.
         */
        t = length / wsize;
        TLOOP(*(word *)dst = *(word *)src; src += wsize; dst += wsize);
        t = length & wmask;
        TLOOP(*dst++ = *src++);
    } else {
        /*
         * Copy backwards.  Otherwise essentially the same.
         * Alignment works as before, except that it takes
         * (t&wmask) bytes to align, not wsize-(t&wmask).
         */
        src += length;
        dst += length;
        t = (long)src;
        if ((t | (long)dst) & wmask) {
            if ((t ^ (long)dst) & wmask || length <= wsize)
                t = length;
            else
                t &= wmask;
            length -= t;
            TLOOP1(*--dst = *--src);
        }
        t = length / wsize;
        TLOOP(src -= wsize; dst -= wsize; *(word *)dst = *(word *)src);
        t = length & wmask;
        TLOOP(*--dst = *--src);
    }
done:
    return (dst0);
}

char *
strcpy(char *to, const char *from)
{
    char *save = to;

    for (; (*to = *from) != '\0'; ++from, ++to);
    return(save);
}

int memcmp(const void *s1, const void *s2, u32_t n)
{
    const unsigned char*  p1   = s1;
    const unsigned char*  end1 = p1 + n;
    const unsigned char*  p2   = s2;
    int                   d = 0;

    for (;;) {
        if (d || p1 >= end1) break;
        d = (int)*p1++ - (int)*p2++;

        if (d || p1 >= end1) break;
        d = (int)*p1++ - (int)*p2++;

        if (d || p1 >= end1) break;
        d = (int)*p1++ - (int)*p2++;

        if (d || p1 >= end1) break;
        d = (int)*p1++ - (int)*p2++;
    }
    return d;
}

/*
 * Compare strings.
 */
int
strcmp(const char *s1, const char *s2)
{
    while (*s1 == *s2++)
        if (*s1++ == 0)
            return (0);
    return (*(unsigned char *)s1 - *(unsigned char *)--s2);
}

void*  memset(void*  dst, int c, u32_t n)
{
    char*  q   = dst;
    char*  end = q + n;

    for (;;) {
        if (q >= end) break; *q++ = (char) c;
        if (q >= end) break; *q++ = (char) c;
        if (q >= end) break; *q++ = (char) c;
        if (q >= end) break; *q++ = (char) c;
    }

  return dst;
}

u32_t
strlen(const char *str)
{
    const char *s;

    for (s = str; *s; ++s)
        ;
    return (s - str);
}

// Below is stolen from http://en.wikipedia.org/wiki/Boyer%E2%80%93Moore_string_search_algorithm

#define ALPHABET_LEN 255
#define NOT_FOUND patlen
#define max(a, b) ((a < b) ? b : a)
 
// delta1 table: delta1[c] contains the distance between the last
// character of pat and the rightmost occurence of c in pat.
// If c does not occur in pat, then delta1[c] = patlen.
// If c is at string[i] and c != pat[patlen-1], we can
// safely shift i over by delta1[c], which is the minimum distance
// needed to shift pat forward to get string[i] lined up 
// with some character in pat.
// this algorithm runs in alphabet_len+patlen time.
void make_delta1(int *delta1, char *pat, int patlen) {
    int i;
    for (i=0; i < ALPHABET_LEN; i++) {
        delta1[i] = NOT_FOUND;
    }
    for (i=0; i < patlen-1; i++) {
        delta1[pat[i]] = patlen-1 - i;
    }
}
 
// true if the suffix of word starting from word[pos] is a prefix 
// of word
int is_prefix(char *word, int wordlen, int pos) {
    int i;
    int suffixlen = wordlen - pos;
    // could also use the strncmp() library function here
    for (i = 0; i < suffixlen; i++) {
        if (word[i] != word[pos+i]) {
            return 0;
        }
    }
    return 1;
}
 
// length of the longest suffix of word ending on word[pos].
// suffix_length("dddbcabc", 8, 4) = 2
int suffix_length(char *word, int wordlen, int pos) {
    int i;
    // increment suffix length i to the first mismatch or beginning
    // of the word
    for (i = 0; (word[pos-i] == word[wordlen-1-i]) && (i < pos); i++);
    return i;
}
 
// delta2 table: given a mismatch at pat[pos], we want to align 
// with the next possible full match could be based on what we
// know about pat[pos+1] to pat[patlen-1].
//
// In case 1:
// pat[pos+1] to pat[patlen-1] does not occur elsewhere in pat,
// the next plausible match starts at or after the mismatch.
// If, within the substring pat[pos+1 .. patlen-1], lies a prefix
// of pat, the next plausible match is here (if there are multiple
// prefixes in the substring, pick the longest). Otherwise, the
// next plausible match starts past the character aligned with 
// pat[patlen-1].
// 
// In case 2:
// pat[pos+1] to pat[patlen-1] does occur elsewhere in pat. The
// mismatch tells us that we are not looking at the end of a match.
// We may, however, be looking at the middle of a match.
// 
// The first loop, which takes care of case 1, is analogous to
// the KMP table, adapted for a 'backwards' scan order with the
// additional restriction that the substrings it considers as 
// potential prefixes are all suffixes. In the worst case scenario
// pat consists of the same letter repeated, so every suffix is
// a prefix. This loop alone is not sufficient, however:
// Suppose that pat is "ABYXCDEYX", and text is ".....ABYXCDEYX".
// We will match X, Y, and find B != E. There is no prefix of pat
// in the suffix "YX", so the first loop tells us to skip forward
// by 9 characters.
// Although superficially similar to the KMP table, the KMP table
// relies on information about the beginning of the partial match
// that the BM algorithm does not have.
//
// The second loop addresses case 2. Since suffix_length may not be
// unique, we want to take the minimum value, which will tell us
// how far away the closest potential match is.
void make_delta2(int *delta2, char *pat, int patlen) {
    int p;
    int last_prefix_index = patlen-1;
 
    // first loop
    for (p=patlen-1; p>=0; p--) {
        if (is_prefix(pat, patlen, p+1)) {
            last_prefix_index = p+1;
        }
        delta2[p] = last_prefix_index + (patlen-1 - p);
    }
 
    // second loop
    for (p=0; p < patlen-1; p++) {
        int slen = suffix_length(pat, patlen, p);
        if (pat[p - slen] != pat[patlen-1 - slen]) {
            delta2[patlen-1 - slen] = patlen-1 - p + slen;
        }
    }
}
 
char* boyer_moore (char *string, uint stringlen, char *pat, uint patlen) {
    int i;
    int delta1[ALPHABET_LEN];
    int delta2[patlen * sizeof(int)];
    make_delta1(delta1, pat, patlen);
    make_delta2(delta2, pat, patlen);
 
    i = patlen-1;
    while (i < stringlen) {
        int j = patlen-1;
        while (j >= 0 && (string[i] == pat[j])) {
            --i;
            --j;
        }
        if (j < 0) {
            return (string + i+1);
        }
 
        i += max(delta1[string[i]], delta2[j]);
    }
    return NULL;
}

/********************************************//**
 *  \brief Search for a string in memory
 *  
 *  Uses the Boyer-Moore algorithm to search. 
 *  \returns First occurrence of @a needle in 
 *  \returns @a haystack
 ***********************************************/
char* 
memstr (char *needle,   ///< String to find
         int n_length,  ///< Length of @a needle
        char *haystack, ///< Where to search
         int h_length)  ///< Length of @a haystack
{
    return boyer_moore (haystack, h_length, needle, n_length);
}

// thanks naehrwert for the tiny printf
static void _putn(char *str, uint x, int base, char fill, int fcnt, int upper)
{
    char buf[65];
    char *digits;
    char *p;
    int c = fcnt;

    if (upper)
        digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    else
        digits = "0123456789abcdefghijklmnopqrstuvwxyz"
    
    if(base > 36)
        return;

    p = buf + 64;
    *p = 0;
    do
    {
        c--;
        *--p = digits[x % base];
        x /= base;
    }while(x);
    
    if(fill != 0)
    {
        while(c > 0)
        {
            *--p = fill;
            c--;
        }
    }
    
    for(; *p != '\0'; *(str++) = *(p++));
}

int sprintf (char *str, const char *fmt, ...)
{
    va_list ap;
    char *s;
    char c, fill;
    int fcnt;
    uint n;
    
    va_start(ap, fmt);
    while(*fmt)
    {
        if(*fmt == '%')
        {
            fmt++;
            fill = 0;
            fcnt = 0;
            if((*fmt >= '0' && *fmt <= '9') || *fmt == ' ')
                if(*(fmt+1) >= '0' && *(fmt+1) <= '9')
                {
                    fill = *fmt;
                    fcnt = *(fmt+1) - '0';
                    fmt++;
                    fmt++;
                }
            switch(*fmt)
            {
            case 'c':
                c = va_arg(ap, uint);
                *(str++) = c;
                break;
            case 's':
                s = va_arg(ap, char *);
                for(; *s != '\0'; *(str++) = *(s++));
                break;
            case 'd':
                n = va_arg(ap, uint);
                _putn(str, n, 10, fill, fcnt, 0);
                break;
            case 'x':
                n = va_arg(ap, uint);
                _putn(str, n, 16, fill, fcnt, 0);
                break;
            case 'X':
                n = va_arg(ap, uint);
                _putn(str, n, 16, fill, fcnt, 1);
            case '%':
                *(str++) = '%';
                break;
            case '\0':
                goto out;
            default:
                *(str++) = '%';
                *(str++) = *fmt;
                break;
            }
        }
        else
            *(str++) = *fmt;
        fmt++;
    }

    out:
    *str = '\0';
    va_end(ap);
}
