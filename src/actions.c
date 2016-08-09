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

#include "pool.h"
#include "transport.h"
#include "actions.h"


/* Strings:
    These are used in user interface elements. */

const lc_char PROGMEM lc_ok_clear[] = {
    LC_EN("Message(s) successfully cleared")
    LC_FR("Messages effac\5s")
    LC_ES("Mensaje borrado")
    LC_END
};

const lc_char PROGMEM lc_ok_save[] = {
    LC_EN("Message successfully saved")
    LC_FR("Message sauveguard\5")
    LC_ES("Mensaje guardado")
    LC_UN(
"\x84\x08\x1c\x80\x9\x2e\x9\x47\x9\x38\x9\x47\x9\x1c\x0\x20\x9\x38\x9\x47\x9\x2d\x0\x20\x9\x2d\x9\x2f\x9\x4b\x0\x0"
    )
    LC_END
};

const lc_char PROGMEM lc_err_save[] = {
    LC_EN("Failed to save message: ")
    LC_FR("Ech\5s en sauvegardant: ")
    LC_ES("No pudo guardarse el mensaje: ")
    LC_END
};

const lc_char PROGMEM lc_err_store_pool[] = {
    LC_EN("Unable to open pooled storage subsystem")
    LC_FR("Incapable d'ouvrir la r\5serve de memoire")
    LC_ES("No se pueden abrir los mensajes guardados")
    LC_END
};

const lc_char PROGMEM lc_err_store_cell[] = {
    LC_EN("Unable to locate storage for this form")
    LC_FR("Memoire insuffisante pour ce rapport")
    LC_ES("Falta de espacio, enviar o borrar")
    LC_END
};

const lc_char PROGMEM lc_err_store_serialize[] = {
    LC_EN("Unable to prepare message for storage")
    LC_FR("Incapable de pr\5parer la memoire")
    LC_ES("No pudo prepararse para guardar")
    LC_END
};

const lc_char PROGMEM lc_err_send_serialize[] = {
    LC_EN("Unable to prepare message for transmission")
    LC_FR("Incapable de pr\5parer le message")
    LC_ES("No pudo prepararse para su transmisi\10n")
    LC_END
};

const lc_char PROGMEM lc_err_store_write[] = {
    LC_EN("Failure while writing data to flash memory")
    LC_FR("Incapable de sauvegarder les donn\5es")
    LC_ES("No pudo guardar datos en la memoria")
    LC_UN(
"\x84\x08\x8e\x80\x9\x2b\x9\x4b\x9\x28\x9\x2e\x9\x3e\x0\x20\x9\x2e\x9\x47\x9\x2e\x9\x4b\x9\x30\x9\x3f\x0\x20\x9\x28\x9\x2d\x9\xf\x9\x15\x9\x3e\x9\x32\x9\x47\x0\x20\x9\x2e\x9\x47\x9\x38\x9\x47\x9\x1c\x0\x20\x9\x38\x9\x47\x9\x2d\x0\x20\x9\x2d\x9\xf\x9\x28\x9\x64\x0\x20\x9\x24\x9\x4d\x9\x2f\x9\x38\x9\x48\x9\x32\x9\x47\x0\x20\x9\x2e\x9\x47\x9\x38\x9\x47\x9\x1c\x0\x20\x9\x5\x9\x39\x9\x3f\x9\x32\x9\x47\x9\x28\x9\x48\x0\x20\x9\x2a\x9\x20\x9\x3e\x9\x9\x9\x28\x9\x41\x0\x20\x9\x39\x9\x4b\x9\x32\x9\x3e\x9\x64\x0\x0"
    )
    LC_END
};

const lc_char PROGMEM lc_ok_send[] = {
    LC_EN("Messages successfully sent")
    LC_FR("Donn\5es envoy\5s avec succ\4s")
    LC_ES("Mensaje enviado")
    LC_UN(
"\x84\x08\x1a\x80\x9\x2e\x9\x47\x9\x38\x9\x47\x9\x1c\x0\x20\x9\x2a\x9\x20\x9\x3e\x9\x8\x9\x2f\x9\x4b\x0\x0"
    )
    LC_END
};

