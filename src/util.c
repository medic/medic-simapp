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

#include "bladox.h"
#include "prototype.h"

#include "util.h"

#include <stdlib.h>
#include <string.h>


const char PROGMEM panic_title[] = "Error";

const char PROGMEM panic_text[] = (
    "A hardware problem has occurred.\n"
    "Please power off and try again.\n\n"
    "If you see this message frequently, "
    "please replace your parallel SIM card."
);

const char PROGMEM panic_memory[] = "Memory allocation failed";


/**
 * @name memzero
 * Write `n` bytes of zeros, starting at offset `ptr`.
 * No loop unrolling: optimized for size, not for speed.
 */

void memzero(void *ptr, size_t n) {

    unsigned int i;
    u8 *p = (u8 *) ptr;

    for (i = 0; i < n; ++i) {
        p[i] = '\0';
    }
}

/**
 * @name safe_add
 * Add two sizes together, checking for integer overflow. If
 * overflow is detected, set the value pointed to by `rv` to true.
 * Otherwise, simply add the two values and return the result.
 */

size_t safe_add(const size_t a, const size_t b, int *rv) {

    if (a + b < a) {
        *rv = TRUE;
    }

    return (a + b);
}


/**
 * @name safe_subtract
 * Subtract size `b` from size `a`, checking for integer underflow.
 * If underflow is detected, set the value pointed to by `rv` to true.
 * Otherwise, simply add the two values and return the result.
 */

size_t safe_subtract(const size_t a, const size_t b, int *rv) {

    if (a - b > a) {
        *rv = TRUE;
    }

    return (a - b);
}


/**
 * @name safe_multiply
 * Multiply two sizes together, checking for integer overflow.
 * If overflow is detected, set the value pointed to by `rv` to
 * true. Otherwise, simply multiply the two and return the result.
 */

size_t safe_multiply(const size_t a, const size_t b, int *rv) {

    if (b != 0 && a > ((size_t) ~0UL / b)) {
        *rv = TRUE;
    }

    return (a * b);
}


#ifdef _MUVUKU_PROTOTYPE

/**
 * Handle an unrecoverable error. This should only
 * be called if a hardware problem is suspected, e.g.
 * persistent memory fails to hold values after write.
 */
void muvuku_panic(const char *detail) {

    printf("muvuku_panic: aborting\n");
    abort();
}

#else

/**
 * Handle an unrecoverable error. This should only
 * be called if a hardware problem is suspected, e.g.
 * persistent memory fails to hold values after write.
 */
void muvuku_panic(const char *detail) {

    for (;;) {
        display_text(panic_text, panic_title);
    }
}

#endif /* defined _MUVUKU_PROTOTYPE */


/**
 * @name xmalloc
 */
void *xmalloc(size_t n) {

    void *rv = malloc(n);

    if (rv == NULL) {
        muvuku_panic(panic_memory);
    }

    return rv;
}


/**
 * @name ucs2_slv_encode
 */
char *ucs2_slv_encode(char *ucs2_str, u8 len) {

    char *rv = (char *) xmalloc(len + 4);

    if (!rv) {
        return NULL;
    }

    rv[0] = '\x84';
    rv[1] = '\x08';
    rv[2] = len + 1;
    rv[3] = '\x80';

    memcpy(&rv[4], ucs2_str, len);
    return rv;
}


/**
 * @name ucs2_slv_to_gsm
 */
char *ucs2_slv_to_gsm(char *str) {

    if (str[0] != '\x84' || str[1] != '\x08' || str[3] != '\x80') {
        return NULL;
    }

    u8 len = str[2];
    char *rv = xmalloc(len + 1);

    if (!rv || len <= 0) {
        return NULL;
    }

    int i = 0;
    char *p = rv;
    char *s = &str[4];

    for (--len; i < len; i += 2) {

        /* ucs2_page = s[i]; */
        *p++ = s[i + 1];
    }

    *p = '\0';
    return rv;

}


/**
 * @name is_numeric_string
 */
u8 is_numeric_string(char *p) {

    while (*p != '\0') {

        if (*p < '0' || *p > '9') {
            return FALSE;
        }

        p++;
    }

    return TRUE;
}

#ifndef _MUVUKU_PROTOTYPE
#ifdef _MUVUKU_DEBUG

/**
 * @name display_hex16
 */
void display_hex16 (u16 val)
{
  u8 i;
  u8 *str = (u8 *) xmalloc(32);
  memzero(str, 32);

  u8 *s = str;

  i = (val >> 12) & 0x0f;
  if (i <= 0x09)
    *s++ = i + '0';
  else
    *s++ = i - 0x0a + 'a';

  i = (val >> 8) & 0x0f;
  if (i <= 0x09)
    *s++ = i + '0';
  else
    *s++ = i - 0x0a + 'a';

  i = (val >> 4) & 0x0f;
  if (i <= 0x09)
    *s++ = i + '0';
  else
    *s++ = i - 0x0a + 'a';

  i = val & 0x0f;
  if (i <= 0x09)
    *s++ = i + '0';
  else
    *s++ = i - 0x0a + 'a';

  display_text(str, NULL);
  free(str);
}

#endif /* _MUVUKU_DEBUG */
#endif /* ! defined _MUVUKU_PROTOTYPE */

