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

#include "schema.h"
#include "prototype.h"
#include "util.h"


/* Forward declarations
    These are type-specific backends for schema_item_prompt. */

#ifndef _MUVUKU_PROTOTYPE

  static u8 schema_item_prompt_numeric(schema_item_t *i,
                                       schema_prompt_flags_t f);

  static u8 schema_item_prompt_string(schema_item_t *i,
                                      schema_prompt_flags_t f);

  static u8 schema_item_prompt_phone(schema_item_t *i,
                                     schema_prompt_flags_t f);

  #ifndef _SCHEMA_DISABLE_SELECT
    static u8 schema_item_prompt_select(schema_item_t *i,
                                        schema_prompt_flags_t f);
  #endif /* ! defined _SCHEMA_DISABLE_SELECT */

  static u8 schema_item_prompt_boolean(schema_item_t *i,
                                       schema_prompt_flags_t f);

#endif /* _MUVUKU_PROTOTYPE */


/* Internally-used types:
    These should not be used outside of this subsystem. */

typedef enum muvuku_unserialize_state {

    ST_BEGIN,
    ST_API_VERSION,
    ST_FORM_IDENTIFIER,
    ST_FIELD,
    ST_FIELD_ESCAPE,
    ST_ACCEPT,
    ST_REJECT

} muvuku_unserialize_state_t;


/* Standard strings:
    Boolean values -- yes and no. */

const u8 PROGMEM lc_yes_en[] = "Yes";
const u8 PROGMEM lc_no_en[] = "No";

const u8 PROGMEM lc_yes_fr[] = "Oui";
const u8 PROGMEM lc_no_fr[] = "Non";

const u8 PROGMEM lc_yes_es[] = "S\7";
const u8 PROGMEM lc_no_es[] = "No";

const u8 *PROGMEM lc_boolean_en[] = {
    lc_yes_en,
    lc_no_en
};

const u8 *PROGMEM lc_boolean_fr[] = {
    lc_yes_fr,
    lc_no_fr
};

const u8 *PROGMEM lc_boolean_es[] = {
    lc_yes_es,
    lc_no_es
};

const lc_list PROGMEM lc_boolean[] = {
    LC_EN_LIST((const u8 *) lc_boolean_en)
    LC_ES_LIST((const u8 *) lc_boolean_es)
    LC_FR_LIST((const u8 *) lc_boolean_fr)
    LC_END_LIST
};


/* Standard strings:
    Validation-related strings. */


lc_char PROGMEM lc_invalid[] = {
    LC_EN("Sorry, ")
    LC_FR("D\5sol\5, ")
    LC_ES("Lo sentimos, ")
    LC_END
};

lc_char PROGMEM lc_invalid_detail[] = {
    LC_EN("please revise and try again")
    LC_FR("r\5visez et r\5essayez")
    LC_ES("por favor verifique e intente de nuevo")
    LC_END
};


/**
 */
schema_info_t *schema_info_init(schema_info_t *o)
{
    o->api_version = 0;
    o->field_count = 0;

    return o;
}


/**
 */
schema_item_t *schema_item_init(schema_item_t *i, const char *caption,
                                u8 data_type, u8 min_length, u8 max_length)
{
    i->caption = caption;

    i->data_type = data_type;
    i->min_length = min_length;
    i->max_length = max_length;

    i->flags = 0;

    #ifndef _SCHEMA_DISABLE_TRIGGER
      i->trigger = NULL;
    #endif /* _SCHEMA_DISABLE_TRIGGER */
    
    #ifndef _SCHEMA_DISABLE_VALIDATE
      i->validate = NULL;
      i->validate_message = NULL;
    #endif /* _SCHEMA_DISABLE_VALIDATE */

    #ifndef _SCHEMA_DISABLE_CONDITION
      i->condition = NULL;
    #endif /* _SCHEMA_DISABLE_CONDITION */

    #ifndef _SCHEMA_DISABLE_SELECT
      i->select_keys = NULL;
      i->select_values = NULL;
      i->select_length = 0;
    #endif /* _SCHEMA_DISABLE_SELECT */

    return schema_item_zero(i);
}


