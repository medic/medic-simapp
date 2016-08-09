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

#ifndef __MUVUKU_UTIL_H__
#define __MUVUKU_UTIL_H__

#include "bladox.h"
#include "prototype.h"

#ifndef FALSE
    #define FALSE (0)
#endif

#ifndef TRUE
    #define TRUE (!(FALSE))
#endif

#define scalar_min(x, y) ((x) < (y) ? (x) : (y))
#define scalar_max(x, y) ((x) > (y) ? (x) : (y))

size_t safe_add(const size_t a, const size_t b, int *rv);

size_t safe_subtract(const size_t a, const size_t b, int *rv);

size_t safe_multiply(const size_t a, const size_t b, int *rv);

void memzero(void *p, size_t n);

void *xmalloc(size_t n);

void muvuku_panic(const char *detail);

char *ucs2_slv_encode(char *ucs2_str, u8 len);

char *ucs2_slv_to_gsm(char *str);

u8 is_numeric_string(char *p);

void display_hex16(u16 val);


#endif /* __MUVUKU_UTIL_H__ */

