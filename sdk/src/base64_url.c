/**
 * Implementation of base64 is based off of the MBed library.
 */
#include <mbedtls/base64.h>
#include <stdint.h>
#include "dslink/base64_url.h"

static const unsigned char base64_enc_map_[64] =
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
    'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '-', '_'
};

static const unsigned char base64_dec_map_[128] =
{
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127,  62, 127, 127,  52,  53,
    54,   55,  56,  57,  58,  59,  60,  61, 127, 127,
    127,  64, 127, 127, 127,   0,   1,   2,   3,   4,
    5,     6,   7,   8,   9,  10,  11,  12,  13,  14,
    15,   16,  17,  18,  19,  20,  21,  22,  23,  24,
    25,  127, 127, 127, 127,  63, 127,  26,  27,  28,
    29,   30,  31,  32,  33,  34,  35,  36,  37,  38,
    39,   40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,   50,  51, 127, 127, 127, 127, 127
};

#define BASE64_SIZE_T_MAX ( (size_t) -1 ) /* SIZE_T_MAX is not standard */

int dslink_base64_url_encode(unsigned char *dst,
                             size_t dLen,
                             size_t *oLen,
                             const unsigned char *src,
                             size_t sLen) {
    size_t i, n;
    int C1, C2, C3;
    unsigned char *p;

    if(sLen == 0 )
    {
        *oLen = 0;
        return( 0 );
    }

    n = sLen / 3 + (sLen % 3 != 0 );

    if( n > ( BASE64_SIZE_T_MAX - 1 ) / 4 )
    {
        *oLen = BASE64_SIZE_T_MAX;
        return( MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL );
    }

    n *= 4;

    if(dLen < n + 1 )
    {
        *oLen = n + 1;
        return( MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL );
    }

    n = (sLen / 3 ) * 3;

    for( i = 0, p = dst; i < n; i += 3 )
    {
        C1 = *src++;
        C2 = *src++;
        C3 = *src++;

        *p++ = base64_enc_map_[(C1 >> 2) & 0x3F];
        *p++ = base64_enc_map_[(((C1 &  3) << 4) + (C2 >> 4)) & 0x3F];
        *p++ = base64_enc_map_[(((C2 & 15) << 2) + (C3 >> 6)) & 0x3F];
        *p++ = base64_enc_map_[C3 & 0x3F];
    }

    if(i < sLen)
    {
        C1 = *src++;
        C2 = (( i + 1 ) < sLen) ? *src++ : 0;

        *p++ = base64_enc_map_[(C1 >> 2) & 0x3F];
        *p++ = base64_enc_map_[(((C1 & 3) << 4) + (C2 >> 4)) & 0x3F];

        if(( i + 1 ) < sLen)
            *p++ = base64_enc_map_[((C2 & 15) << 2) & 0x3F];
    }

    *oLen = p - dst;
    *p = 0;

    return( 0 );
}

int dslink_base64_url_decode(unsigned char *dst,
                             size_t dLen,
                             size_t *oLen,
                             const unsigned char *src,
                             size_t sLen) {
    size_t alignedLen = (((sLen + 3) / 4) * 4);
    size_t i, n;
    uint32_t j, x;
    unsigned char *p;

    /* First pass: check for validity and get output length */
    for( i = n = j = 0; i < sLen; i++ )
    {
        /* Skip spaces before checking for EOL */
        x = 0;
        while(i < sLen && src[i] == ' ' )
        {
            ++i;
            ++x;
        }

        /* Spaces at end of buffer are OK */
        if(i == sLen)
            break;

        if((sLen - i ) >= 2 &&
            src[i] == '\r' && src[i + 1] == '\n' )
            continue;

        if( src[i] == '\n' )
            continue;

        /* Space inside a line is an error */
        if( x != 0 )
            return( MBEDTLS_ERR_BASE64_INVALID_CHARACTER );

        if( src[i] == '.' && ++j > 2 )
            return( MBEDTLS_ERR_BASE64_INVALID_CHARACTER );

        if( src[i] > 127 || base64_dec_map_[src[i]] == 128 ) {
            return (MBEDTLS_ERR_BASE64_INVALID_CHARACTER);
        }

        if( base64_dec_map_[src[i]] < 64 && j != 0 )
            return( MBEDTLS_ERR_BASE64_INVALID_CHARACTER );

        n++;
    }

    if( n == 0 )
    {
        *oLen = 0;
        return( 0 );
    }

    n = ( ( n * 6 ) + 7 ) >> 3;
    n -= j;

    if( dst == NULL || dLen < n )
    {
        *oLen = n;
        return( MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL );
    }

    size_t diff = alignedLen - sLen;
    i += diff;
    for( j = 3, n = x = 0, p = dst; i > 0; i--, src++ )
    {
        if( *src == '\r' || *src == '\n' || *src == ' ' )
            continue;

        j -= ( base64_dec_map_[*src] == 64 );
        x  = ( x << 6 ) | ( base64_dec_map_[*src] & 0x3F );

        if( ++n == 4 )
        {
            n = 0;
            if( j > 0 ) *p++ = (unsigned char)( x >> 16 );
            if( j > 1 ) *p++ = (unsigned char)( x >>  8 );
            if( j > 2 ) *p++ = (unsigned char)( x       );
        }
    }

    *oLen = p - dst - diff;

    return( 0 );
}