/**
 */
schema_item_t *schema_item_new(const char *caption, u8 data_type,
                               u8 min_length, u8 max_length)
{
    schema_item_t *i = (schema_item_t *) xmalloc(sizeof(schema_item_t));
    schema_item_init(i, caption, data_type, min_length, max_length);

    return i;
}


/**
 */
schema_item_t *schema_item_zero(schema_item_t *i)
{
    i->validity = 0;
    i->string_value = NULL;

    #ifndef _SCHEMA_DISABLE_SLV
      i->string_value_slv = NULL;
    #endif /* ! defined _SCHEMA_DISABLE_SLV */

    i->value.string = NULL;
    i->value.msisdn = NULL;
    i->value.integer = 0;
    i->value.boolean = FALSE;

    #ifdef _MUVUKU_USE_FPU
        i->value.numeric = 0;
    #endif

    return i;
}


/**
 */
schema_item_t *schema_item_clear_result(schema_item_t *i)
{
    if (i->string_value) {
        free(i->string_value);
    }

    #ifndef _SCHEMA_DISABLE_SLV
      if (i->string_value_slv) {
          free(i->string_value_slv);
      }
    #endif /* ! defined _SCHEMA_DISABLE_SLV */

    if (i->data_type == TS_PHONE && i->value.msisdn != NULL) {
        free(i->value.msisdn);
    }

    return schema_item_zero(i);
}


/**
 */
u8 schema_item_compare_integer(schema_item_t *i, int val)
{
    if (i->data_type != TS_INTEGER && i->data_type != TS_SELECT) {
        return FALSE;
    }

    if (i->value.integer != val) {
        return FALSE;
    }

    return TRUE;
}

#ifndef _SCHEMA_DISABLE_CONDITION

/**
 */
schema_item_t *schema_item_set_condition(schema_item_t *i,
                                         schema_condition_t condition)
{
    i->condition = condition;
    return i;
}

#endif /* _SCHEMA_DISABLE_CONDITION */

#ifndef _SCHEMA_DISABLE_VALIDATE

/**
 */
schema_item_t *schema_item_set_validation(schema_item_t *i,
                                          schema_validation_t validate)
{
    i->validate = validate;
    return i;
}

#endif /* ! defined _SCHEMA_DISABLE_VALIDATE */

#ifndef _SCHEMA_DISABLE_TRIGGER

/**
 */
schema_item_t *schema_item_set_trigger(schema_item_t *i,
                                       schema_trigger_t trigger)
{
    i->trigger = trigger;
    return i;
}

#endif /* ! defined _SCHEMA_DISABLE_TRIGGER */

#ifndef _SCHEMA_DISABLE_VALIDATE

/**
 */
u8 schema_item_fail_validation(schema_item_t *i, u8 *message)
{
    i->validate_message = message;
    return FALSE;
}

#endif /* ! defined _SCHEMA_DISABLE_VALIDATE */

/**
 */
void schema_item_teardown(schema_item_t *i)
{
    schema_item_clear_result(i);

    if (i->caption) {
        i->caption = NULL;
    }
}


/**
 */
void schema_item_delete(schema_item_t *i)
{
    schema_item_teardown(i);
    free(i);
}


/**
 */
u8 schema_item_validate(schema_list_t *l, schema_item_t *i)
{
    if (!i->validate) {
        i->validity |= VL_IS_VALID;
    } else {
        if (i->validate(l, i)) {
          i->validity |= VL_IS_VALID;
        } else {
          i->validity &= ~VL_IS_VALID;
          return FALSE;
        }
    }

    return TRUE;
}

#ifndef _SCHEMA_DISABLE_TRIGGER

