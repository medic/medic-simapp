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

#include "util.h"
#include "string.h"

/**
 * Copy the string {src} to memory location {dst}, while
 * escaping instances of {delimiter} with {escape}.
 */
size_t strncpy_esc(size_t dstsz, char *dst,
                   char *src, char delimiter, char escape)
{
    size_t avail = dstsz;
    char *sp = src, *dp = dst;

    while (*sp != '\0' && dstsz > 1) {
        if (*sp == delimiter || *sp == escape) {
            /* Space check: esc + *sp + null */
            if (dstsz < 3)
                break;
            /* Escaped character */
            *dp++ = escape;
            *dp++ = *sp++;
            dstsz -= 2;
        } else {
            /* Any other character */
            *dp++ = *sp++;
            dstsz--;
        }
    }

    *dp = '\0';

    /* Total written, including terminator */
    return (avail - dstsz + 1);
}

/**
 * Compare two strings, regardless of the memory source
 * the strings occupy. This function copies the strings
 * in to RAM for comparison; the strings must fit there.
 */
int strcmp_any(const char *s1, const char *s2)
{
    int rv;

    size_t s1_len = strlen(s1) + 1;
    size_t s2_len = strlen(s2) + 1;

    char *s1_copy = xmalloc(s1_len);
    char *s2_copy  = xmalloc(s2_len);

    memcpy(s1_copy, s1, s1_len);
    memcpy(s2_copy, s2, s2_len);

    rv = strcmp(s1_copy, s2_copy);

    free(s1_copy);
    free(s2_copy);

    return rv;
}


