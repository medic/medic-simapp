/**
 * Muvuku: An STK data collection framework
 *
 * Copyright 2011-2012 Medic Mobile, Inc. <david@medicmobile.org>
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

#include "flash.h"
#include "util.h"
#include "settings.h"



/* Identifier for settings schema */
const u8 PROGMEM lc_settings_code[] = "MUVU";


/* Strings for settings schema */
lc_char PROGMEM lc_phone_sms[] = {
    LC_EN("SMS Phone Number")
    LC_FR("Num\5ro de la passerelle")
    LC_ES("N\6mero de tel\5fono")
    LC_END
};

lc_char PROGMEM lc_err_phone[] = {
    LC_EN("Phone number is invalid")
    LC_FR("Num\5ro invalide")
    LC_ES("El n\6mero de tel\5fono es invalido")
    LC_END
};


#ifndef _MUVUKU_PROTOTYPE

/* Validation rule:
    This is used to validate the phone number setting. */

u8 is_phone_number(schema_list_t *l, schema_item_t *i)
{
    if (i->data_type == TS_PHONE && i->value.msisdn != NULL) {
        return TRUE;
    }

    return schema_item_fail_validation(i, locale(lc_err_phone));
}

#endif /* defined _MUVUKU_PROTOTYPE */


/* Default phone number:
    This is used if no settings are saved in EEPROM. */

lc_char PROGMEM lc_default_phone[] = {
    LC_EN("+15146906436")
    LC_FR("+15146906436")
    LC_ES("+15146906436")
    LC_END
};


/* Create new settings:
    This should be used during application initialization
    to allocate new storage from EEPROM and set defaults. */

muvuku_settings_t *muvuku_settings_create() {

    muvuku_allocator_t *eeprom = &muvuku_eeprom_allocator;

    muvuku_settings_t *s = eeprom->alloc(
        sizeof(muvuku_settings_t), NULL
    );

    #ifndef _DISABLE_STORAGE
        /* Create new pooled storage in flash */
        muvuku_pool_t *p = muvuku_pool_new(
            &muvuku_flash_allocator, MUVUKU_FLASH_RESERVED,
                MUVUKU_NR_FORMS_MAX, &muvuku_flash_reserved
        );
    #endif /* _DISABLE_STORAGE */

    /* Zero space in EEPROM for application settings */
    eeprom->zero(s, sizeof(*s));

    #ifndef _DISABLE_STORAGE
        /* Save handle for new pool in EEPROM */
        muvuku_pool_handle_t h = muvuku_pool_handle(p);
        eeprom->write(&s->flash_pool, &h, sizeof(h));
        muvuku_pool_close(p);
    #endif /* _DISABLE_STORAGE */

    return s;
}


/* Permenantly destroy settings:
    This should be used during application uninstallation
    to free all storage previously allocated from EEPROM. */

void muvuku_settings_delete(muvuku_settings_t *s) {

    muvuku_allocator_t *eeprom = &muvuku_eeprom_allocator;

    #ifndef _DISABLE_STORAGE
        muvuku_pool_handle_t h;
        eeprom->read(&h, &s->flash_pool, sizeof(h));

        if (h != NULL) {
            muvuku_pool_delete(
                muvuku_pool_open(&muvuku_flash_allocator, h)
            );
        }
    #endif /* _DISABLE_STORAGE */

    eeprom->free(s);
}


#ifndef _MUVUKU_PROTOTYPE

/* Retrieve settings stored in EEPROM:
    This provides persistent storage for application settings,
    or other data that needs to be saved across power cycles. */

void muvuku_settings_read(muvuku_settings_t *s,
                          schema_list_t *settings_schema) {

    unsigned int magic = 0;
    muvuku_allocator_t *eeprom = &muvuku_eeprom_allocator;

    /* Read "magic" value */
    eeprom->read(&magic, &s->magic, sizeof(unsigned int));

    /* Try to read settings from EEPROM:
        If the "magic" value doesn't match what's expected,
        then there's no data stored -- use the default number. */

    if (magic == MUVUKU_SETTINGS_MAGIC) {

        size_t len = sizeof(s->msisdn_text);
        char *saved_phone = (char *) xmalloc(len);

        eeprom->read(saved_phone, s->msisdn_text, len);

        schema_item_set_phone(
            &settings_schema->list[0],
                saved_phone, strlen(saved_phone) + 1, FL_NONE
        );

        free(saved_phone);

    } else {

        schema_item_set_phone(
            &settings_schema->list[0], locale(lc_default_phone),
                strlen(locale(lc_default_phone)) + 1, FL_NONE
        );
    }
}

#endif /* defined _MUVUKU_PROTOTYPE */


/* Save settings to EEPROM:
    This serializes the settings from `schema_settings` to EEPROM. */

u8 muvuku_settings_write(muvuku_settings_t *s,
                         schema_list_t *schema_settings) {

    return muvuku_settings_write_phone(
        s, &schema_settings->list[0]
    );
}


/* Save phone number to EEPROM:
    This function writes the phone number in `schema_item_phone`
    to the `muvuku_settings_t` in EEPROM that is pointed to by `s`. */

u8 muvuku_settings_write_phone(muvuku_settings_t *s,
                               schema_item_t *schema_item_phone) {

    muvuku_allocator_t *eeprom = &muvuku_eeprom_allocator;

    unsigned int magic = MUVUKU_SETTINGS_MAGIC;
    size_t len = strlen(schema_item_phone->string_value) + 1;

    /* Write to EEPROM:
        This will be read back in during initialization;
        for details, see action_menu and muvuku_read_settings. */

    eeprom->write(&s->magic, &magic, sizeof(magic));
    eeprom->write(s->msisdn_text, schema_item_phone->string_value, len);

    return TRUE;
}