/**
 */
u8 schema_item_trigger(schema_list_t *l, schema_item_t *i)
{
    if (i->trigger) {
        return i->trigger(l, i);
    }

    return FALSE;
}

#endif /* ! defined _SCHEMA_DISABLE_TRIGGER */

#ifndef _SCHEMA_DISABLE_CONDITION

/**
 */
u8 schema_item_condition(schema_list_t *l, schema_item_t *i)
{
    if (i->condition) {
        return i->condition(l, i);
    }

    return TRUE;
}

#endif /* _SCHEMA_DISABLE_CONDITION */

#ifndef _MUVUKU_PROTOTYPE

/**
 */
u8 schema_item_prompt(schema_list_t *l,
                      schema_item_t *i, schema_prompt_flags_t f) {
    u8 rv;

    for (;;) {
        switch (i->data_type) {
            case TS_INTEGER:
                rv = schema_item_prompt_numeric(
                    i, (f & ~PR_DECIMAL_PORTION)
                );
                break;
            #ifdef _MUVUKU_USE_FPU
                case TS_NUMERIC:
                    rv = schema_item_prompt_numeric(
                        i, (f | PR_DECIMAL_PORTION)
                    );
                    break;
            #endif
            case TS_PHONE:
                rv = schema_item_prompt_phone(i, f);
                break;
            case TS_BOOLEAN:
                rv = schema_item_prompt_boolean(i, f);
                break;

            #ifndef _SCHEMA_DISABLE_SELECT
                case TS_SELECT:
                    rv = schema_item_prompt_select(i, f);
                    break;
            #endif /* ! defined _SCHEMA_DISABLE_SELECT */

            default:
            case TS_STRING:
                rv = schema_item_prompt_string(i, f);
                break;
        }

        if (rv == APP_OK) {

            #ifndef _SCHEMA_DISABLE_VALIDATE
              if (!schema_item_validate(l, i)) {
                  display_text(
                      (i->validate_message ?
                          i->validate_message : locale(lc_invalid_detail)),
                      locale(lc_invalid)
                  );
                  continue;
              }
            #endif /* ! defined _SCHEMA_DISABLE_VALIDATE */

            #ifndef _SCHEMA_DISABLE_TRIGGER
              schema_item_trigger(l, i);
            #endif /* _SCHEMA_DISABLE_TRIGGER */
        }

        break;
    }

    return rv;
}

#endif /* _MUVUKU_PROTOTYPE */


/**
 */
schema_item_t *schema_item_set(schema_item_t *i, const char *s, size_t len)
{
    schema_item_t *rv;

    switch (i->data_type) {
        case TS_INTEGER:
            rv = schema_item_set_numeric(i, s, len, PR_NORMAL);
            break;
        #ifdef _MUVUKU_USE_FPU
            case TS_NUMERIC:
                rv = schema_item_set_numeric(i, s, len, PR_DECIMAL_PORTION);
                break;
        #endif
        case TS_PHONE:
            rv = schema_item_set_phone(i, s, len, PR_NORMAL);
            break;
        case TS_BOOLEAN:
            rv = schema_item_set_boolean(i, atoi(s), PR_NORMAL);
            break;
        #ifndef _SCHEMA_DISABLE_SELECT
          case TS_SELECT:
              rv = schema_item_set_select(i, atoi(s), PR_NORMAL);
              break;
        #endif /* ! defined _SCHEMA_DISABLE_SELECT */
        default:
        case TS_STRING:
            rv = schema_item_set_string(i, s, len, PR_NORMAL);
            break;
    }

    return rv;
}


#ifndef _MUVUKU_PROTOTYPE

/**
 */
