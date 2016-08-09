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

#ifndef __MUVUKU_POOL_H__
#define __MUVUKU_POOL_H__

#include <stdint.h>

#include "bladox.h"
#include "prototype.h"


/* Initialize pool subsystem:
    This must be called before any other function. */

void muvuku_subsystem_init_pool();



/* Optimal data size for processor */
typedef unsigned int muvuku_word_t;

/* Integer that can hold a pointer */
typedef uintptr_t muvuku_intptr_t;



/* Bits per machine word:
    This is here for cases when we want the preprocessor
    to access the literal value. Since sizeof is evaluated at
    compile-time, we need separate macros to accomplish this. */

#ifdef WORD_BIT
    #define MUVUKU_WORD_BITS (WORD_BIT)
#else
    #define MUVUKU_WORD_BITS (sizeof(muvuku_word_t) * CHAR_BIT)
#endif 



/* Alignment macros:
    These are used to align, or test for alignment, on a machine
    word boundary, or on a page boundary (e.g. for flash memory). */

#define MUVUKU_PAGE_SHIFT   8
#define MUVUKU_PAGE_SIZE    (1 << MUVUKU_PAGE_SHIFT)

#define MUVUKU_WORD_SIZE    (sizeof(muvuku_word_t))
#define MUVUKU_WORD_SHIFT   (MUVUKU_WORD_SIZE >> 1)


#define muvuku_is_aligned(shift, p) \
    !((muvuku_intptr_t) (p) & ((1 << (shift)) - 1))

#define muvuku_align(shift, ptr, ptr_type, upward) \
    ((ptr_type *) ( \
        (((muvuku_intptr_t) (ptr)) & ~((1 << (shift)) - 1)) \
        + ((upward) ? (1 << (shift)) : 0) \
    ))

#define muvuku_is_word_aligned(ptr) \
    muvuku_is_aligned(MUVUKU_WORD_SHIFT, (ptr))

#define muvuku_is_page_aligned(ptr) \
    muvuku_is_aligned(MUVUKU_PAGE_SHIFT, (ptr))

#define muvuku_align_word(ptr, ptr_type, upward) \
    (muvuku_is_word_aligned(ptr) ? (ptr_type *) (ptr) : \
        muvuku_align(MUVUKU_WORD_SHIFT, (ptr), ptr_type, (upward)))

#define muvuku_align_page(ptr, ptr_type, upward) \
    (muvuku_is_page_aligned(ptr) ? (ptr_type *) (ptr) : \
        muvuku_align(MUVUKU_PAGE_SHIFT, (ptr), ptr_type, (upward)))



/** @name muvuku_allocator_t **/

typedef struct muvuku_allocator {

    void *  (*alloc)(size_t, void *);
    void    (*free)(void *);
    void    (*read)(void *, void *, size_t);
    void    (*write)(void *, void *, size_t);
    void    (*zero)(void *, size_t);

} muvuku_allocator_t;

muvuku_allocator_t muvuku_ram_allocator;
muvuku_allocator_t muvuku_flash_allocator;
muvuku_allocator_t muvuku_eeprom_allocator;


/** @name muvuku_pool_t **/

/* Null value for `muvuku_cell_t` */
#define INVALID_CELL    (0)

/* Flags for `muvuku_cellinfo_t` */
#define CL_OCCUPIED     (1)


typedef unsigned int muvuku_cell_t;

typedef unsigned long muvuku_cellinfo_t;


typedef union {

    muvuku_word_t bitmap;
    unsigned char raw;

} __attribute__((packed)) muvuku_pool_union_t;


/* Persistent storage for pool */
typedef struct muvuku_pool_data {

    size_t cell_size;
    size_t bitmap_length;

    unsigned int item_count;
    unsigned int item_limit;

    /* Extensible structure */
    muvuku_pool_union_t data[];
    /* ... */

} __attribute__((packed)) muvuku_pool_data_t;


/* In-core representation of pool */
typedef struct muvuku_pool {

    /* Allocator for reading/writing */
    muvuku_allocator_t *allocator;

    /* Pointer to persistent storage */
    muvuku_pool_data_t *pool;

} muvuku_pool_t;


/* Opaque handle for persistent pool data */
typedef muvuku_pool_data_t* muvuku_pool_handle_t;


muvuku_pool_t *muvuku_pool_new(muvuku_allocator_t *a, size_t size,
                               unsigned int n, void *allocate_options);

muvuku_pool_t *muvuku_pool_open(muvuku_allocator_t *a,
                                muvuku_pool_handle_t p);

muvuku_pool_handle_t muvuku_pool_handle(muvuku_pool_t *p);

void muvuku_pool_close(muvuku_pool_t *p);

void muvuku_pool_delete(muvuku_pool_t *p);


void *muvuku_pool_acquire(muvuku_pool_t *p);

muvuku_pool_t *muvuku_pool_release(muvuku_pool_t *p, void *x);


void *muvuku_pool_address(muvuku_pool_t *p, muvuku_cell_t cell);

muvuku_cell_t muvuku_pool_cell(muvuku_pool_t *p, void *x);

muvuku_cellinfo_t muvuku_pool_cellinfo(muvuku_pool_t *p,
                                       muvuku_cell_t cell);

size_t muvuku_pool_cell_size(muvuku_pool_t *p);

void muvuku_pool_write(muvuku_pool_t *p, void *x, void *data, size_t n);

void muvuku_pool_read(muvuku_pool_t *p, void *data, void *x, size_t n);



/** @name muvuku_stringlist_t **/


/* String size, in bytes */
#ifdef _MUVUKU_TINY_STRINGS
    typedef uint8_t muvuku_string_size_t;
#else
    typedef muvuku_word_t muvuku_string_size_t;
#endif


/* Byte string, with explicit length */
typedef struct muvuku_string {

    muvuku_string_size_t len;
    char string[]; /* ... */

} __attribute__((packed)) muvuku_string_t;


/* Ordered list of byte strings */
typedef struct muvuku_stringlist_data {

    size_t item_count;
    size_t bytes_remaining;
    muvuku_string_t strings[]; /* ... */

} __attribute__((packed)) muvuku_stringlist_data_t;


/* In-core representation of string list */
typedef struct muvuku_stringlist {

    muvuku_pool_t *pool;
    muvuku_stringlist_data_t *list;

} muvuku_stringlist_t;


/* Iterator callback for `muvuku_stringlist_each` */
typedef int (*muvuku_stringlist_fn_t)(muvuku_stringlist_t *,
                                      char *, size_t, void *);


muvuku_stringlist_t *muvuku_stringlist_init(muvuku_pool_t *p, void *addr);

muvuku_stringlist_t *muvuku_stringlist_open(muvuku_pool_t *p, void *addr);

void muvuku_stringlist_close(muvuku_stringlist_t *l);

int muvuku_stringlist_add(muvuku_stringlist_t *l,
                          char *src, muvuku_string_size_t len);

int muvuku_stringlist_each(muvuku_stringlist_t *l,
                           muvuku_stringlist_fn_t fn, void *state);

size_t _muvuku_stringlist_size(muvuku_stringlist_t *l,
                               muvuku_stringlist_data_t *rl);

size_t muvuku_stringlist_size(muvuku_stringlist_t *l);


#endif /* __MUVUKU_POOL_H__ */

