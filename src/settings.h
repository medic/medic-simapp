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

#ifndef __MUVUKU_SETTINGS_H__
#define __MUVUKU_SETTINGS_H__

#include "pool.h"
#include "schema.h"


/* Maximum form identifier length:
    This is a unique code used to identify forms; it's 
    kept inside the `schema_list_t` struct as `type_id`. */

#define MUVUKU_TYPE_LENGTH_MAX (4)


/* Maximum length of phone number:
    GSM says this is at most fifteen digits including country code;
    we leave some extra room for the `+` symbol and/or whitespace. */

#define MUVUKU_MSISDN_LENGTH_MAX (24)


/* "Magic" value:
    This value occupies the first field of `muvuku_settings`,
    and tells `muvuku_read_settings` that data is present. */

#define MUVUKU_SETTINGS_MAGIC (0x55aa)


/* Maximum number of forms:
    This value is used when allocating per-form resources, such
    as flash memory. The available amount of storage for each
    form decreases as the number of available forms increases. */

#define MUVUKU_NR_FORMS_MAX (4)


/* Structures */

typedef struct muvuku_cell_map {

    muvuku_cell_t cell;
    char type_id[MUVUKU_TYPE_LENGTH_MAX + 1];

} __attribute__((packed)) muvuku_cell_map_t;


typedef struct muvuku_settings {

    u16 magic;
    muvuku_pool_handle_t flash_pool;
    char msisdn_text[MUVUKU_MSISDN_LENGTH_MAX];
    muvuku_cell_map_t cell_map[MUVUKU_NR_FORMS_MAX];

} __attribute__((packed)) muvuku_settings_t;


/* Methods */

muvuku_settings_t *muvuku_settings_create();

void muvuku_settings_delete();

void muvuku_settings_read(muvuku_settings_t *s, schema_list_t *l);

u8 muvuku_settings_write(muvuku_settings_t *s, schema_list_t *l);

schema_list_t *muvuku_settings_schema();


u8 muvuku_settings_write_phone(muvuku_settings_t *s, schema_item_t *i);

u8 muvuku_settings_trigger_phone(schema_list_t *l, schema_item_t *i);


muvuku_pool_t *muvuku_storage_open(muvuku_settings_t *s);

muvuku_cell_t muvuku_storage_retrieve(
    muvuku_settings_t *s,
        muvuku_pool_t *from_pool, schema_list_t *for_schema_list
);


u8 muvuku_require_pin(const char *caption, const char *pin);


#endif /* __MUVUKU_SETTINGS_H__ */