static u8 schema_item_prompt_generic(schema_item_t *i,
                                     schema_prompt_flags_t f,
                                     schema_set_function_t fn,
                                     unsigned int stk_input_flags)
{
    const char *string_value = i->string_value;

    /* Detect UCS-2 */
    #ifndef _SCHEMA_DISABLE_SLV
      if (rb(&(i->caption[0])) == 0x84) {
          stk_input_flags |= Q_GET_INPUT_UCS2;
          string_value = i->string_value_slv;
      }
    #endif /* ! defined _SCHEMA_DISABLE_SLV */

    /* Translate flags */
    if (i->flags & FL_NO_ECHO) {
        stk_input_flags |= Q_GET_INPUT_NO_ECHO;
    }

    /* Read a string */
    u8 *res = get_input(
        i->caption, i->min_length,
            i->max_length, string_value, stk_input_flags
    );
    
    /* Null-terminate string */
    if (res == NULL) {
        return APP_BACK;
    } else if (res == ENULL) {
        return APP_LEAVE;
    } else {
        if (res[0] == 0) {
            (*fn)(i, "", 1, f);
        } else {
            size_t len = res[0];
            char *result = &res[2];
    
            #ifndef _SCHEMA_DISABLE_SLV
              char *string_value_slv = NULL;
            #endif /* ! defined _SCHEMA_DISABLE_SLV */

            res[len + 1] = '\0';

            #ifndef _SCHEMA_DISABLE_SLV
              if (stk_input_flags & Q_GET_INPUT_UCS2) {

                  /* Add SLV header, convert to GSM */
                  string_value_slv = ucs2_slv_encode(result, len);
                  result = ucs2_slv_to_gsm(string_value_slv);
               
                  /* Update length and terminator */
                  len /= 2;
                  result[len] = '\0';
              }
            #endif /* ! defined _SCHEMA_DISABLE_SLV */

            (*fn)(i, result, len + 1, f);
            
            #ifndef _SCHEMA_DISABLE_SLV
              i->string_value_slv = string_value_slv;

              if (stk_input_flags & Q_GET_INPUT_UCS2) {
                  free(result);
              }
            #endif /* ! defined _SCHEMA_DISABLE_SLV */
        }
    }

    return APP_OK;

}

#endif /* _MUVUKU_PROTOTYPE */


/**
 */
schema_item_t *schema_item_set_numeric(schema_item_t *i, const char *v,
                                       size_t len, schema_prompt_flags_t f)
{
    schema_item_clear_result(i);

    #ifdef _MUVUKU_USE_FPU
        if (!(f & PR_DECIMAL_PORTION)) {
    #endif

    i->value.integer = atoi(v);

    #ifdef _MUVUKU_USE_FPU
        } else {
            i->value.numeric = atof(v);
        }
    #endif

    i->string_value = (unsigned char *) xmalloc(len);
    memcpy(i->string_value, v, len);

    return i;
}


#ifndef _MUVUKU_PROTOTYPE

/**
 */
static u8 schema_item_prompt_numeric(schema_item_t *i,
                                     schema_prompt_flags_t f)
{
    return schema_item_prompt_generic(
        i, f, &schema_item_set_numeric, Q_GET_INPUT_DIGITS
    );
}

#endif /* _MUVUKU_PROTOTYPE */


/**
 */
schema_item_t *schema_item_set_string(schema_item_t *i, const char *v,
                                      size_t len, schema_prompt_flags_t f)
{
    schema_item_clear_result(i);

    i->value.string = xmalloc(len);
    i->string_value = i->value.string;
    memcpy(i->value.string, v, len);

    return i;
}


#ifndef _MUVUKU_PROTOTYPE

/**
 */
static u8 schema_item_prompt_string(schema_item_t *i,
                                    schema_prompt_flags_t f)
{
    return schema_item_prompt_generic(
        i, f, &schema_item_set_string,
            (i->flags & FL_INPUT_DIGITS_ONLY ?
                Q_GET_INPUT_DIGITS : Q_GET_INPUT_ALPHABET)
    );
}