const lc_char PROGMEM lc_err_nothing_sent[] = {
    LC_EN("No messages available to send")
    LC_FR("Aucunes donn\5es a envoy\5r")
    LC_ES("No hay mensajes para enviar")
    LC_UN(
"\x84\x08\x4a\x80\x9\x2a\x9\x20\x9\x3e\x9\x9\x9\x28\x9\x47\x0\x20\x9\x2e\x9\x47\x9\x38\x9\x47\x9\x1c\x0\x20\x9\x28\x9\x2d\x9\xf\x9\x15\x9\x3e\x9\x32\x9\x47\x0\x20\x9\x2a\x9\x20\x9\x3e\x9\x9\x9\x28\x0\x20\x0\x20\x9\x5\x9\x38\x9\x2b\x9\x32\x0\x20\x9\x2d\x9\x2f\x9\x4b\x0\x0"
    )
    LC_END
};

const lc_char PROGMEM lc_err_send[] = {
    LC_EN("Failed to send message(s): ")
    LC_FR("Ech\5s d'envoie: ")
    LC_ES("No puedo enviar el mensaje: ")
    LC_END
};

const lc_char PROGMEM lc_err_send_sms[] = {
    LC_EN("Failure while sending SMS data")
    LC_FR("SMS non envoy\5")
    LC_ES("No pudo enviar los datos de SMS")
    LC_END
};

const lc_char PROGMEM lc_err_incomplete[] = {
    LC_EN("Form is incomplete; please revise and try again")
    LC_FR("Rapport incomplet. Revisez et re\5ssayez")
    LC_ES("La forma est\177 incompleta. Por favor revise e intente de nuevo")
    LC_UN(
"\x84\x08\x94\x80\x9\x2b\x9\x3e\x9\x30\x9\x2e\x0\x20\x9\x2a\x9\x41\x9\x30\x9\x3e\x0\x20\x9\x28\x9\x2d\x9\x30\x9\x47\x9\x15\x9\x4b\x9\x32\x9\x47\x0\x20\x9\x17\x9\x30\x9\x4d\x9\x26\x9\x3e\x0\x20\x9\x2e\x9\x47\x9\x38\x9\x47\x9\x1c\x0\x20\x9\x38\x9\x47\x9\x2d\x0\x20\x9\x2d\x9\xf\x9\x28\x9\x64\x0\x20\x9\x15\x9\x43\x9\x2a\x9\x2f\x9\x3e\x0\x20\x9\x2b\x9\x47\x9\x30\x9\x3f\x0\x20\x9\x2d\x9\x30\x9\x47\x9\x30\x0\x20\x9\x2a\x9\x4d\x9\x30\x9\x2f\x9\x3e\x9\x38\x0\x20\x9\x17\x9\x30\x9\x4d\x9\x28\x9\x41\x9\x39\x9\x4b\x9\x32\x9\x3e\x9\x64\x0\x0"
    )
    LC_END
};

const lc_char PROGMEM lc_confirm_clear[] = {
    LC_EN("Do you really want to erase your saved messages?")
    LC_FR("Voulez-vous vraiment effacer tout rapports sauvegard\5s?")
    LC_ES("\140Est\177 seguro de querer borrar sus mensajes guardados?")
    LC_END
};

#ifndef _DISABLE_STORAGE
    #ifdef _ENABLE_STORAGE_INFO

    const lc_char PROGMEM lc_show_count[] = {
        LC_EN("Forms: ")
        LC_FR("Rapports: ")
        LC_ES("Formas guardados:")
        LC_END
    };

    const lc_char PROGMEM lc_show_size[] = {
        LC_EN("Size: ")
        LC_FR("Utilis\5s: ")
        LC_ES("Utilisado: ")
        LC_END
    };

    const lc_char PROGMEM lc_show_remaining[] = {
        LC_EN("Remaining: ")
        LC_FR("Restants: ")
        LC_ES("Bytes restantes: ")
        LC_END
    };

    #endif /* _ENABLE_STORAGE_INFO */
#endif /* !_DISABLE_STORAGE */


/**
 * @name muvuku_send_state
 */
struct muvuku_send_state {

