/**
 * Test cases for Muvuku's pooled allocator (AVR)
 *
 * Copyright (c) 2012, Medic Mobile <hello@medicmobile.org>
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MEDIC MOBILE BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <flash.h>
#include <schema.h>
#include <pool.h>
#include <util.h>


/* ----------------------------------------------------------------------*/

u8 *sprint_hex16 (u8 * s, u16 val)
{
  u8 i;

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

  return s;
}

/* ----------------------------------------------------------------------*/


lc_char PROGMEM lc_topmenu_name[] = {
    LC_EN("SIM Services")
    LC_END
};

typedef struct app_data {

    muvuku_pool_handle_t flash_pool;
    muvuku_pool_handle_t eeprom_pool;

} app_data_t;


app_data_t app;

muvuku_allocator_t *flash;
muvuku_allocator_t *eeprom;

muvuku_cell_t cell;
muvuku_allocator_t *current_allocator;
muvuku_pool_handle_t *current_pool_handle;


void sync_app_data() {

    app_data_t *d = (app_data_t *) app_data();
    eeprom->write(d, &app, sizeof(app));
}


/* ----------------------------------------------------------------------*/


lc_char PROGMEM lc_menu_top[] = {
    LC_EN("Pooled Storage")
    LC_END
};

lc_char PROGMEM lc_not_alloc[] = {
    LC_EN("Pool not allocated")
    LC_END
};

u8 menu_top_ctx (SCtx *ctx, u8 action)
{
    return APP_OK;
}

SNodeP menu_top_n = { lc_menu_top, menu_top_ctx };


/* ----------------------------------------------------------------------*/


lc_char PROGMEM lc_menu_driver[] = {
    LC_EN("Change Driver")
    LC_END
};

lc_char PROGMEM lc_driver_eeprom[] = {
    LC_EN("Now using EEPROM driver")
    LC_END
};

lc_char PROGMEM lc_driver_flash[] = {
    LC_EN("Now using flash memory driver")
    LC_END
};

u8 menu_driver_ctx (SCtx *ctx, u8 action)
{
    if (action == APP_ENTER) {

        if (current_pool_handle == &app.eeprom_pool) {

            current_allocator = flash;
            current_pool_handle = &app.flash_pool;
            display_text(locale(lc_driver_flash), NULL);

        } else {
            
            current_allocator = eeprom;
            current_pool_handle = &app.eeprom_pool;
            display_text(locale(lc_driver_eeprom), NULL);
        }

        cell = INVALID_CELL;
    }

    return APP_OK;
}

SNodeP menu_driver_n = {
    lc_menu_driver, menu_driver_ctx
};


/* ----------------------------------------------------------------------*/


lc_char PROGMEM lc_menu_alloc[] = {
    LC_EN("Allocate")
    LC_END
};

lc_char PROGMEM lc_alloc_already[] = {
    LC_EN("Pool already allocated")
    LC_END
};

lc_char PROGMEM lc_alloc_success[] = {
    LC_EN("Pool allocated successfully")
    LC_END
};

u8 menu_alloc_ctx (SCtx *ctx, u8 action)
{
    if (action == APP_ENTER) {

        muvuku_pool_handle_t h = *current_pool_handle;

        if (h != NULL) {
            display_text(locale(lc_alloc_already), NULL);
            return APP_OK;
        }

        muvuku_pool_t *p = muvuku_pool_new(
            current_allocator, sizeof(*muvuku_flash_reserved), 8,
                &muvuku_flash_reserved
        );

        *current_pool_handle = muvuku_pool_handle(p);
        sync_app_data();

        display_text(locale(lc_alloc_success), NULL);
        muvuku_pool_close(p);
    }

    return APP_OK;
}

SNodeP menu_alloc_n = {
    lc_menu_alloc, menu_alloc_ctx
};


/* ----------------------------------------------------------------------*/


lc_char PROGMEM lc_menu_free[] = {
    LC_EN("Free")
    LC_END
};

lc_char PROGMEM lc_free_success[] = {
    LC_EN("Pool freed successfully")
    LC_END
};

u8 menu_free_ctx (SCtx *ctx, u8 action)
{
    if (action == APP_ENTER) {

        muvuku_pool_handle_t h = *current_pool_handle;

        if (h == NULL) {
            display_text(locale(lc_not_alloc), NULL);
            return APP_OK;
        }

        muvuku_pool_t *p = muvuku_pool_open(current_allocator, h);
        muvuku_pool_delete(p);

        *current_pool_handle = NULL;
        cell = INVALID_CELL;
        sync_app_data();

        display_text(locale(lc_free_success), NULL);
    }

    return APP_OK;
}