#endif /* _MUVUKU_PROTOTYPE */


/**
 */
schema_item_t *schema_item_set_phone(schema_item_t *i, const char *v,
                                     size_t len, schema_prompt_flags_t f)
{
    schema_item_clear_result(i);

    i->string_value = (u8 *) xmalloc(len);
    memcpy(i->string_value, v, len);
    i->value.msisdn = str2msisdn(i->string_value, MSISDN_ADN, MEM_R);

    return i;
}


#ifndef _MUVUKU_PROTOTYPE

/**
 */
static u8 schema_item_prompt_phone(schema_item_t *i, schema_prompt_flags_t f)
{
    return schema_item_prompt_generic(
        i, f, &schema_item_set_phone, Q_GET_INPUT_DIGITS
    );
}

#endif /* _MUVUKU_PROTOTYPE */


/**
 */
schema_item_t *schema_item_set_boolean(schema_item_t *i,
                                       unsigned v, schema_prompt_flags_t f)
{
    i->value.boolean = (
        (v == 1 ? TRUE : FALSE)
    );

    return i;
}


#ifndef _MUVUKU_PROTOTYPE

/**
 */
static u8 schema_item_prompt_boolean(schema_item_t *i,
                                     schema_prompt_flags_t f)
{
    /* Prompt for a yes/no answer */
    u8 *res = select_item(
        2, (const u8 **) locale_list(lc_boolean), i->caption,
            NULL, (i->value.boolean ? 1 : 2), Q_SELECT_ITEM_CHOICE
    );

    u8 idx = get_tag(res, T_ITEM_ID);

    /* Null-terminate string */
    if (idx <= 0) {
        return APP_BACK;
    } else {
        idx += 2; /* Skip tag, length */
        schema_item_set_boolean(i, res[idx], f);
    }

    return APP_OK;
}

#endif /* _MUVUKU_PROTOTYPE */


#ifndef _SCHEMA_DISABLE_SELECT

/**
 */
schema_item_t *schema_item_set_select(schema_item_t *i,
                                      unsigned v, schema_prompt_flags_t f)
{
    i->value.integer = (
        i->select_keys ?
            i->select_keys[v - 1] : v
    );

    return i;
}

#ifndef _MUVUKU_PROTOTYPE

/**
 */
static u8 schema_item_prompt_select(schema_item_t *i,
                                    schema_prompt_flags_t f)
{
    u16 n, selected_index = i->value.integer;

    /* Map field value to selection index:
        This is a linear search for the field's value; n is likely
        very small, so this should not be a performance problem.
        In fact, it's likely optimal for these small-sized lists. */

    if (i->select_keys) {
        for (n = 0; n < i->select_length; n++) {
            if (i->select_keys[n] == i->value.integer) {
                selected_index = n;
                break; /* Found */
            }
        }
    }

    if (selected_index <= 0) {
        selected_index = 1;
    }
 
    /* Broken STK select_item?
        On some really inexpensive phones, the STK select_item
        implementation either (a) doesn't show or (b) doesn't
        leave enough room for the actual question text. If we're
        on one of those phones, use an extra STK display_text to
        ensure that the question can be read by the handset user. */

    if (f & PR_BROKEN_STK_SELECT) {
        broken_stk_select_back:
            if (display_text(i->caption, NULL) != APP_OK) {
                return APP_BACK;
            }
    }

    /* Prompt for a list selection */
    u8 *res = select_item(
        i->select_length, (const u8 **) i->select_values,
            i->caption, NULL, selected_index, Q_SELECT_ITEM_CHOICE
    );

    u8 idx = get_tag(res, T_ITEM_ID);

    if (idx <= 0) {
        if (f & PR_BROKEN_STK_SELECT) {
            goto broken_stk_select_back;
        }
        return APP_BACK;
    } else {
        idx += 2; /* Skip tag, length */
        schema_item_set_select(i, res[idx], f);
    }

    return APP_OK;
}

