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

#include <stdint.h>
#include <limits.h>

#include "bladox.h"
#include "prototype.h"

#include "pool.h"
#include "string.h"
#include "util.h"


#ifdef _MUVUKU_PROTOTYPE

    /* In prototyping mode:
        When operating in prototyping mode, provide stubs for
        programmed-I/O instructions to make testing possible.
        These are not emitted when building Muvuku for AVR. */

    #define _lx(x) ((uintptr_t) (x))

    #define rb(p) _prototype_rb(p)
    #define wb(p, v) _prototype_wb(p, v)

    u8 _prototype_rb(void *p) {
        u8 rv = *((u8 *) p);
        #ifdef _MUVUKU_PROTOTYPE_DEBUG
            printf("rb: read byte `0x%lx` from 0x%lx\n", _lx(rv), _lx(p));
        #endif /* _MUVUKU_PROTOTYPE_DEBUG */
        return rv;
    }

    void _prototype_wb(void *p, u8 v) {
        #ifdef _MUVUKU_PROTOTYPE_DEBUG
            printf("wb: write byte `0x%lx` to 0x%lx\n", _lx(v), _lx(p));
        #endif /* _MUVUKU_PROTOTYPE_DEBUG */
        *((u8 *) p) = v;
    }

    #define rw(p) _prototype_rw(p)
    #define ww(p, v) _prototype_ww(p, v)

    u16 _prototype_rw(void *p) {
        u16 rv = *((u16 *) p);
        #ifdef _MUVUKU_PROTOTYPE_DEBUG
            printf("rw: read word `0x%lx` from 0x%lx\n", _lx(rv), _lx(p));
        #endif /* _MUVUKU_PROTOTYPE_DEBUG */
        return rv;
    }

    void _prototype_ww(void *p, u16 v) {
        #ifdef _MUVUKU_PROTOTYPE_DEBUG
            printf("ww: write word `0x%lx` to 0x%lx\n", _lx(v), _lx(p));
        #endif /* _MUVUKU_PROTOTYPE_DEBUG */
        *((u16 *) p) = v;
    }

    #define emalloc(n) _prototype_emalloc(n)
    #define efree(p) _prototype_efree(p)

    void *_prototype_emalloc(size_t n) {
        void *rv = xmalloc(n);
        #ifdef _MUVUKU_PROTOTYPE_DEBUG
            printf("emalloc: allocated %lu bytes at 0x%lx\n", _lx(n), _lx(rv));
        #endif /* _MUVUKU_PROTOTYPE_DEBUG */
        return rv;
    }

    void _prototype_efree(void *p) {
        free(p);
        #ifdef _MUVUKU_PROTOTYPE_DEBUG
            printf("efree: freed 0x%lx\n", _lx(p));
        #endif /* _MUVUKU_PROTOTYPE_DEBUG */
    }

    #define progmem_write(dst, src) _prototype_progmem_write(dst, src)

    void _prototype_progmem_write(void *dst, void *src) {

        /* Simulated alignment requirement:
            If the destination pointer isn't page-aligned, complain. */

        if (!muvuku_is_page_aligned(dst)) {
            printf("progmem_write: target 0x%lx is unaligned\n", _lx(dst));
            printf("progmem_write: simulated bus error, dumping core");
            abort();
        }

        memcpy(dst, src, MUVUKU_PAGE_SIZE);

        #ifdef _MUVUKU_PROTOTYPE_DEBUG
            printf(
                "progmem_write: copied %d bytes from 0x%lx to 0x%lx\n",
                    MUVUKU_PAGE_SIZE, _lx(src), _lx(dst)
            );
        #endif /* _MUVUKU_PROTOTYPE_DEBUG */

        #ifdef _MUVUKU_PROTOTYPE_DEBUG_VERBOSE
            printf(
                "progmem_write: verbose mode, dumping all %d bytes\n"
                    " ", MUVUKU_PAGE_SIZE
            );

            unsigned int i = 0;

            for (i = 0; i < MUVUKU_PAGE_SIZE; ++i) {

                if (i > 0 && i % 8 == 0) {
                    printf("\n ");
                }
                printf(" 0x%.2x", ((unsigned char *) src)[i]);
            }

            printf("\n");

        #endif /* _MUVUKU_PROTOTYPE_DEBUG_VERBOSE */
    }

#endif