SNodeP menu_free_n = {
    lc_menu_free, menu_free_ctx
};


/* ----------------------------------------------------------------------*/


lc_char PROGMEM lc_menu_acquire[] = {
    LC_EN("Acquire Cell")
    LC_END
};

lc_char PROGMEM lc_acquire_success[] = {
    LC_EN("Acquisition successful")
    LC_END
};

lc_char PROGMEM lc_acquire_fail[] = {
    LC_EN("Couldn't acquire cell")
    LC_END
};

lc_char PROGMEM lc_initialize_fail[] = {
    LC_EN("Couldn't initialize cell")
    LC_END
};

lc_char PROGMEM lc_cell_number[] = {
    LC_EN("Selected cell ")
    LC_END
};

lc_char PROGMEM lc_cell_address[] = {
    LC_EN(" at address 0x")
    LC_END
};


void display_selected_cell(muvuku_pool_t *p, const lc_char *lc_success) {

    void *x = muvuku_pool_address(p, cell);

    size_t len = MUVUKU_PAGE_SIZE;
    char *output = (char *) malloc(len);
    memset(output, '\0', len);

    char *r = sprints(output, locale(lc_success));
    r = sprintc(r, ':');
    r = sprintc(r, ' ');
    r = sprints(r, locale(lc_cell_number));
    r = sprintc(r, '#');
    r = sprinti(r, cell);
    r = sprints(r, locale(lc_cell_address));
    r = sprint_hex16(r, (u16) x);

    display_text(output, NULL);
    free(output);
}


u8 menu_acquire_ctx (SCtx *ctx, u8 action)
{
    if (action == APP_ENTER) {

        muvuku_pool_handle_t h = *current_pool_handle;

        if (h == NULL) {
            display_text(locale(lc_not_alloc), NULL);
            return APP_OK;
        }

        muvuku_pool_t *p = muvuku_pool_open(current_allocator, h);
        void *x = muvuku_pool_acquire(p);
        cell = muvuku_pool_cell(p, x);

        if (x == NULL || cell == INVALID_CELL) {
            display_text(locale(lc_acquire_fail), NULL);
            goto exit;
        }

        if (!muvuku_stringlist_init(p, x)) {
            display_text(locale(lc_initialize_fail), NULL);
            goto exit;
        }

        display_selected_cell(p, lc_acquire_success);

        exit:
            muvuku_pool_close(p);
    }

    return APP_OK;
}

SNodeP menu_acquire_n = {
    lc_menu_acquire, menu_acquire_ctx
};


/* ----------------------------------------------------------------------*/


lc_char PROGMEM lc_menu_release[] = {
    LC_EN("Release Cell")
    LC_END
};


u8 menu_release_ctx (SCtx *ctx, u8 action)
{
    if (action == APP_ENTER) {

        muvuku_pool_handle_t h = *current_pool_handle;

        if (h == NULL) {
            display_text(locale(lc_not_alloc), NULL);
            return APP_OK;
        }

        muvuku_pool_t *p = muvuku_pool_open(current_allocator, h);

        void *x = muvuku_pool_address(p, cell);
        muvuku_pool_release(p, x);
        cell = INVALID_CELL;

        muvuku_pool_close(p);
    }

    return APP_OK;
}

SNodeP menu_release_n = {
    lc_menu_release, menu_release_ctx
};


/* ----------------------------------------------------------------------*/


lc_char PROGMEM lc_menu_select[] = {
    LC_EN("Select Cell")
    LC_END
};

lc_char PROGMEM lc_switch_success[] = {
    LC_EN("Selection successful")
    LC_END
};

lc_char PROGMEM lc_cell_number_prompt[] = {
    LC_EN("Enter cell number")
    LC_END
};

lc_char PROGMEM lc_invalid_cell_number[] = {
    LC_EN("The cell number you entered is invalid")
    LC_END
};

u8 menu_select_ctx (SCtx *ctx, u8 action)
{
    if (action == APP_ENTER) {

        muvuku_pool_handle_t h = *current_pool_handle;

        if (h == NULL) {
            display_text(locale(lc_not_alloc), NULL);
            return APP_OK;
        }

        muvuku_pool_t *p = muvuku_pool_open(current_allocator, h);

        /* Read a cell number */
        u8 *res = get_input(
            locale(lc_cell_number_prompt),
                0, MAX_SMS_LENGTH, NULL, Q_GET_INPUT_DIGITS
        );

        /* Validate cell and set */
        if (res == NULL || res == ENULL) {
            goto exit;
        } else {
            /* Capture string and length, convert */
            muvuku_string_size_t len = res[0];
            res[len + 1] = '\0';
            muvuku_cell_t c = atoi(&res[2]);

            /* Validate cell number, save */
            if (muvuku_pool_address(p, c) != NULL) {
                cell = c;
                display_selected_cell(p, lc_switch_success);
            } else {
                display_text(locale(lc_invalid_cell_number), NULL);
            }
        }

        exit:
            muvuku_pool_close(p);
    }

    return APP_OK;
}