#endif /* ! defined _MUVUKU_PROTOTYPE */
#endif /* ! defined _SCHEMA_DISABLE_SELECT */


/**
 */
schema_list_t *schema_list_new(const char *type, size_t count)
{
    int overflow = FALSE;
    size_t type_len = strlen(type) + 1;

    schema_list_t *l = (schema_list_t *) xmalloc(
        sizeof(schema_list_t)
    );

    /* Schema items: like calloc */
    size_t list_size = safe_multiply(
        sizeof(schema_item_t), count, &overflow
    );

    if (overflow) {
        return NULL;
    }

    /* Array of schema items */
    l->list = (schema_item_t *) xmalloc(list_size);

    /* Schema identifier */
    l->type_id = (u8 *) xmalloc(type_len);
    memcpy(l->type_id, type, type_len);

    return l;
};


/**
 */
void schema_list_clear_result(schema_list_t *l)
{
    schema_item_t *p = l->list; 

    for (;;) {
        schema_item_clear_result(p);

        if ((p->flags & FL_LIST_TERMINATOR))
            break;

        p++;
    };
};


/**
 */
void schema_list_teardown(schema_list_t *l)
{
    schema_item_t *p = l->list; 

    for (;;) {
        schema_item_teardown(p);

        if ((p->flags & FL_LIST_TERMINATOR))
            break;

        p++;
    };

    free(l->type_id);
    free(l->list);
};


/**
 */
void schema_list_delete(schema_list_t *l)
{
    schema_list_teardown(l);
    free(l);
};


/**
 */
size_t schema_list_count(schema_list_t *l, u8 valid_only)
{
    schema_item_t *p = l->list; 
    size_t rv = 0;

    for (;;) {
        if (!valid_only || (p->validity & VL_IS_VALID)) {
            rv++;
        }

        if ((p->flags & FL_LIST_TERMINATOR))
            break;

        p++;
    };

    return rv;
};


/**
 */
schema_item_t *schema_list_get(schema_list_t *l, unsigned int position)
{
    if (position >= schema_list_count(l, FALSE)) {
        return NULL;
    }

    return &l->list[position];
}


/**
 */
u8 schema_list_is_complete(schema_list_t *l)
{
    return (
        schema_list_count(l, TRUE) == schema_list_count(l, FALSE)
    );
}

/**
 */
u8 schema_list_is_empty(schema_list_t *l)
{
    return (schema_list_count(l, TRUE) == 0);
}


#ifndef _MUVUKU_PROTOTYPE

/**
 */
u8 schema_list_prompt(schema_list_t *l, schema_prompt_flags_t f)
{
    u8 last_direction = TRUE;
    schema_item_t *p = l->list;

    do {
        #ifndef _SCHEMA_DISABLE_CONDITION
          if (schema_item_condition(l, p)) {
        #endif /* _SCHEMA_DISABLE_CONDITION */
            switch (schema_item_prompt(l, p, f)) {
                case APP_BACK:
                    last_direction = FALSE;
                    goto resume;
                case APP_LEAVE:
                    return APP_LEAVE;
                default:
                case APP_OK:
                    last_direction = TRUE;
                    break;
            }
        #ifndef _SCHEMA_DISABLE_CONDITION
          } else {
              schema_item_clear_result(p);
              p->validity |= VL_IS_VALID;
              p->validity |= VL_IS_NULL;
          }
        #endif /* _SCHEMA_DISABLE_CONDITION */
      
        /* Termination condition */
        if ((p->flags & FL_LIST_TERMINATOR)) {
            break;
        }
        
        resume:
            p += (last_direction ? 1 : -1);

    } while (p >= l->list);

    return APP_OK;
};

#endif /* _MUVUKU_PROTOTYPE */


/**
 */