/* Macro definitions for assignment:
    These use the pool's allocator, which may use programmed
    I/O. Because the address-of operator is used below, it's
    permissable to use the dereference or structure-dereference
    operator on a `muvuku_pool_data_t` passed via `lhs` or `rhs`. */

#define _write_pool_value(p, lhs, rhs) \
    do { \
        (p)->allocator->write(&(lhs), &(rhs), sizeof(rhs)); \
    } while (0)

#define _read_pool_value(p, lhs, rhs) \
    do { \
        (p)->allocator->read(&(lhs), &(rhs), sizeof(rhs)); \
    } while (0)


/* I/O verification:
    Optionally, verify writes both before and after. The
    former reduces electrical wear on persistent memory cells
    by avoiding unnecessary writes; the latter detects failure
    of write operations and alerts the user that the persistent
    memory is no longer reliable. Both of these are important;
    please don't disable this functionality when in production. */

#ifndef _MUVUKU_DISABLE_WEAR_REDUCTION
    #define _write_if_necessary(type, ptr, value) \
        do { \
            w##type(ptr, value); \
        } while (0)
#else
    #define _write_if_necessary(type, ptr, value) \
        do { \
            if (r##type(ptr) == (value)) \
                return FALSE; \
            \
            w##type(ptr, value); \
        } while (0)
#endif


#ifndef _MUVUKU_DISABLE_WRITE_VERIFICATION
    #define _write_with_verify(type, ptr, value) \
        do { \
            _write_if_necessary(type, ptr, value); \
            \
            if (r##type(ptr) != (value)) \
                muvuku_panic(NULL); \
            \
            return TRUE; \
        } while (0)
#else
    #define _write_with_verify(type, ptr, value) \
        do { \
            _write_if_necessary(type, ptr, value); \
            return TRUE; \
        } while (0)
#endif



/* Single-page buffer for flash memory write operations:
    This is initialized in `muvuku_subsystem_init_pool`,
    and is used by `muvuku_flash_write` and `muvuku_flash_zero`. */

u8 *muvuku_flash_buffer = NULL;


/**
 * Find the first non-zero bit in a machine word, starting with
 * the least significant bit as offset 1. Viewed another way,
 * this function counts the number of consecutive ones, beginning
 * with the least significant bit of `v`.
 */
unsigned int find_first_zero(muvuku_word_t v) {

    unsigned int i, mask = 1;

    for (i = 0; i < MUVUKU_WORD_BITS; ++i) {
        if (!(mask & v)) {
            return (i + 1);
        }
        mask <<= 1;
    }

    return 0;
}


/**
 * I/O primitive: read a single byte.
 */
u8 muvuku_rb(u8 *ptr) {

    return rb(ptr);
}


/**
 * I/O primitive: read a single machine word.
 */
u16 muvuku_rw(u16 *ptr) {

    return rw(ptr);
}


/**
 * I/O primitive: write a single byte with verification.
 */
u8 muvuku_wb(u8 *ptr, u8 v) {

    _write_with_verify(b, ptr, v);
}


/**
 * I/O primitive: write a machine word with verification.
 */
u8 muvuku_ww(u16 *ptr, u16 v) {

    _write_with_verify(w, ptr, v);
}



/**
 * Allocator:
 *   Ordinary in-core random access memory
 */
#ifdef _ENABLE_RAM_POOL

    void *muvuku_ram_alloc(size_t n, void *unused) {
        return xmalloc(n);
    }


    void muvuku_ram_free(void *x) {
        free(x);
    }


    void muvuku_ram_read(void *buf, void *x, size_t n) {
        memcpy(buf, x, n);
    }


    void muvuku_ram_write(void *x, void *buf, size_t n) {
        memcpy(x, buf, n);
    }


    void muvuku_ram_zero(void *x, size_t n) {
        memset(x, 0, n);
    }


    muvuku_allocator_t muvuku_ram_allocator = {

        /* Linker issue:
            Initializing struct members with function pointers
            seems to severely corrupt the memory layout on AVR.
            Use `muvuku_subsystem_init_pool` at startup instead. */

        NULL, NULL, NULL, NULL, NULL
    };

#endif /* _ENABLE_RAM_POOL */


/**
 * Allocator:
 *  AVR EEPROM (typically 4KiB). Suitable for small objects.
 */

void *muvuku_eeprom_alloc(size_t n, void *unused) {
    return emalloc(n);
}


void muvuku_eeprom_free(void *x) {
    efree(x);
}

void muvuku_eeprom_read(void *buf, void *x, size_t n) {

    size_t i = 0;

    u8 *src = (u8 *) x;
    u8 *dst = (u8 *) buf;

    if (n <= 0) {
        return;
    }

    if (!muvuku_is_word_aligned(src)) {
        /* Source unaligned; copy first byte */
        dst[0] = muvuku_rb(&src[0]); ++i;
    }

    for (; (i + 1) < n; i += 2) {
        /* Read aligned 16-bit words */
        *((u16 *) &dst[i]) = muvuku_rw(((u16 *) &src[i]));
    }

    for (; i < n; ++i) {
        /* Final byte, if n is odd */
        dst[i] = muvuku_rb(&src[i]);
    }
}

void muvuku_eeprom_write(void *x, void *buf, size_t n) {

    size_t i = 0;

    u8 *dst = (u8 *) x;
    u8 *src = (u8 *) buf;

    if (n <= 0) {
        return;
    }

    if (!muvuku_is_word_aligned(dst)) {
        /* Destination unaligned; copy first byte */
        muvuku_wb(&dst[0], src[0]); ++i;
    }

    for (; (i + 1) < n; i += 2) {
        /* Write aligned 16-bit words */
        muvuku_ww((u16 *) &dst[i], *((u16 *) &src[i]));
    }

    for (; i < n; ++i) {
        /* Final byte, if n is odd */
        muvuku_wb(&dst[i], src[i]);
    }
}


void muvuku_eeprom_zero(void *x, size_t n) {

    size_t i = 0;

    u8 *dst = (u8 *) x;

    if (!muvuku_is_word_aligned(dst)) {
        /* Destination unaligned; zero first byte */
        muvuku_wb(&dst[0], 0); ++i;
    }

    for (; (i + 1) < n; i += 2) {
        /* Write aligned 16-bit words */
        muvuku_ww((u16 *) &dst[i], 0);
    }

    for (; i < n; ++i) {
        /* Final byte, if n is odd */
        muvuku_wb(&dst[i], 0);
    }
}


muvuku_allocator_t muvuku_eeprom_allocator = {

    /* Linker issue:
        Initializing struct members with function pointers
        seems to severely corrupt the memory layout on AVR.
        Use `muvuku_subsystem_init_pool` at startup instead. */

    NULL, NULL, NULL, NULL, NULL
};


/**
 * Allocator:
 *  AVR flash memory, also known as progmem. This can be
 *  used for persistent storage of relatively large objects.
 */

void *muvuku_flash_alloc(size_t unused, void *region_ptr) {

    /* Round up to page boundary:
        Alignment may result in less available space than anticipated. */

    return muvuku_align_page(region_ptr, void, TRUE);
}


void muvuku_flash_free(void *x) {

    /* No-op:
        No underlying allocator to which the region can be released. */

    return;
}


void muvuku_flash_read(void *buf, void *x, size_t n) {

    /* Read interface is identical to EEPROM */
    muvuku_eeprom_read(buf, x, n);
}


void muvuku_flash_write(void *x, void *buf, size_t n) {

    size_t i = 0;
    size_t pgsz = MUVUKU_PAGE_SIZE;

    u8 *p = (u8 *) x;       /* In flash memory */
    u8 *src = (u8 *) buf;   /* In primary memory */

    u8 *pb = muvuku_flash_buffer;

    /* Page range containing data:
        Data starts in `lhs_page`, ends in `rhs_page`. */

    u8 *lhs_page = muvuku_align_page(p, u8, FALSE);
    u8 *rhs_page = muvuku_align_page(p + n, u8, FALSE);

    /* First (partial) page:
        Read whole page, copy new suffix from `src`, write back. */

    if (lhs_page < p) {

        size_t lhs_seek = ((u8 *) x - lhs_page);
        size_t lhs_size = (pgsz - lhs_seek);

        muvuku_flash_read(pb, lhs_page, pgsz);
        memcpy(pb + lhs_seek, src, scalar_min(n, lhs_size));
        progmem_write(lhs_page, pb);

        p += lhs_size; i += lhs_size;
    }

    /* Full page(s):
        We're now aligned; copy until we hit the last page. */

    while (p < rhs_page) {

        memcpy(pb, &src[i], pgsz);
        progmem_write(p, pb);
        
        p += pgsz; i += pgsz;
    }

    /* Final (partial) page:
        Read whole page, copy new prefix from `src`, write back. */

    if (i < n) {

        size_t rhs_size = (n - i);

        muvuku_flash_read(pb, rhs_page, pgsz);
        memcpy(pb, &src[i], rhs_size);
        progmem_write(rhs_page, pb);
    }

    return;
}


void muvuku_flash_zero(void *x, size_t n) {

    size_t i = 0;
    size_t pgsz = MUVUKU_PAGE_SIZE;

    u8 *p = (u8 *) x;
    u8 *pb = muvuku_flash_buffer;

    /* Page range containing data:
        Target starts in `lhs_page`, ends in `rhs_page`. */

    u8 *lhs_page = muvuku_align_page(p, u8, FALSE);
    u8 *rhs_page = muvuku_align_page(p + n, u8, FALSE);

    /* First (partial) page:
        Read whole page, zero suffix, write back. */

    if (lhs_page < p) {

        size_t lhs_seek = ((u8 *) x - lhs_page);
        size_t lhs_size = (pgsz - lhs_seek);

        muvuku_flash_read(pb, lhs_page, pgsz);
        memset(pb + lhs_seek, 0, scalar_min(n, lhs_size));
        progmem_write(lhs_page, pb);

        p += lhs_size; i += lhs_size;
    }

    /* Full page(s):
        We're now aligned; zero the flash buffer, and repeatedly
        copy it over the vast majority of the flash memory region. */

    if (p < rhs_page) {
        memset(pb, 0, pgsz);
    }

    while (p < rhs_page) {
        progmem_write(p, pb);
        p += pgsz; i += pgsz;
    }

    /* Final (partial) page:
        Read whole page, zero prefix, write back. */

    if (i < n) {

        size_t rhs_size = (n - i);

        muvuku_flash_read(pb, rhs_page, pgsz);
        memset(pb, 0, rhs_size);
        progmem_write(rhs_page, pb);
    }

    return;
}


muvuku_allocator_t muvuku_flash_allocator = {

    /* Linker issue:
        Initializing struct members with function pointers
        seems to severely corrupt program memory on AVR.
        Use `muvuku_subsystem_init_pool` at startup instead. */

    NULL, NULL, NULL, NULL, NULL
};


/**
 * Create a new memory pool comprised of `n` objects,
 * occupying a total size `size`. Use a single contiguous
 * allocation from the memory allocator function `a`.
 */
muvuku_pool_t *muvuku_pool_new(muvuku_allocator_t *a, size_t size,
                               unsigned int n, void *allocate_options) {

    int overflow = FALSE;

    /* Length of free-space map:
        Machine words used for free-space bitmap (floor) */

    size_t bitmap_length = (n / MUVUKU_WORD_BITS);

    /* Round up:
        If `n` is not a multiple of word size, use ceiling */

    if (n % MUVUKU_WORD_BITS) {
        ++bitmap_length;
    }

    /* Structure size:
        Total size is struct + free-space map + data blocks. */

    size_t bitmap_size = safe_multiply(
        bitmap_length, sizeof(muvuku_word_t), &overflow
    );
    
    size_t data_size = safe_subtract(
        size, safe_add(bitmap_size, sizeof(muvuku_pool_data_t), &overflow),
            &overflow
    );

    if (overflow) {
        return NULL;
    }

    /* Allocate persistent storage:
        This memory cannot be written or read directly. You
        must use the allocator methods to access this storage. */

    muvuku_pool_data_t *p = (muvuku_pool_data_t *) a->alloc(
        size, allocate_options
    );

    /* Zero entire persistent structure:
        This is important, because it zeros the memory
        that will soon be occupied by the free-space bitmap. */

    a->zero(p, size);

    /* Write persistent pool data:
        Writes might be expensive, so buffer in core memory first. */

    muvuku_pool_data_t *pool =
        (muvuku_pool_data_t *) xmalloc(sizeof(*pool));

    pool->item_count = 0;
    pool->item_limit = n;
    pool->cell_size = data_size / n;
    pool->bitmap_length = bitmap_length;

    a->write(p, pool, sizeof(*pool));
    free(pool);

    return muvuku_pool_open(a, p);
}


/**
 */
muvuku_pool_t *muvuku_pool_open(muvuku_allocator_t *a,
                                muvuku_pool_handle_t p) {

    /* Pass-through for failed operations:
        This allows a allocator/handle search to be nested directly. */

    if (a == NULL || p == NULL) {
        return NULL;
    }

    /* In-memory pool data:
        Commonly-accessed data can be cached here. */

    muvuku_pool_t *rv = (muvuku_pool_t *) xmalloc(sizeof(*rv));

    rv->pool = p;
    rv->allocator = a;

    return rv;
}


/**
 * Retrieve an opaque handle that uniquely identifies the
 * persistent memory pool `p`. This handle can be used with
 * `muvuku_pool_open` to attach to an already-created pool.
 */
muvuku_pool_handle_t muvuku_pool_handle(muvuku_pool_t *p) {

    return p->pool;
}


/**
 * Close a pool, freeing any in-core or cached data. Calling
 * this does not destroy any persistently-stored data; you can
 * restore the pool object by calling `muvuku_pool_open`, and
 * supplying the pool handle provided by `muvuku_pool_handle`.
 */
void muvuku_pool_close(muvuku_pool_t *p) {

    free(p);
}


/**
 * Destroy a pool, and return any occupied memory back to the
 * underlying allocator. Calling this function will permenantly
 * destroy any and all data in the pool.
 */
void muvuku_pool_delete(muvuku_pool_t *p) {

    p->allocator->free(p->pool);
    muvuku_pool_close(p);
}


/**
 * Given a one-based cell number, examine the appropriate portion
 * of the free-space map. If `bitmap` is non-null, write the bitmap
 * data just read to the location pointed to by `bitmap`. If `index`
 * is non-null, write the free-space bitmap offset to the location
 * pointed to by `index`. Finally, and unconditionally, return the
 * one-based bit offset (relative to `bitmap`) that describes `cell`.
 */
muvuku_word_t _muvuku_pool_bitmap(muvuku_pool_t *p,
                                  muvuku_word_t *bitmap,
                                  size_t *index, muvuku_cell_t cell) {
    if (cell == 0) {
        return 0;
    }

    cell -= 1;
    size_t i = (cell / MUVUKU_WORD_BITS);
    muvuku_word_t bit = (cell % MUVUKU_WORD_BITS);

    if (bitmap) {
        _read_pool_value(p, *bitmap, p->pool->data[i].bitmap);
    }

    if (index) {
        *index = i;
    }

    return (bit + 1);
}


/**
 * Given a one-based cell number (which identifies a distinct storage
 * location in the pool), return a pointer to the memory region it
 * occupies. Note that, in the underscore-prefixed version, `rp`
 * must point to a copy of `p` that resides in *main memory* -- e.g.
 * the pool structure (minus data from its free-space map and actual
 * data cells) must be copied in to RAM before being passed to this
 * function. If you want this done for you, use the exported version.
 */
void *_muvuku_pool_address(muvuku_pool_t *p,
                           muvuku_pool_data_t *rp, muvuku_cell_t c) {

    if (c == INVALID_CELL) {
        return NULL;
    }

    return (void *) (
        &p->pool->data[rp->bitmap_length].raw + (rp->cell_size * (c - 1))
    );
}


/**
 * This is the exported version of `_muvuku_pool_address`.
 * For ease of error handling, supplying `INVALID_CELL` for
 * `c` will cause this function to return a `NULL` pointer.
 */
void *muvuku_pool_address(muvuku_pool_t *p, muvuku_cell_t c) {

    void *rv = NULL;

    muvuku_pool_data_t *pool =
        (muvuku_pool_data_t *) xmalloc(sizeof(*pool));

    _read_pool_value(p, *pool, *p->pool);

    /* Zero is the invalid cell */
    if (c <= 0 || c > pool->item_limit) {
        goto exit;
    }

    muvuku_cellinfo_t info = muvuku_pool_cellinfo(p, c);

    if (!(info & CL_OCCUPIED)) {
        goto exit;
    }

    rv = _muvuku_pool_address(p, pool, c);

    exit:
        free(pool);
        return rv;
}


/**
 * Given a pool and a pointer returned from `muvuku_pool_acquire`,
 * return the one-based cell number that identifies the region.
 * Note that, in the underscore-prefixed version, `rp` must
 * point to a copy of `p` that resides in *main memory* -- e.g. the
 * pool structure (minus data from its free-space map and actual
 * data cells) must be copied in to RAM before being passed to this
 * function. If you want this done for you, use the exported version.
 */
muvuku_cell_t _muvuku_pool_cell(muvuku_pool_t *p,
                                muvuku_pool_data_t *rp, void *x) {

    return (muvuku_cell_t) (
        (((unsigned char *) x - &p->pool->data[rp->bitmap_length].raw)
            / rp->cell_size) + 1
    );
}


/**
 * This is the exported version of `_muvuku_pool_cell`.
 * For ease of error-checking, supplying `NULL` for `x` will
 * cause this function to return the value `INVALID_CELL`.
 */
muvuku_cell_t muvuku_pool_cell(muvuku_pool_t *p, void *x) {

    muvuku_cell_t rv = INVALID_CELL;

    if (x == NULL) {
        return rv;
    }

    muvuku_pool_data_t *pool =
        (muvuku_pool_data_t *) xmalloc(sizeof(*pool));

    _read_pool_value(p, *pool, *p->pool);
    muvuku_cell_t cell = _muvuku_pool_cell(p, pool, x);

    if (cell <= 0 || cell > pool->item_limit) {
        goto exit;
    }

    muvuku_cellinfo_t info = muvuku_pool_cellinfo(p, cell);

    if (!(info & CL_OCCUPIED)) {
        goto exit;
    }

    rv = cell;

    exit:
        free(pool);
        return rv;
}


/**
 * Interrogate the one-based cell number `cell` in the pool `p`.
 * Returns an integer value describing the state of the cell, with
 * the lowest-order bit set if and only if the cell is in use
 * (that is, the cell has been acquired and not yet freed).
 */
muvuku_cellinfo_t muvuku_pool_cellinfo(muvuku_pool_t *p,
                                       muvuku_cell_t cell) {

    muvuku_word_t bitmap = 0;
    muvuku_word_t bit = _muvuku_pool_bitmap(p, &bitmap, NULL, cell);

    return (bit && (bitmap & (1 << (bit - 1))) ? 1 : 0);
}


/**
 * Return the size of a single cell in the pool. All cells
 * in the pool are identically-sized.
 */
size_t muvuku_pool_cell_size(muvuku_pool_t *p) {

    size_t rv;
    _read_pool_value(p, rv, p->pool->cell_size);

    return rv;
}


/**
 * Copy `n` bytes from `data` to the pool-managed cell of memory
 * located at `x`. If you need to write to a particular cell but
 * do not have a pointer to it, use `muvuku_pool_address`.
 */
void muvuku_pool_write(muvuku_pool_t *p, void *x, void *data, size_t n) {

    p->allocator->write(x, data, n);
}


/**
 * Copy `n` bytes from the pool-managed cell of memory at `x` to
 * the in-core memory pointed to by `data`. If you need to read
 * from a particular cell but * do not have a pointer to it, use
 * `muvuku_pool_address`.
 */
void muvuku_pool_read(muvuku_pool_t *p, void *data, void *x, size_t n) {

    p->allocator->read(data, x, n);
}

/**
 * Get a new fixed-size block of memory from the pool.
 */
void *muvuku_pool_acquire(muvuku_pool_t *p) {

    void *rv = NULL;
    unsigned int i, bit = 0;

    muvuku_pool_data_t *pool =
        (muvuku_pool_data_t *) xmalloc(sizeof(*pool));

    _read_pool_value(p, *pool, *p->pool);

    /* Space available?
        If we've reached the item limit, fail. */

    if (pool->item_count >= pool->item_limit) {
        return NULL;
    }

    for (i = 0; i < pool->bitmap_length; ++i) {

        muvuku_word_t bitmap = 0;
        _read_pool_value(p, bitmap, p->pool->data[i].bitmap);

        if ((bit = find_first_zero(bitmap)) <= 0) {

            /* No space available:
                Go around and check the next machine word.
                If there's no more index space available, and
                we manage to get to this point somehow, bail. */

            if (i + 1 >= pool->bitmap_length) {
                goto exit;
            }

        } else {

            /* Found block: mark as in-use */
            bitmap |= (1 << (bit - 1));
            pool->item_count++;

            /* Write modified values back */
            _write_pool_value(p, p->pool->data[i].bitmap, bitmap);
            _write_pool_value(p, p->pool->item_count, pool->item_count);

            break;
        }
    }

    /* Sanity check */
    if (bit == 0) {
        return NULL;
    }

    /* Use a one-based bit index as a one-based cell:
        Then, return a pointer to the computed cell's memory region. */

    rv = _muvuku_pool_address(
        p, pool, (i * sizeof(muvuku_word_t)) + bit
    );

    exit:
        free(pool);
        return rv;
}


/**
 * Release the block of memory `x` back to the pool.
 */
muvuku_pool_t *muvuku_pool_release(muvuku_pool_t *p, void *x) {

    /* Ignore one specific error case:
        If we get a null address (e.g. from `muvuku_pool_address`),
        then simply treat it as a no-op and return the pool pointer.
        This can happen if e.g. `muvuku_pool_address` is invoked on
        an invalid cell number, or on an already-freed cell number. */

    if (x == NULL) {
        return p;
    }

    muvuku_pool_data_t *pool =
        (muvuku_pool_data_t *) xmalloc(sizeof(*pool));

    _read_pool_value(p, *pool, *p->pool);

    size_t i = 0;
    muvuku_word_t bitmap = 0;

    muvuku_cell_t c = _muvuku_pool_cell(p, pool, x);
    muvuku_word_t bit = _muvuku_pool_bitmap(p, &bitmap, &i, c);

    /* Check bit: if already free, exit */
    if (!bit || (bitmap & (1 << (bit - 1))) == 0) {
        return NULL;
    }

    /* Clear bit: block no longer in use */
    bitmap &= ~(1 << (bit - 1));

    /* Decrement item count */
    pool->item_count--;

    /* Write modified values back */
    _write_pool_value(p, p->pool->data[i].bitmap, bitmap);
    _write_pool_value(p, p->pool->item_count, pool->item_count);

    return p;
}


/**
 * Return a new object representing the stringlist at the pool-managed
 * memory location pointed to by `addr`.
 */
muvuku_stringlist_t *muvuku_stringlist_open(muvuku_pool_t *p, void *addr) {

    /* Pass-through for failed operations:
        This allows a pool/cell search to be nested directly. */

    if (p == NULL || addr == NULL) {
        return NULL;
    }

    muvuku_stringlist_t *rv =
        (muvuku_stringlist_t *) xmalloc(sizeof(*rv));

    rv->pool = p;
    rv->list = (muvuku_stringlist_data_t *) addr;

    return rv;
}


/**
 * Destroy an object in core memory that represents a string list.
 * Leave the persistently-stored data untouched and intact.
 */
void muvuku_stringlist_close(muvuku_stringlist_t *l) {

    free(l);
}


/**
 * Create a tightly-packed list of strings in the pool-managed
 * memory location specified by `l`.
 */
muvuku_stringlist_t *muvuku_stringlist_init(muvuku_pool_t *p, void *addr) {

    muvuku_stringlist_data_t *list =
        (muvuku_stringlist_data_t *) xmalloc(sizeof(*list));

    list->item_count = 0;

    list->bytes_remaining =
        muvuku_pool_cell_size(p) - sizeof(muvuku_stringlist_data_t);

    /* Copy to persistent storage */
    muvuku_stringlist_data_t *l = (muvuku_stringlist_data_t *) addr;
    _write_pool_value(p, *l, *list);
    free(list);

    return muvuku_stringlist_open(p, l);
}


/**
 */
size_t _muvuku_stringlist_size(muvuku_stringlist_t *l,
                               muvuku_stringlist_data_t *rl) {
    return (
        muvuku_pool_cell_size(l->pool) -
            sizeof(muvuku_stringlist_data_t) - rl->bytes_remaining
    );
}


/**
 */
size_t muvuku_stringlist_size(muvuku_stringlist_t *l) {

    size_t rv;

    muvuku_stringlist_data_t *list =
        (muvuku_stringlist_data_t *) xmalloc(sizeof(*list));

    _read_pool_value(l->pool, *list, *l->list);
    rv = _muvuku_stringlist_size(l, list);
    free(list);

    return rv;
}


/**
 * Add a byte string to the packed stringlist `l` (in the pool `p`).
 * This function is binary safe, and can function with or without
 * null terminators. The return value is true is the string was
 * successfully added, or false if there was insufficient space.
 */
int muvuku_stringlist_add(muvuku_stringlist_t *l,
                          char *src, muvuku_string_size_t len) {
    int rv = FALSE;

    muvuku_stringlist_data_t *list =
        (muvuku_stringlist_data_t *) xmalloc(sizeof(*list));

    _read_pool_value(l->pool, *list, *l->list);

    size_t necessary = len + sizeof(muvuku_string_t);
    size_t total_size = _muvuku_stringlist_size(l, list);

    if (len <= 0 || list->bytes_remaining < necessary) {
        goto exit;
    }

    /* Pack it up, pack it in */
    muvuku_string_t *str = (muvuku_string_t *) (
        (char *) l->list->strings + total_size
    );

    /* Let me begin */
    _write_pool_value(l->pool, str->len, len);
    list->bytes_remaining -= sizeof(muvuku_string_t);

    /* I came to win */
    l->pool->allocator->write(str->string, src, len);

    /* Battle me, that's a sin */
    list->bytes_remaining -= len;
    list->item_count++;

    /* I won't tear the stack up... */
    _write_pool_value(l->pool, *l->list, *list);
    rv = TRUE;

    exit:
        free(list);
        return rv;
}


/**
 * Iterate over some or all of the strings in the packed stringlist
 * `l` (residing inside of the pool `p`). The callback `fn will be
 * invoked once for each byte string, and provided the current pool,
 * the current stringlist, a pointer and length to the byte string,
 * and the pass-through parameter `state`. Note that no data is
 * copied before the callback is invoked; rather, the callback is
 * responsible for copying data with `muvuku_pool_read` should it
 * need its own copy.
 */
int muvuku_stringlist_each(muvuku_stringlist_t *l,
                           muvuku_stringlist_fn_t fn, void *state) {
    int rv = FALSE;

    muvuku_stringlist_data_t *list =
        (muvuku_stringlist_data_t *) xmalloc(sizeof(*list));

    _read_pool_value(l->pool, *list, *l->list);

    size_t offset = 0;
    size_t total_size = _muvuku_stringlist_size(l, list);

    /* Within allocated part of stringlist */
    while (offset < total_size) {

        /* Find current string */
        muvuku_string_t *str = (muvuku_string_t *) (
            (char *) l->list->strings + offset
        );

        /* Read string length */
        muvuku_string_size_t len;
        _read_pool_value(l->pool, len, str->len);
        offset += sizeof(muvuku_string_t);

        /* Invoke callback */
        if (!fn(l, str->string, (size_t) len, state)) {
            /* False means stop */
            goto exit;
        }

        /* Next string */
        offset += len;
    }

    rv = TRUE;

    exit:
        free(list);
        return rv;
}


/**
 * Initialize the pooled-storage subsystem. This function only has
 * a visible effect on the first call; subsequent calls are ignored.
 */
void muvuku_subsystem_init_pool()
{
    muvuku_allocator_t *a;

    /* Ignore duplicate calls */
    if (muvuku_flash_buffer != NULL) {
        return;
    }

    /* Electrically-erasable memory driver */
    a = &muvuku_eeprom_allocator;
    a->alloc = &muvuku_eeprom_alloc;
    a->free = &muvuku_eeprom_free;
    a->read = &muvuku_eeprom_read;
    a->write = &muvuku_eeprom_write;
    a->zero = &muvuku_eeprom_zero;

    /* Flash technology memory driver */
    a = &muvuku_flash_allocator;
    a->alloc = &muvuku_flash_alloc;
    a->free = &muvuku_flash_free;
    a->read = &muvuku_eeprom_read;
    a->write = &muvuku_flash_write;
    a->zero = &muvuku_flash_zero;

    /* In-core random-access memory driver */
    #ifdef _ENABLE_RAM_POOL
        a = &muvuku_ram_allocator;
        a->alloc = &muvuku_ram_alloc;
        a->free = &muvuku_ram_free;
        a->read = &muvuku_ram_read;
        a->write = &muvuku_ram_write;
        a->zero = &muvuku_ram_zero;
    #endif /* _ENABLE_RAM_POOL */

    /* Allocate a single-page buffer in RAM:
        This is used by the flash technology memory allocator.
        We use this buffer to temporarily store a page, modify it,
        and then write its entire contents back to flash memory.
        The buffer is allocated once and retained permanently. */

    muvuku_flash_buffer = (u8 *) xmalloc(MUVUKU_PAGE_SIZE);
}