SNodeP menu_select_n = {
    lc_menu_select, menu_select_ctx
};


/* ----------------------------------------------------------------------*/


lc_char PROGMEM lc_menu_add[] = {
    LC_EN("Add New Data")
    LC_END
};

lc_char PROGMEM lc_no_cell[] = {
    LC_EN("No cell is currently selected")
    LC_END
};

lc_char PROGMEM lc_invalid_cell[] = {
    LC_EN("Failed to open the selected cell")
    LC_END
};

lc_char PROGMEM lc_string_prompt[] = {
    LC_EN("Please enter a string")
    LC_END
};

u8 menu_add_ctx (SCtx *ctx, u8 action)
{
    u8 rv = APP_OK;

    if (action == APP_ENTER) {

        muvuku_pool_handle_t h = *current_pool_handle;

        if (h == NULL) {
            display_text(locale(lc_not_alloc), NULL);
            return APP_OK;
        }

        if (cell == INVALID_CELL) {
            display_text(locale(lc_no_cell), NULL);
            return APP_OK;
        }

        muvuku_pool_t *p = muvuku_pool_open(current_allocator, h);

        muvuku_stringlist_t *l = muvuku_stringlist_open(
            p, muvuku_pool_address(p, cell)
        );

        if (l == NULL) {
            display_text(locale(lc_invalid_cell), NULL);
            goto exit;
        }

        /* Read a string */
        u8 *res = get_input(
            locale(lc_string_prompt),
                0, MAX_SMS_LENGTH, NULL, Q_GET_INPUT_ALPHABET
        );

        /* Add a string */
        if (res == NULL || res == ENULL) {
            goto exit;
        } else {
            muvuku_string_size_t len = res[0];
            res[len + 1] = '\0';
            muvuku_stringlist_add(l, &res[2], len);
        }

        exit:
            muvuku_pool_close(p);
    }

    return rv;
}

SNodeP menu_add_n = {
    lc_menu_add, menu_add_ctx
};


/* ----------------------------------------------------------------------*/


lc_char PROGMEM lc_menu_show[] = {
    LC_EN("Show All Data")
    LC_END
};


int show_callback(muvuku_stringlist_t *l,
                  char *src, size_t len, void *state) {

    u8 rv = TRUE;
    len = scalar_min(len, MAX_SMS_LENGTH);

    char *buf = (char *) malloc(len + 1);
    memset(buf, '\0', len + 1);

    muvuku_pool_read(l->pool, buf, src, len);

    if (display_text(buf, NULL) != APP_OK) {
        rv = FALSE;
    }

    free(buf);
    return rv;
};


u8 menu_show_ctx (SCtx *ctx, u8 action)
{
    u8 rv = APP_OK;

    if (action == APP_ENTER) {

        muvuku_pool_handle_t h = *current_pool_handle;

        if (h == NULL) {
            display_text(locale(lc_not_alloc), NULL);
            return APP_OK;
        }

        if (cell == INVALID_CELL) {
            display_text(locale(lc_no_cell), NULL);
            return APP_OK;
        }

        muvuku_pool_t *p = muvuku_pool_open(current_allocator, h);

        muvuku_stringlist_t *l = muvuku_stringlist_open(
            p, muvuku_pool_address(p, cell)
        );

        if (l == NULL) {
            display_text(locale(lc_invalid_cell), NULL);
            goto exit;
        }

        muvuku_stringlist_each(l, show_callback, NULL);

        exit:
            muvuku_pool_close(p);
    }

    return rv;
}

SNodeP menu_show_n = {
    lc_menu_show, menu_show_ctx
};



/* ----------------------------------------------------------------------*/


lc_char PROGMEM lc_menu_stat[] = {
    LC_EN("Statistics")
    LC_END
};

lc_char PROGMEM lc_stat_current[] = {
    LC_EN("Cells Allocated: ")
    LC_END
};

lc_char PROGMEM lc_stat_total[] = {
    LC_EN("Total Cells: ")
    LC_END
};

lc_char PROGMEM lc_stat_selected[] = {
    LC_EN("Selected Cell: ")
    LC_END
};