u8 *schema_list_serialize(schema_list_t *l, schema_flags_t filter)
{
    size_t len = MAX_SMS_LENGTH;
    size_t bufsz = MAX_SMS_FIELD_LENGTH;

    schema_item_t *i = l->list;

    /* Skip overflow checking:
        These sizes are predefined constants, not user input. */

    u8 *buf = (u8 *) xmalloc(bufsz + 1);
    u8 *rv = (u8 *) xmalloc(len + 1);
    u8 *p = rv;

    /* SMS API Version:
        The API version number begins each message,
        followed by the "magic" delimiter string (e.g. 1!). */

    /* Stringify SMS API version number */
    itoa(SMS_API_VERSION, buf, 10);

    /* Copy SMS API version to output */
    size_t version_len = strncpy_esc(
        bufsz + 1, p, buf, SMS_MAGIC_DELIMITER, SMS_ESCAPE
    );

    /* Advance to terminator */
    p += version_len - 1;

    /* Overwrite terminator and advance */
    *p++ = SMS_MAGIC_DELIMITER;
    len -= version_len;


    /* SMS Form Identifier:
        The form identified is a four-character code that uniquely
        identifies the sequence of fields used. This is appended
        to the SMS version using the magic delimiter (e.g. 1!PSMS!) */

    /* Copy form identifier to output */
    size_t header_len = strncpy_esc(
        bufsz + 1, p, l->type_id, SMS_MAGIC_DELIMITER, SMS_ESCAPE
    );

    /* Advance to terminator */
    p += header_len - 1;

    /* Overwrite terminator and advance */
    *p++ = SMS_MAGIC_DELIMITER;
    len -= header_len;


    /* Serialization begins here:
        From now on, we use the regular delimiter to
        append each serialized field to the output string. */

    /* While space is remaining in destination */
    while (len > 0) {

        u8 *field;
        size_t field_len;

        if (i->validity & VL_IS_VALID && !(i->validity & VL_IS_NULL)) {

            /* Convert field to string */
            switch (i->data_type) {
                case TS_SELECT:
                case TS_INTEGER:
                    itoa(i->value.integer, buf, 10);
                    field = buf;
                    break;
                #ifdef USE_FPU
                    case TS_NUMERIC:
                        dtostrf(i->value.numeric, 8, 2, buf);
                        field = buf;
                        break;
                #endif
                default:
                case TS_STRING:
                    field = i->string_value;
                    break;
                case TS_BOOLEAN:
                    buf[0] = (i->value.boolean ? '1' : '0'); buf[1] = '\0';
                    field = buf;
                    break;
                case TS_PHONE:
                    field = msisdn2str(i->value.msisdn, MSISDN_ADN, MEM_R);
                    break;
            }

            /* Copy field to result buffer */
            field_len = strncpy_esc(
                bufsz + 1, p, field, schema_item_delimiter(i), SMS_ESCAPE
            );

            /* Overwrite terminator */
            len -= (field_len - 1);
            p += (field_len - 1);

            /* Free resources if necessary */
            switch (i->data_type) {
                case TS_PHONE:
                    free(field);
                    break;
                default:
                    break;
            }
        }

        /* Stop on last field */
        if ((i->flags & FL_LIST_TERMINATOR)) {
            break;
        }

        /* Add delimiter */
        *p++ = schema_item_delimiter(i);
        len -= 1;

        i++;
    }

    /* Null-terminate return string */
    *p = '\0';

    free(buf);
    return rv;
};


/**
 */
u8 is_digit(const char c) {

    return (c <= '9' && c >= '0');
}


/**
 */
u8 is_alphanumeric(const char c) {

    return (
        is_digit(c) ||
            (c <= 'Z' && c >= 'A') || (c <= 'z' && c >= 'a')
    );
}

#ifdef _SCHEMA_PROVIDE_UNSERIALIZE
/**
 */