    unsigned int size;
    unsigned int count;
    schema_list_t *settings;
};


#ifndef _DISABLE_STORAGE
    #ifdef _ENABLE_STORAGE_INFO

    /**
     * @name _muvuku_action_show_one
     */
    static int _muvuku_action_show_one(muvuku_stringlist_t *sl,
                                       char *src, size_t len, void *ptr) {

        struct muvuku_send_state *state = (struct muvuku_send_state *) ptr;

        state->count++;
        state->size += len;

        return TRUE;
    }


    /**
     * @name muvuku_action_show
     */
    unsigned int muvuku_action_show(muvuku_settings_t *s,
                                    schema_list_t *settings,
                                    schema_list_t *l) {
        size_t len = 64;
        char *buffer = xmalloc(len);

        muvuku_pool_t *p = muvuku_storage_open(s);
        struct muvuku_send_state state = { 0, 0, settings };

        if (!p) {
            display_text(locale(lc_err_store_pool), locale(lc_err_send));
            goto exit;
        }

        size_t cell_size = muvuku_pool_cell_size(p);
        void *ptr = muvuku_pool_address(p, muvuku_storage_retrieve(s, p, l));

        muvuku_stringlist_t *sl = muvuku_stringlist_open(p, ptr);

        if (!sl) {
            display_text(locale(lc_err_store_cell), locale(lc_err_send));
            goto exit_pool;
        }

        if (!muvuku_stringlist_each(sl, _muvuku_action_show_one, &state)) {
            goto exit_stringlist;
        }

        memzero(buffer, len);

        /* Build status string */
        char *r = sprints(buffer, locale(lc_show_count));
        r = sprinti(r, state.count);
        r = sprintc(r, '\n');
        
        r = sprints(r, locale(lc_show_size));
        r = sprinti(r, state.size);
        r = sprintc(r, '\n');

        r = sprints(r, locale(lc_show_remaining));
        r = sprinti(r, cell_size - state.size);
        r = sprintc(r, '\n');

        /* Display */
        display_text(buffer, NULL);

        exit_stringlist:
            muvuku_stringlist_close(sl);

        exit_pool:
            muvuku_pool_close(p);

        exit:
            free(buffer);
            return state.count;
    }

    #endif /* _ENABLE_STORAGE_INFO */
#endif /* !_DISABLE_STORAGE */


/**
 * @name _muvuku_action_send_one
 */
static int _muvuku_action_send_one(muvuku_stringlist_t *sl,
                                   char *src, size_t len, void *ptr) {

    u8 rv = FALSE;
    len = scalar_min(len, MAX_SMS_LENGTH);

    char *buf = (char *) xmalloc(len + 1);
    memset(buf, '\0', len + 1);

    muvuku_pool_read(sl->pool, buf, src, len);
    struct muvuku_send_state *state = (struct muvuku_send_state *) ptr;

    if (!muvuku_send_sms(buf, state->settings)) {
        goto exit;
    }

    rv = TRUE;
    state->count++;
    state->size += len;

    exit:
        free(buf);
        return rv;
}


/**
 * @name muvuku_action_send
 */
unsigned int muvuku_action_send(muvuku_settings_t *s,
                                schema_list_t *settings, schema_list_t *l) {

    muvuku_pool_t *p = muvuku_storage_open(s);
    struct muvuku_send_state state = { 0, 0, settings };

    if (!p) {
        display_text(locale(lc_err_store_pool), locale(lc_err_send));
        goto exit;
    }

    void *ptr = muvuku_pool_address(p, muvuku_storage_retrieve(s, p, l));
    muvuku_stringlist_t *sl = muvuku_stringlist_open(p, ptr);

    if (!sl) {
        display_text(locale(lc_err_store_cell), locale(lc_err_send));
        goto exit_pool;
    }

    if (!muvuku_stringlist_each(sl, _muvuku_action_send_one, &state)) {
        goto exit_stringlist;
    }

    /* Success */
    muvuku_stringlist_init(p, ptr);

    if (state.count > 0) {
        display_text(locale(lc_ok_send), NULL);
    } else {
        display_text(locale(lc_err_nothing_sent), NULL);
    }
    
    exit_stringlist:
        muvuku_stringlist_close(sl);

    exit_pool:
        muvuku_pool_close(p);

    exit:
        return state.count;
}


