/**
 * Muvuku: An STK data collection framework
 *
 * Copyright 2011-2012 Medic Mobile, Inc. <hello@medicmobile.org>
 * All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL MEDIC MOBILE BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MUVUKU_PROTOTYPE_H_
#define _MUVUKU_PROTOTYPE_H_

#ifdef _MUVUKU_PROTOTYPE

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Basic type definitions:
    Simulate the Bladox environment without including any
    hardware-specific headers or incorrect type definitions. */

#undef NULL
#include "../include/bladox/types.h"

#undef PROGMEM
#define PROGMEM

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

typedef int8_t b8;
typedef int16_t b16;
typedef int32_t b32;

typedef struct _lc_char
{
    u8 lang;
    u8 len;
    const u8 s[];

} lc_char;

typedef struct _lc_list
{
    u8 lang;
    const u8 * s;

} lc_list;


/* Stubs for unsupported functions:
    We don't support STK-specific or Bladox-specific functionality
    when in prototyping mode -- just enough to build `schema` and
    allow for prototyping of string/data manipulation functions. */

#define str2msisdn(s, type, mem) \
        ((char *) NULL)

#define msisdn2str(msisdn, type, mem) \
        ((char *) NULL)


#endif /* _MUVUKU_PROTOTYPE */
#endif /* _MUVUKU_PROTOTYPE_H_ */

