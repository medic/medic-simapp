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

#ifndef __MUVUKU_SCHEMA_H__
#define __MUVUKU_SCHEMA_H__

#include "bladox.h"
#include "prototype.h"

#include "util.h"
#include "string.h"
#include "pool.h"


#define TS_INTEGER  (1)
#define TS_NUMERIC  (2)
#define TS_STRING   (3)
#define TS_BOOLEAN  (4)
#define TS_PHONE    (5)
#define TS_SELECT   (6)

typedef u8 schema_flags_t;
typedef u8 schema_validity_t;

#define FL_NONE                 ((schema_flags_t) 0)    /* 0 */
#define FL_LIST_TERMINATOR      ((schema_flags_t) 1)    /* 1 */
#define FL_UNIQUE_KEY           ((schema_flags_t) 2)    /* 2 */
#define FL_PRESERVE_VALUE       ((schema_flags_t) 4)    /* 3 */
#define FL_SPECIAL_DELIMITER    ((schema_flags_t) 8)    /* 4 */
#define FL_NO_ECHO              ((schema_flags_t) 16)   /* 5 */
#define FL_INPUT_DIGITS_ONLY    ((schema_flags_t) 32)   /* 6 */

#define VL_IS_VALID             ((schema_validity_t) 1) /* 1 */
#define VL_IS_NULL              ((schema_validity_t) 2) /* 2 */

#define MAX_SMS_LENGTH          (160)
#define MAX_SMS_FIELD_LENGTH    (64)
#define SMS_DELIMITER           ('#')
#define SMS_MAGIC_DELIMITER     ('!')
#define SMS_ESCAPE              ('\\')
#define SMS_API_VERSION         (1)


/* Flags passed to schema_item_prompt:
    These flags affect the rendering code, but are not
    intrinsic attributes of the schema_list_t / question.
    The data type for these flags is schema_prompt_flags_t. */

typedef u16 schema_prompt_flags_t;

#define PR_NORMAL ((schema_prompt_flags_t) 0)               /* 0 */
#define PR_DECIMAL_PORTION ((schema_prompt_flags_t) 1)      /* 1 */
#define PR_BROKEN_STK_SELECT ((schema_prompt_flags_t) 2)    /* 2 */


extern const lc_list lc_boolean[];


typedef union
{
    u8 *string;
    u8 *msisdn;
    u8 boolean;
    int integer;
    #ifdef _MUVUKU_USE_FPU
        float numeric;
    #endif /* defined _MUVUKU_USE_FPU */

} __attribute__((packed)) schema_value_t;


struct schema_item;
struct schema_list;

typedef u8 (*schema_condition_t)(
    struct schema_list *, struct schema_item *
);

typedef u8 (*schema_validation_t)(
    struct schema_list *, struct schema_item *
);

typedef u8 (*schema_trigger_t)(
    struct schema_list *, struct schema_item *
);


/* Packed to conserve memory */
typedef struct schema_item
{
    u8 flags;
    u8 validity;
    u8 data_type;
    u8 min_length;
    u8 max_length;
    u8 *string_value;
    #ifndef _SCHEMA_DISABLE_SLV
      u8 *string_value_slv;
    #endif /* ! defined _SCHEMA_DISABLE_SLV */
    #ifndef _SCHEMA_DISABLE_SPECIAL_DELIMITERS
      u8 special_delimiter;
    #endif /* !defined _SCHEMA_DISABLE_SPECIAL_DELIMITERS */
    const char *caption;
    #ifndef _SCHEMA_DISABLE_SELECT
      u8 select_length;
      char **select_values;
      const u8 *select_keys;
    #endif /* ! defined _SCHEMA_DISABLE_SELECT */
    const u8 *validate_message;
    #ifndef _SCHEMA_DISABLE_CONDITION
      schema_condition_t condition;
    #endif /* ! defined _SCHEMA_DISABLE_CONDITION */
    #ifndef _SCHEMA_DISABLE_VALIDATE
      schema_validation_t validate;
    #endif /* ! defined _SCHEMA_DISABLE_VALIDATE */
    #ifndef _SCHEMA_DISABLE_TRIGGER
      schema_trigger_t trigger;
    #endif /* ! defined _SCHEMA_DISABLE_TRIGGER */
    schema_value_t value;

} __attribute__((packed)) schema_item_t;


typedef struct schema_list {

    u8 *type_id;
    schema_item_t *list;

} __attribute((packed)) schema_list_t;


typedef struct schema_info {

    u8 api_version;
    u8 field_count;
    char *form_identifier;

} __attribute__((packed)) schema_info_t;


schema_info_t *schema_info_init(schema_info_t *o);

schema_item_t *schema_item_new(const char *caption, u8 data_type,
                               u8 min_length, u8 max_length);

schema_item_t *schema_item_init(schema_item_t *i,
                                const char *caption, u8 data_type,
                                u8 min_length, u8 max_length);

schema_item_t *schema_item_clear_result(schema_item_t *i);

schema_item_t *schema_item_set_condition(
    schema_item_t *i, schema_condition_t condition
);

schema_item_t *schema_item_set_validation(
    schema_item_t *i, schema_validation_t validate
);

schema_item_t *schema_item_set_trigger(
    schema_item_t *i, schema_trigger_t trigger
);


u8 schema_item_condition(schema_list_t *l, schema_item_t *i);

u8 schema_item_trigger(schema_list_t *l, schema_item_t *i);