#ifndef _MUVUKU_PROTOTYPE

/* Trigger form of `muvuku_settings_write_phone`:
    This function saves the phone number stored in `schema_item_phone`
    to the settings structure (in EEPROM) pointed to by app_data(). */

u8 muvuku_settings_trigger_phone(schema_list_t *schema_settings,
                                 schema_item_t *schema_item_phone) {

    return muvuku_settings_write_phone(app_data(), schema_item_phone);
}

#endif /* defined _MUVUKU_PROTOTYPE */


/* Pool constructor:
    Open the pooled storage belonging to the settings subsystem.
    Returns a `muvuku_pool_t` that stores saved/outgoing SMSs. */

muvuku_pool_t *muvuku_storage_open(muvuku_settings_t *s) {

    muvuku_pool_handle_t h;
    muvuku_allocator_t *eeprom = &muvuku_eeprom_allocator;

    /* Read pool handle from EEPROM */
    eeprom->read(&h, &s->flash_pool, sizeof(h));

    /* Open flash pool using handle */
    return muvuku_pool_open(&muvuku_flash_allocator, h);
}


/* Storage cell locator:
    Find the pooled storage cell that belongs to the `schema_list`
    specified in `l`. If the `schema_list` does not currently have
    a storage cell assigned, assign a cell and return it. The mapping
    between `schema_list_t` and `muvuku_cell_t` is kept in EEPROM. */

muvuku_cell_t muvuku_storage_retrieve(muvuku_settings_t *s,
                                      muvuku_pool_t *from_pool,
                                      schema_list_t *for_schema_list) {
    /* Locals */
    u8 found_empty = FALSE;
    unsigned int i, i_empty = 0;
    muvuku_cell_t rv = INVALID_CELL;

    /* EEPROM read/write driver */
    muvuku_allocator_t *eeprom = &muvuku_eeprom_allocator;

    /* In-core storage for current map entry */
    muvuku_cell_map_t *e = xmalloc(sizeof(muvuku_cell_map_t));

    /* Find `type_id` comparison length */
    size_t len = strlen(for_schema_list->type_id);
    len = scalar_min(len, MUVUKU_TYPE_LENGTH_MAX);

    /* Initial search:
        Look for existing entry with matching `type_id` */

    for (i = 0; i < MUVUKU_NR_FORMS_MAX; ++i) {

        /* Bring entry in to core memory */
        eeprom->read(e, &(s->cell_map[i]), sizeof(*e));

        /* If current cell is empty, skip over it */
        if (e->type_id[0] == '\0') {

            /* Save `i` for first empty entry */
            if (!found_empty) {
                i_empty = i;
                found_empty = TRUE;
            }

            continue;
        }

        /* Check for matching `type_id` */
        if (memcmp(e->type_id, for_schema_list->type_id, len) == 0) {
            rv = e->cell;
            goto exit; /* Cell found */
        }
    }

    /* Did we see an empty cell?
        If not, we've hit the storage limit */

    if (!found_empty) {
        goto exit; /* Fail */
    }

    /* No match found, space available:
        Try to add a new entry to the `cell_map` in EEPROM */

    void *x = muvuku_pool_acquire(from_pool);
    e->cell = muvuku_pool_cell(from_pool, x);

    /* Check status of cell acquisition */
    if (e->cell == INVALID_CELL) {
        goto exit; /* Fail */
    }

    /* Initialize stringlist in new cell */
    muvuku_stringlist_init(from_pool, x);

    /* Copy `type_id` to new entry */
    memzero(e->type_id, sizeof(e->type_id));
    memcpy(e->type_id, for_schema_list->type_id, len);

    /* Write the new map entry to EEPROM */
    eeprom->write(&(s->cell_map[i_empty]), e, sizeof(*e));

    /* Return cell */
    rv = e->cell;

    exit:
        free(e);
        return rv;
}


#ifndef _MUVUKU_PROTOTYPE

/* Settings schema constructor:
    Create a new `schema_list_t` for settings, and return it. */

schema_list_t *muvuku_settings_schema() {

    SCHEMA_BEGIN(rv, lc_settings_code, 1);
        SCHEMA_ITEM(locale(lc_phone_sms), TS_PHONE, 4, 20);
            SCHEMA_ITEM_VALIDATE(is_phone_number);
            SCHEMA_ITEM_TRIGGER(muvuku_settings_trigger_phone);
    SCHEMA_END();

    return rv;
}


/* Settings schema constructor:
    Create a new `schema_list_t` for pin entry, and return it. */

schema_list_t *muvuku_pin_schema(const char *caption) {

    SCHEMA_BEGIN(rv, lc_settings_code, 1);
        SCHEMA_ITEM(caption, TS_INTEGER, 0, 64);
            SCHEMA_ITEM_FLAGS(FL_NO_ECHO);
    SCHEMA_END();

    return rv;
}


/* PIN support:
    This allows certain secions of the application to be
    protected with a simple password string. This is security through
    obscurity; don't rely on this for protection from determined
    attackers, or from anyone who knows how to disassemble a binary
    (or even just get the .trb file and run strings(1) on it). */

u8 muvuku_require_pin(const char *caption, const char *pin) {

    u8 rv = FALSE;
    schema_list_t *l = muvuku_pin_schema(caption);

    schema_list_prompt(l, PR_NORMAL);
    char *result = l->list[0].string_value;

    if (result && strcmp_any(result, pin) == 0) {
        rv = TRUE;
    }

    schema_list_delete(l);
    return rv;
};


#endif /* defined _MUVUKU_PROTOTYPE */