/**
 * @name muvuku_action_save_explicit
 */
unsigned int muvuku_action_save_explicit(muvuku_settings_t *s,
                                         schema_list_t *settings, 
                                         schema_list_t *l) {
    if (schema_list_is_complete(l)) {
        return muvuku_action_save(s, settings, l, FALSE);
    }

    display_text(locale(lc_err_incomplete), NULL);
    return FALSE;
}


/**
 * @name muvuku_action_save_explicit
 */
unsigned int muvuku_action_save_implicit(muvuku_settings_t *s,
                                         schema_list_t *settings, 
                                         schema_list_t *l) {
    if (!schema_list_is_empty(l)) {
        if (!schema_list_is_complete(l)) {
            display_text(locale(lc_err_incomplete), NULL);
            return FALSE;
        } else {
            return muvuku_action_save(s, settings, l, TRUE);
        }
    }

    return TRUE;
}


/**
 * @name muvuku_action_save
 */
unsigned int muvuku_action_save(muvuku_settings_t *s,
                                schema_list_t *settings, 
                                schema_list_t *l, u8 silent_success) {

    unsigned int rv = FALSE;
    muvuku_pool_t *p = muvuku_storage_open(s);

    if (!p) {
        display_text(locale(lc_err_store_pool), locale(lc_err_save));
        goto exit;
    }

    muvuku_stringlist_t *sl = muvuku_stringlist_open(
        p, muvuku_pool_address(p, muvuku_storage_retrieve(s, p, l))
    );

    if (!sl) {
        display_text(locale(lc_err_store_cell), locale(lc_err_save));
        goto exit_pool;
    }

    char *sms = schema_list_serialize(l, FL_NONE);

    if (!sms) {
        display_text(locale(lc_err_store_serialize), locale(lc_err_save));
        goto exit_stringlist;
    }

    if (!muvuku_stringlist_add(sl, sms, strlen(sms) + 1)) {
        display_text(locale(lc_err_store_write), NULL);
        goto exit_unserialize;
    }

    rv = TRUE;
    schema_list_clear_result(l);

    if (!silent_success) {
        display_text(locale(lc_ok_save), NULL);
    }

    exit_unserialize:
        free(sms);

    exit_stringlist:
        muvuku_stringlist_close(sl);

    exit_pool:
        muvuku_pool_close(p);

    exit:
        return rv;
}


/**
 * @name muvuku_action_clear
 */
unsigned int muvuku_action_clear(muvuku_settings_t *s,
                                 schema_list_t *settings, schema_list_t *l) {

    if (display_text(locale(lc_confirm_clear), NULL) != APP_OK) {
        return FALSE;
    }

    muvuku_pool_t *p = muvuku_storage_open(s);

    if (!p) {
        display_text(locale(lc_err_store_pool), NULL);
        return FALSE;
    }

    void *ptr = muvuku_pool_address(p, muvuku_storage_retrieve(s, p, l));

    muvuku_stringlist_init(p, ptr);

    display_text(locale(lc_ok_clear), NULL);
    muvuku_pool_close(p);

    return TRUE;
}


/**
 * @name muvuku_action_send_without_save
 */
unsigned int muvuku_action_send_without_save(muvuku_settings_t *s,
                                             schema_list_t *settings,
                                             schema_list_t *l) {
    unsigned int rv = FALSE;

    if (!schema_list_is_complete(l)) {
        display_text(locale(lc_err_incomplete), NULL);
        return rv;
    }

    char *sms = schema_list_serialize(l, FL_NONE);

    if (!sms) {
        display_text(locale(lc_err_send_serialize), locale(lc_err_send));
        return rv;
    }

    if (!muvuku_send_sms(sms, settings)) {
        display_text(locale(lc_err_send_sms), locale(lc_err_send));
        goto exit_unserialize;
    }

    rv = TRUE;
    display_text(locale(lc_ok_send), NULL);

    exit_unserialize:
        free(sms);
        return rv;
}