u8 schema_item_validate(schema_list_t *l, schema_item_t *i);

u8 schema_item_fail_validation(schema_item_t *i, u8 *message);

u8 schema_item_compare_integer(schema_item_t *i, int val);


void schema_item_teardown(schema_item_t *i);

void schema_item_delete(schema_item_t *i);

u8 schema_item_prompt(schema_list_t *l,
                      schema_item_t *i, schema_prompt_flags_t f);


typedef schema_item_t * (*schema_set_function_t)(
    schema_item_t *i, const char *v,
        size_t len, schema_prompt_flags_t f
);


schema_item_t *schema_item_set(schema_item_t *i, const char *s, size_t len);


schema_item_t *schema_item_set_numeric(schema_item_t *i, const char *v,
                                       size_t len, schema_prompt_flags_t f);

schema_item_t *schema_item_set_string(schema_item_t *i, const char *v,
                                      size_t len, schema_prompt_flags_t f);

schema_item_t *schema_item_set_phone(schema_item_t *i, const char *v,
                                     size_t len, schema_prompt_flags_t f);

schema_item_t *schema_item_set_boolean(schema_item_t *i,
                                       unsigned v, schema_prompt_flags_t f);

#ifndef _SCHEMA_DISABLE_SELECT
  schema_item_t *schema_item_set_select(schema_item_t *i,
                                        unsigned v, schema_prompt_flags_t f);
#endif /* ! defined _SCHEMA_DISABLE_SELECT */

schema_item_t *schema_item_zero(schema_item_t *i);


#ifdef  _SCHEMA_DISABLE_SPECIAL_DELIMITERS
  #define schema_item_delimiter(i) (SMS_DELIMITER)
#else
  #define schema_item_delimiter(i) \
      (((i) != NULL && ((i)->flags & FL_SPECIAL_DELIMITER)) ? \
          (i)->special_delimiter : SMS_DELIMITER)
#endif /* !defined _SCHEMA_DISABLE_SPECIAL_DELIMITERS */


schema_list_t *schema_list_new(const char *form_identifier, size_t size);

schema_item_t *schema_list_get(schema_list_t *l, unsigned int position);

u8 schema_list_prompt(schema_list_t *l, schema_prompt_flags_t f);

void schema_list_clear_result(schema_list_t *l);

u8 *schema_list_serialize(schema_list_t *l, schema_flags_t filter);

#if 0
u8 schema_list_unserialize(schema_list_t *l, schema_info_t *o,
                           const char *s, size_t len, schema_flags_t filter);
#endif

void schema_list_teardown(schema_list_t *l);

void schema_list_delete(schema_list_t *l);

size_t schema_list_count(schema_list_t *l, u8 valid_only);

u8 schema_list_is_complete(schema_list_t *l);

u8 schema_list_is_empty(schema_list_t *l);


#define SCHEMA_BEGIN(_name, _type_id, _size) \
    schema_list_t *_name = schema_list_new(_type_id, (size_t) (_size)); \
    do { \
        schema_item_t *__schema_ptr = (_name)->list;

#define SCHEMA_ITEM(args...) \
    schema_item_init(__schema_ptr, ##args); \
    __schema_ptr++

#ifndef _SCHEMA_DISABLE_SELECT
  #define SCHEMA_ITEM_SELECT(_size, _keys, _values) \
      __schema_ptr--; \
      __schema_ptr->select_keys = (_keys); \
      __schema_ptr->select_values = (char **) (_values); \
      __schema_ptr->select_length = (size_t) (_size); \
      __schema_ptr++
#endif /* ! defined _SCHEMA_DISABLE_SELECT */

#ifndef _SCHEMA_DISABLE_CONDITION
  #define SCHEMA_ITEM_CONDITION(args...) \
      __schema_ptr--; \
      schema_item_set_condition(__schema_ptr, ##args); \
      __schema_ptr++
#endif /* ! defined _SCHEMA_DISABLE_CONDITION */

#ifndef _SCHEMA_DISABLE_VALIDATE
  #define SCHEMA_ITEM_VALIDATE(args...) \
      __schema_ptr--; \
      schema_item_set_validation(__schema_ptr, ##args); \
      __schema_ptr++
#endif /* ! defined _SCHEMA_DISABLE_VALIDATE */

#ifndef _SCHEMA_DISABLE_TRIGGER
  #define SCHEMA_ITEM_TRIGGER(args...) \
      __schema_ptr--; \
      schema_item_set_trigger(__schema_ptr, ##args); \
      __schema_ptr++
#endif /* ! defined _SCHEMA_DISABLE_TRIGGER */

#define SCHEMA_ITEM_FLAGS(_flags) \
    __schema_ptr--; \
    __schema_ptr->flags |= _flags; \
    __schema_ptr++

#ifndef _SCHEMA_DISABLE_SPECIAL_DELIMITERS
  #define SCHEMA_ITEM_DELIMITER(_delimiter) \
      __schema_ptr--; \
      __schema_ptr->special_delimiter = _delimiter; \
      __schema_ptr->flags |= FL_SPECIAL_DELIMITER; \
      __schema_ptr++
#endif /* !defined _SCHEMA_DISABLE_SPECIAL_DELIMITERS */

#define SCHEMA_END() \
        __schema_ptr--; \
        __schema_ptr->flags |= FL_LIST_TERMINATOR; \
    } while (0)


#endif  /* __MUVUKU_SCHEMA_H__ */

