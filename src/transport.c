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

#include "transport.h"
#include "schema.h"


/* Strings for send/receive user interface */

lc_char PROGMEM lc_cannot_send_1[] = {
    LC_EN("Failed to send form, but your answers remain saved.")
    LC_FR("Ech\5s d'envoie, rapport non envoy\5.")
    LC_ES("No pudo enviar la forma, forma esta guardado")
    LC_UN("\x84\x08\x54\x80\x9\x2e\x9\x47\x9\x38\x9\x47\x9\x1c"
          "\x0\x20\x9\x2a\x9\x20\x9\x3e\x9\x9\x9\x28\x0\x20"
          "\x9\x5\x9\x38\x9\x2b\x9\x32\x0\x20\x9\x2d\x9\x2f"
          "\x9\x4b\x0\x20\x9\x24\x9\x30\x0\x20\x9\x2e\x9\x47"
          "\x9\x38\x9\x47\x9\x1c\x0\x20\x9\x38\x9\x47\x9\x2d\x0\x20"
          "\x9\x2d\x9\xf\x9\x15\x9\x4b\x0\x20\x9\x1b\x9\x64\x0\x0")
    LC_END
};

lc_char PROGMEM lc_cannot_send_2[] = {
    LC_EN("Once you have a signal, please select 'Transmit' again.")
    LC_FR("Assurez vous que vous avez acc\4s au r\5seau et re\5ssayez.")
    LC_ES("Verifique que tiene se\175al e intente de nuevo.")
    LC_UN("\x84\x08\x52\x80\x9\x2b\x9\x4b\x9\x28\x9\x15"
          "\x9\x4b\x0\x20\x9\x1f\x9\x3e\x9\x35\x9\x30"
          "\x0\x20\x9\x6\x9\xf\x9\x2a\x9\x1b\x9\x3f"
          "\x0\x20\x9\x2a\x9\x41\x9\x28\x0\x3a\x0\x20"
          "\x9\x2e\x9\x47\x9\x38\x9\x47\x9\x1c\x0\x20"
          "\x9\x2a\x9\x20\x9\x3e\x9\x9\x9\x28\x9\x41"
          "\x0\x20\x9\x39\x9\x4b\x9\x32\x9\x3e\x9\x64\x0\x0")
    LC_END
};


lc_char PROGMEM lc_cannot_send_3[] = {
    LC_EN("If you continue to experience problems please contact your supervisor.")
    LC_FR("Si vous continuez a avoir des probl\4mes contactez votre superviseur.")
    LC_ES("Si continua teniendo problemas, por favor contacte al administrador.")
    LC_UN("\x84\x08\xa2\x80\x9\x2b\x9\x4b\x9\x28\x9\x15\x9\x4b\x0\x20\x9\x1f"
          "\x9\x3e\x9\x35\x9\x30\x0\x20\x9\x6\x9\xf\x9\x2a\x9\x1b\x9\x3f\x0\x20"
          "\x9\x2a\x9\x28\x9\x3f\x0\x20\x9\x2e\x9\x47\x9\x38\x9\x47\x9\x1c\x0\x20"
          "\x9\x2a\x9\x20\x9\x3e\x9\x9\x9\x28\x0\x20\x9\x5\x9\x38\x9\x2b\x9\x32"
          "\x0\x20\x9\x2d\x9\xf\x9\x2e\x9\x3e\x0\x20\x9\x38\x9\x4d\x9\x35\x9\x3e"
          "\x9\x38\x9\x4d\x9\x25\x9\x4d\x9\x2f\x0\x20\x9\x38\x9\x2\x9\x38\x9\x4d"
          "\x9\x25\x9\x3e\x9\x2e\x9\x3e\x0\x20\x9\x38\x9\x2e\x9\x4d\x9\x2a\x9\x30"
          "\x9\x4d\x9\x15\x0\x20\x9\x17\x9\x30\x9\x4d\x9\x28\x9\x41\x9\x39\x9\x4b"
          "\x9\x32\x9\x3e\x9\x64\x0\x0")
    LC_END
};

/* Support for sending SMS messages:
    This function serializes a `schema_list_t` and transmits it via
    SMS to the phone number stored in `schema_settings`, after applying
    the proper checks and text encoding steps. Once the send operation
    concludes, this function checks the result buffer to ensure that
    message transmission was successful. If the send wasn't successful,
    a detailed error message is displayed, and FALSE is returned. */

u8 muvuku_send_sms(char *s, schema_list_t *settings)
{
    u8 rv = FALSE;

    if (!settings->list->value.msisdn) {
        goto error;
    }

    size_t len = dcs_78(s, strlen(s), DCS_8_TO_7);

    u8 *result = send_sms(
        s, len, settings->list->value.msisdn,
            MSISDN_ADN, 0x00, 0x00, NULL, NULL
    );

    /* Find T_RESULT tag:
        Then examine the third byte in the result array. This byte
        will be set to zero if the previous operation was successful. */

    u8 tag = get_tag(result, T_RESULT); 

    if (tag != 0 && result[tag + 2] == 0x00) {
        rv = TRUE;
        goto success;
    }

    error:
        display_text(locale(lc_cannot_send_1), NULL);
        display_text(locale(lc_cannot_send_2), NULL);
        display_text(locale(lc_cannot_send_3), NULL);

    success:
        return rv;
}

