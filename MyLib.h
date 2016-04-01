#ifndef MYLIB_H
#define MYLIB_H

#define BITS_PER_BYTE 8
#define TRUE 1
#define FALSE 0
#define BITS_PER_INT 32
#define STRING_SIZE 1024

typedef unsigned char UChar;
typedef unsigned long ULong;
typedef unsigned int UInt;
typedef unsigned short UShort;

#ifdef LITTLE_ENDIAN

#define SwapULong(val) (val << 24 | (val << 8 & 0xFF0000) \
 | (val >> 8 & 0xFF00) | val >> 24 & 0xFF)
#define SwapUShort(val) (val << BITS_PER_BYTE | val >> BITS_PER_BYTE)

#else

#define SwapULong(val) (val)
#define SwapUShort(val) (val)

#endif

#endif