lc_char PROGMEM lc_stat_address[] = {
    LC_EN("Cell Address: 0x")
    LC_END
};

u8 menu_stat_ctx (SCtx *ctx, u8 action)
{
    if (action == APP_ENTER) {

        muvuku_pool_handle_t h = *current_pool_handle;

        if (h == NULL) {
            display_text(locale(lc_not_alloc), NULL);
            return APP_OK;
        }

        muvuku_pool_t *p = muvuku_pool_open(current_allocator, h);

        muvuku_pool_data_t *pool = malloc(sizeof(*pool));
        muvuku_pool_read(p, pool, p->pool, sizeof(*pool));

        size_t len = MUVUKU_PAGE_SIZE;
        char *output = (char *) malloc(len);
        memset(output, '\0', len);

        void *x = muvuku_pool_address(p, cell);
        char *r = sprints(output, locale(lc_stat_selected));

        r = sprintc(r, '#');
        r = sprinti(r, cell);
        r = sprintc(r, '\n');

        r = sprints(r, locale(lc_stat_address));
        r = sprint_hex16(r, (u16) x);
        r = sprintc(r, '\n');

        r = sprints(r, locale(lc_stat_current));
        r = sprinti(r, pool->item_count);
        r = sprintc(r, '\n');

        r = sprints(r, locale(lc_stat_total));
        r = sprinti(r, pool->item_limit);

        display_text(output, NULL);

        free(output);
        free(pool);
    }

    return APP_OK;
}

SNodeP menu_stat_n = {
    lc_menu_stat, menu_stat_ctx
};



/* ----------------------------------------------------------------------*/


/* Edge data:
    This structure contains all possible navigation paths. */

SEdgeP menu_edges_p[] = {

    { &menu_top_n, &menu_driver_n },
    { &menu_top_n, &menu_alloc_n },
    { &menu_top_n, &menu_free_n },
    { &menu_top_n, &menu_stat_n },
    { &menu_top_n, &menu_acquire_n },
    { &menu_top_n, &menu_release_n },
    { &menu_top_n, &menu_select_n },
    { &menu_top_n, &menu_add_n },
    { &menu_top_n, &menu_show_n },
    { NULL, NULL }
};


/* Top-level STK handler:
    This draws the top-level menu, using the spider library. */

void action_menu(void *data)
{
    SCtx *c = spider_init();
    muvuku_subsystem_init_pool();

    if (c == NULL)
        return;

    app_data_t *d = (app_data_t *) app_data();

    if (d != NULL) {
        eeprom->read(&app, d, sizeof(app));
    } else {
        d = (app_data_t *) eeprom->alloc(sizeof(*d), NULL);
        eeprom->zero(d, sizeof(*d));
        memset(&app, '\0', sizeof(app));
        reg_app_data(d);
    }

    c->n = (SNode *) &menu_top_n;
    c->eP = (SEdgeP *) &menu_edges_p;

    spider(c);
}


/* Entry point:
    The application starts here. */

void turbo_handler(u8 action, void *data)
{
    muvuku_subsystem_init_pool();

    cell = INVALID_CELL;

    flash = &muvuku_flash_allocator;
    eeprom = &muvuku_eeprom_allocator;

    app.flash_pool = NULL;
    app.eeprom_pool = NULL;

    current_allocator = eeprom;
    current_pool_handle = &app.eeprom_pool;

    switch (action)
    {
        case ACTION_APP_REGISTER: {
            reg_app_data(NULL);
            set_proc_8(PROC_8_CONFIG_SETUP, 0);
            break;
        }
        case ACTION_APP_UNREGISTER: {
            app_data_t *d = (app_data_t *) app_data();

            if (d != NULL) {
                eeprom->read(&app, d, sizeof(app));

                if (app.eeprom_pool != INVALID_CELL) {
                    muvuku_pool_t *p =
                        muvuku_pool_open(eeprom, app.eeprom_pool);

                    muvuku_pool_delete(p);
                }

                if (app.flash_pool != INVALID_CELL) {
                    muvuku_pool_t *p =
                        muvuku_pool_open(flash, app.flash_pool);

                    muvuku_pool_delete(p);
                }
            
                eeprom->free(d);
            }

            reg_app_data(NULL);
            break;
        }
        case ACTION_APP_INIT: {
            break;
        }
        case ACTION_INSERT_MENU: {
            set_menu_alpha(locale(lc_topmenu_name));
            insert_menu(locale(lc_menu_top));
            break;
        }
        case ACTION_MENU_SELECTION: {
            stk_thread(action_menu, data);
            break;
        }
        default: {
            break;
        }
    }
}

