#ifndef _LOS_MIP_TYPED_H
#define _LOS_MIP_TYPED_H

#include <stdint.h>

#if defined (__CC_ARM)

#define PACK_STRUCT_BEGIN __packed
#define PACK_STRUCT_STRUCT 
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x


#elif defined (__GNUC__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#endif



typedef uint8_t   u8_t;
typedef int8_t    s8_t;
typedef uint16_t  u16_t;
typedef int16_t   s16_t;
typedef uint32_t  u32_t;
typedef int32_t   s32_t;

/* for unsigned short */
#define MIP_HTONS(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define MIP_NTOHS(x) MIP_HTONS(x)

/* for unsigned long */
#define MIP_HTONL(x) ((((x) & 0xff) << 24) | \
                     (((x) & 0xff00) << 8) | \
                     (((x) & 0xff0000UL) >> 8) | \
                     (((x) & 0xff000000UL) >> 24))
#define MIP_NTOHL(x) MIP_HTONL(x)


#define MIP_TRUE 1
#define MIP_FALSE 0


#endif