u8 schema_list_unserialize(schema_list_t *l, schema_info_t *o,
                           const char *s, size_t len, schema_flags_t filter)
{
    int i;
    schema_item_t *ip = l->list;
    size_t bufsz = MAX_SMS_FIELD_LENGTH;

    /* Skip overflow checking:
        These sizes are predefined constants, not user input. */

    u8 *buf = (u8 *) xmalloc(bufsz + 1);
    u8 *bufp = buf, *limit = buf + bufsz;

    if (o != NULL) {
        o->field_count = 0;
        o->api_version = 0;
        o->form_identifier = (u8 *) xmalloc(bufsz + 1);
    }

    muvuku_unserialize_state_t state = ST_BEGIN;

    #define _bounds_check_and_push() \
        do { \
            if (bufp >= limit) { \
                state = ST_REJECT; \
                break; \
            } \
            *bufp++ = s[i]; \
        } while (0)

    #define _push_completed_field() \
        do { \
            if (filter != FL_NONE && (ip->flags & filter) == FL_NONE) { \
                break; \
            } \
            *bufp = '\0'; \
            schema_item_set(ip, buf, bufp - buf + 1); \
            schema_item_validate(l, ip); \
            if (o != NULL) { \
                o->field_count++; \
            } \
        } while (0)


    for (i = 0; i < len; ++i) {

        switch (state) {

            /* First magic field: protocol version */
            case ST_BEGIN:
            case ST_API_VERSION:

                if (s[i] == SMS_MAGIC_DELIMITER) {

                    /* Null-terminate and parse API version */
                    if (o != NULL) {
                        *bufp = '\0';
                        o->api_version = atoi(buf);
                    }

                    /* Reset and go to next state */
                    bufp = buf;
                    state = ST_FORM_IDENTIFIER;

                } else if (is_digit(s[i])) {

                    _bounds_check_and_push();

                } else {

                    /* Invalid character */
                    state = ST_REJECT;
                }
                break;

            /* Second magic field: form identifier */
            case ST_FORM_IDENTIFIER:

                if (s[i] == SMS_MAGIC_DELIMITER) {

                    /* Copy form identifier to schema_info */
                    if (o != NULL) {
                        *bufp = '\0';
                        memcpy(o->form_identifier, buf, bufsz + 1);
                    }

                    /* Reset field buffer and go to next state */
                    bufp = buf;
                    state = ST_FIELD;

                } else if (is_alphanumeric(s[i])) {

                    _bounds_check_and_push();

                } else {
                    /* Invalid character */
                    state = ST_REJECT;
                }

                break;

            /* Ordinary field */
            case ST_FIELD:

                if (s[i] == SMS_ESCAPE) {

                    /* Start escape sequence */
                    state = ST_FIELD_ESCAPE;

                } else if (s[i] == schema_item_delimiter(ip)) {

                    /* Done with current field */
                    _push_completed_field();

                    /* Last available form field? */
                    if (ip->flags & FL_LIST_TERMINATOR) {
                        state = ST_ACCEPT;
                        break;
                    }

                    /* Next field */
                    ++ip;

                    /* Reset field buffer and go around */
                    bufp = buf;

                } else {
                    _bounds_check_and_push();
                }

                break;

            /* Inside of escape sequence */
            case ST_FIELD_ESCAPE:
                _bounds_check_and_push();
                break;

            case ST_ACCEPT:
                /* Additional input after end of form */
                state = ST_REJECT;
                break;

            default:
            case ST_REJECT:
                /* Sink state */
                break;
        }
    }

    if (bufp > buf && state == ST_FIELD) {

        /* Final field might not have delimiter:
            Make sure that the field gets pushed anyway. */

        _push_completed_field();
        state = ST_ACCEPT;
    }

    if (!(ip->flags & FL_LIST_TERMINATOR)) {
        state = ST_REJECT;
    }

    return (state == ST_ACCEPT);
}
#endif /* _SCHEMA_PROVIDE_UNSERIALIZE */

