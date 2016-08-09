/**
 * Test cases for Muvuku subsystems (prototype mode)
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

#include "muvuku.h"
#include "bladox.h"
#include "prototype.h"


/* Reserve some memory to test in:
    This space is used throughout the tests. */

char PROGMEM reserved[MUVUKU_PAGE_SIZE * 64] = { 0 };


/* Test cases:
    These attempt to exercise Muvuku's various subsystems
    using the normal gcc and a regular Unix-like machine.
    This prototyping-mode provides for the use of gdb, and
    can potentially save us a lot of phone-fiddling effort. */


#define assert(_expr, _message) \
    do { \
        _assert((#_expr), (_expr), __FILE__, __LINE__, (_message)); \
    } while (0)


#define assert_string(_s, _value, _message) \
    do { \
        assert(strcmp((_s), (_value)) == 0, (_message)); \
    } while (0)


void _assert(char *expr, u8 value, char *file, unsigned line, char *msg) {

    if (!value) {
        printf(
            "  [-] Test failure: expression `%s' at %s:%d\n",
                expr, file, line
        );
        printf("  [x] Failed test was `%s`\n", msg);
        exit(~0);
    }

    printf("  [+] Test successful: `%s` at %s:%d\n", msg, file, line);
}


char *itoa(int i, char *buf, int base) {

    snprintf(buf, 16, "%d", i);
    return buf;
}


/** @name test_simple_serialization */


void test_simple_serialization() {

    puts("[>] test_simple_serialization");

    schema_info_t o;
    schema_info_init(&o);

    SCHEMA_BEGIN(l, "MUVX", 10);
        SCHEMA_ITEM("i1", TS_INTEGER, 1, 4);
        SCHEMA_ITEM("s2", TS_STRING, 1, 4);
        SCHEMA_ITEM("i3", TS_INTEGER, 2, 4);
            SCHEMA_ITEM_DELIMITER('-');
        SCHEMA_ITEM("i4", TS_INTEGER, 2, 4);
        SCHEMA_ITEM("s5", TS_STRING, 1, 32);
            SCHEMA_ITEM_DELIMITER('/');
    SCHEMA_END();

    schema_item_t *i = l->list;
    assert(i != NULL, "Created schema_list_t successfully");

    schema_item_set(i, "1234", 5);
    schema_item_validate(l, i++);
    
    schema_item_set(i, "I am a string", 14);
    schema_item_validate(l, i++);

    schema_item_set(i, "13", 3);
    schema_item_validate(l, i++);

    schema_item_set(i, "14", 3);
    schema_item_validate(l, i++);

    schema_item_set(i, "This is a test of a long string.", 33);
    schema_item_validate(l, i++);

    char *s = schema_list_serialize(l, FL_NONE);

    assert_string(
        s, "1!MUVX!1234#I am a string#13-14#This is a test of a long string.",
            "Serialized string matches"
    );

    schema_list_clear_result(l);
    i = l->list;

    assert(i[0].value.integer == 0, "Field #1 cleared");
    assert(i[1].value.string == NULL, "Field #2 cleared");
    assert(i[2].value.integer == 0, "Field #3 cleared");
    assert(i[3].value.string == NULL, "Field #4 cleared");

    u8 ok = schema_list_unserialize(l, &o, s, strlen(s), FL_NONE);

    assert(ok == TRUE, "Unserialization successful");
    assert(o.api_version == 1, "API version set correctly");
    assert(o.field_count == 5, "Field count set correctly");
    assert_string(o.form_identifier, "MUVX", "Form code set correctly");

    free(s);

    s = "1!MUVX!1234#S2#22-33#S4/";
    schema_list_clear_result(l);
    schema_info_init(&o);

    ok = schema_list_unserialize(l, &o, s, strlen(s), FL_NONE);
    assert(ok == TRUE, "Unserialization successful");

    assert(i[0].value.integer == 1234, "Field #1 set as expected");
    assert_string("S2", i[1].value.string, "Field #2 set as expected");
    assert(i[2].value.integer == 22, "Field #3 set as expected");
    assert(i[3].value.integer == 33, "Field #4 set as expected");
    assert_string("S4", i[4].value.string, "Field #5 set as expected");

    s = "1!MUVX!1#2#3#";
    schema_list_clear_result(l);
    schema_info_init(&o);

    ok = schema_list_unserialize(l, &o, s, strlen(s), FL_NONE);
    assert(ok == FALSE, "Unserialization intentionally unsuccessful");

    assert(i[0].value.integer == 1, "Field #1 set as expected");
    assert_string("2", i[1].value.string, "Field #2 set as expected");
    assert(i[2].value.integer == 3, "Field #3 set as expected");
    assert(i[3].value.integer == 0, "Field #4 set as expected");
    assert(i[4].value.string == NULL, "Field #5 set as expected");

    s = "1!MUVX!1#2#3-4#5/F#";
    schema_list_clear_result(l);
    schema_info_init(&o);

    ok = schema_list_unserialize(l, &o, s, strlen(s), FL_NONE);
    assert(ok == FALSE, "Unserialization intentionally unsuccessful");

    assert(i[0].value.integer == 1, "Field #1 set as expected");
    assert_string("2", i[1].value.string, "Field #2 set as expected");
    assert(i[2].value.integer == 3, "Field #3 set as expected");
    assert(i[3].value.integer == 4, "Field #4 set as expected");
    assert_string("5", i[4].value.string, "Field #5 set as expected");

    schema_list_delete(l);

    char *choices[] = {
        "Yes", "Maybe", "No", "Probably Not"
    };

    SCHEMA_BEGIN(ll, "MUVX", 10);
        SCHEMA_ITEM("i1", TS_INTEGER, 4, 4);
        SCHEMA_ITEM("c2", TS_SELECT, 1, 2);
            SCHEMA_ITEM_SELECT(3, NULL, choices);
        SCHEMA_ITEM("b3", TS_BOOLEAN, 1, 1);
        SCHEMA_ITEM("s4", TS_STRING, 0, 10);
        SCHEMA_ITEM("b5", TS_BOOLEAN, 1, 1);
    SCHEMA_END();

    i = ll->list;
    assert(i != NULL, "Created schema_list_t successfully");

    schema_item_set(i, "1234", 5);
    schema_item_validate(ll, i++);

    schema_item_set(i, "2", 2);
    schema_item_validate(ll, i++);

    schema_item_set(i, "1", 2);
    schema_item_validate(ll, i++);

    schema_item_set(i, "Testing", 8);
    schema_item_validate(ll, i++);

    schema_item_set(i, "0", 2);
    schema_item_validate(ll, i++);

    s = schema_list_serialize(l, FL_NONE);

    assert_string(
        s, "1!MUVX!1234#2#1#Testing#0",
            "Serialized string matches"
    );

    schema_list_clear_result(l);
    schema_info_init(&o);

    strcat(s, "#");

    assert_string(
        s, "1!MUVX!1234#2#1#Testing#0#",
            "Successfully modified string"
    );

    ok = schema_list_unserialize(l, &o, s, strlen(s), FL_NONE);
    assert(ok == TRUE, "Unserialization successful");

    s = schema_list_serialize(l, FL_NONE);

    assert_string(
        s, "1!MUVX!1234#2#1#Testing#0",
            "Reserialized string matches original"
    );

    free(s);
    schema_list_delete(ll);

    puts("[<] test_simple_serialization");
}


/** @name test_date_serialization */

void test_date_serialization() {

    puts("[>] test_date_serialization");

    schema_info_t o;
    schema_info_init(&o);

    SCHEMA_BEGIN(l, "MUVX", 10);
        SCHEMA_ITEM("x", TS_INTEGER, 1, 4);
        SCHEMA_ITEM("y", TS_INTEGER, 1, 4);
            SCHEMA_ITEM_DELIMITER('-');
        SCHEMA_ITEM("m", TS_INTEGER, 1, 4);
            SCHEMA_ITEM_DELIMITER('-');
        SCHEMA_ITEM("d", TS_INTEGER, 1, 4);
        SCHEMA_ITEM("xx", TS_INTEGER, 1, 4);
    SCHEMA_END();

    schema_item_t *i = l->list;
    assert(i != NULL, "Created schema_list_t successfully");

    schema_item_set(i, "0", 2);
    schema_item_validate(l, i++);
    
    schema_item_set(i, "2012", 5);
    schema_item_validate(l, i++);

    schema_item_set(i, "05", 3);
    schema_item_validate(l, i++);

    schema_item_set(i, "24", 3);
    schema_item_validate(l, i++);

    schema_item_set(i, "0", 2);
    schema_item_validate(l, i++);
    
    char *s = schema_list_serialize(l, FL_NONE);

    assert_string(
        s, "1!MUVX!0#2012-5-24#0", "Serialized string matches"
    );

    free(s);
    schema_list_delete(l);
}


/** @name test_selective_serialization */


void test_selective_serialization() {

    puts("[>] test_selective_serialization");

    schema_info_t o;
    schema_info_init(&o);

    char *choices[] = {
        "Yes", "Maybe", "No", "Probably Not"
    };

    SCHEMA_BEGIN(l, "MUVX", 10);
        SCHEMA_ITEM("i1", TS_INTEGER, 4, 4);
            SCHEMA_ITEM_FLAGS(FL_UNIQUE_KEY);
            SCHEMA_ITEM_FLAGS(FL_PRESERVE_VALUE);
        SCHEMA_ITEM("c2", TS_SELECT, 1, 2);
            SCHEMA_ITEM_FLAGS(FL_PRESERVE_VALUE);
            SCHEMA_ITEM_SELECT(3, NULL, choices);
        SCHEMA_ITEM("b3", TS_BOOLEAN, 1, 1);
        SCHEMA_ITEM("s4", TS_STRING, 0, 10);
        SCHEMA_ITEM("b5", TS_BOOLEAN, 1, 1);
            SCHEMA_ITEM_FLAGS(FL_PRESERVE_VALUE);
        SCHEMA_ITEM("s6", TS_STRING, 0, 10);
    SCHEMA_END();

    schema_item_t *i = l->list;
    assert(i != NULL, "Created schema_list_t successfully");

    schema_item_set(i, "9876", 5);
    schema_item_validate(l, i++);

    schema_item_set(i, "4", 2);
    schema_item_validate(l, i++);

    schema_item_set(i, "1", 2);
    schema_item_validate(l, i++);

    schema_item_set(i, "String", 7);
    schema_item_validate(l, i++);

    schema_item_set(i, "0", 2);
    schema_item_validate(l, i++);

    schema_item_set(i, "Another string", 15);
    schema_item_validate(l, i++);

    char *s = schema_list_serialize(l, FL_NONE);

    assert_string(
        s, "1!MUVX!9876#4#1#String#0#Another string",
            "Serialized string matches"
    );

    i = l->list;
    schema_list_clear_result(l);
    assert(i != NULL, "Cleared schema_list_t");

    assert(i[0].value.string == NULL, "Field #1 empty as expected");
    assert(i[1].value.string == NULL, "Field #2 empty as expected");
    assert(i[2].value.string == NULL, "Field #3 empty as expected");
    assert(i[3].value.string == NULL, "Field #4 empty as expected");
    assert(i[4].value.string == NULL, "Field #5 empty as expected");
    assert(i[5].value.string == NULL, "Field #6 empty as expected");

    /* Restore only fields marked with 'preserve' flag */
    u8 ok = schema_list_unserialize(l, &o, s, strlen(s), FL_PRESERVE_VALUE);
    assert(ok == TRUE, "Unserialization successful");

    assert(i[0].value.integer == 9876, "Field #1 correctly preserved");
    assert(i[1].value.integer == 4, "Field #2 correctly preserved");
    assert(i[2].value.string == NULL, "Field #3 empty as expected");
    assert(i[3].value.string == NULL, "Field #4 empty as expected");
    assert(i[4].value.boolean == 0, "Field #5 correctly preserved");
    assert(i[5].value.string == NULL, "Field #6 empty as expected");

    i = l->list;
    schema_list_clear_result(l);
    assert(i != NULL, "Cleared schema_list_t");

    /* Restore unique key fields only */
    ok = schema_list_unserialize(l, &o, s, strlen(s), FL_UNIQUE_KEY);
    assert(ok == TRUE, "Unserialization successful");

    assert(i[0].value.integer == 9876, "Field #1 correctly preserved");
    assert(i[1].value.string == NULL, "Field #2 empty as expected");
    assert(i[2].value.string == NULL, "Field #3 empty as expected");
    assert(i[3].value.string == NULL, "Field #4 empty as expected");
    assert(i[4].value.string == NULL, "Field #5 empty as expected");
    assert(i[5].value.string == NULL, "Field #6 empty as expected");

    free(s);
    schema_list_delete(l);

    puts("[<] test_selective_serialization");
}


/** @name test_eeprom_pool */


void test_eeprom_pool() {

    puts("[>] test_eeprom_pool");

    char buf[MAX_SMS_LENGTH];
    memset(&buf, '\0', MAX_SMS_LENGTH);

    char *test = "1!MUVX!This#is#a#test#3#2#1";
    size_t test_len = strlen(test);

    muvuku_pool_t *p = muvuku_pool_new(
        &muvuku_eeprom_allocator, 8192, 48, NULL
    );

    void *x1 = muvuku_pool_acquire(p);
    void *x2 = muvuku_pool_acquire(p);
    void *x3 = muvuku_pool_acquire(p);
    void *x4 = muvuku_pool_acquire(p);

    muvuku_pool_release(p, x2);
    muvuku_pool_write(p, x1, test, test_len);

    void *x5 = muvuku_pool_acquire(p);
    unsigned int x5_cell = muvuku_pool_cell(p, x5);

    muvuku_pool_write(p, x5, test, test_len);

    muvuku_pool_release(p, x3);
    muvuku_pool_release(p, x4);

    muvuku_pool_read(p, &buf, x1, test_len);
    assert_string(&buf, test, "Copied data matches");

    memset(&buf, '\0', MAX_SMS_LENGTH);

    void *x5_lookup = muvuku_pool_address(p, x5_cell);
    assert(x5_lookup == x5, "Cell-pointer conversion works");
    assert(p->pool->item_count == 2, "Item count is correct");

    muvuku_pool_release(p, x5);
    muvuku_pool_release(p, x5);

    assert(p->pool->item_count == 1, "Double-free is harmless");

    assert(
        (muvuku_pool_cellinfo(p, 1) & 1) == 1,
            "Introspection detects used cell"
    );
    assert(
        (muvuku_pool_cellinfo(p, 4) & 1) == 0,
            "Introspection detects unused cell"
    );

    puts("[<] test_eeprom_pool");
}


/** @name test_stringlist_pool */


typedef struct verify_state {
    
    int index;
    int limit;
    char **strings;

} verify_state_t;


int verify_string(muvuku_stringlist_t *l,
                  char *str, size_t len, void *verify_state) {

    size_t max_length = 255;
    char buffer[max_length];

    verify_state_t *vs = (verify_state_t *) verify_state;

    assert(len < max_length, "String length is okay");
    muvuku_pool_read(l->pool, &buffer, str, len);
    buffer[len] = '\0';

    assert_string(
        vs->strings[vs->index], &buffer, "Strings match"
    );

    return (++(vs->index) >= vs->limit ? FALSE : TRUE);
}


void test_stringlist_pool(muvuku_allocator_t *a, void *options) {

    puts("[>] test_stringlist_pool");

    muvuku_pool_t *p = muvuku_pool_new(
        a, sizeof(reserved), 16, options
    );

    muvuku_stringlist_t *l1 =
        muvuku_stringlist_init(p, muvuku_pool_acquire(p));

    muvuku_stringlist_t *l2 =
        muvuku_stringlist_init(p, muvuku_pool_acquire(p));

    int i;

    char *test[5] = {
        "123456789", "abcdefg", "ABCDEFG", "987654321", "xxxxxxxxx"
    };

    size_t test_len[5] = {
        strlen(test[0]), strlen(test[1]), strlen(test[2]),
        strlen(test[3]), strlen(test[4])
    };

    for (i = 0; i < 4; ++i) {
        assert(
            muvuku_stringlist_add(l1, test[i], test_len[i]),
                "String addition successful"
        );
    }

    for (i = 0; i < 100; ++i) {
        /* Fill up the remaining space */
        muvuku_stringlist_add(l1, test[4], test_len[4]);
    }

    assert(
        !muvuku_stringlist_add(l1, test[0], test_len[0]),
            "String addition fails as expected"
    );

    verify_state_t verify_state = { 0, 5, test };
    muvuku_stringlist_each(l1, &verify_string, &verify_state);

    puts("[<] test_stringlist_pool");
}


/** @name test_settings_storage */

void test_settings_storage_map() {

    puts("[>] test_settings_storage_map");
    memset(&reserved, '\0', sizeof(reserved));

    SCHEMA_BEGIN(l1, "MUV1", 10);
        SCHEMA_ITEM("i", TS_INTEGER, 4, 4);
    SCHEMA_END();

    SCHEMA_BEGIN(l2, "MUV2", 10);
        SCHEMA_ITEM("i", TS_INTEGER, 4, 4);
    SCHEMA_END();

    SCHEMA_BEGIN(l3, "MUV3", 10);
        SCHEMA_ITEM("i", TS_INTEGER, 4, 4);
    SCHEMA_END();

    SCHEMA_BEGIN(l4, "MUV4", 10);
        SCHEMA_ITEM("i", TS_INTEGER, 4, 4);
    SCHEMA_END();

    SCHEMA_BEGIN(l5, "MUV5", 10);
        SCHEMA_ITEM("i", TS_INTEGER, 4, 4);
    SCHEMA_END();

    muvuku_settings_t s;
    memset(&s, '\0', sizeof(s));

    muvuku_pool_t *p = muvuku_pool_new(
        &muvuku_flash_allocator, sizeof(reserved), 10, &reserved
    );

    muvuku_cell_t c1 = muvuku_storage_retrieve(&s, p, l1);
    muvuku_cell_t c2 = muvuku_storage_retrieve(&s, p, l2);
    muvuku_cell_t c3 = muvuku_storage_retrieve(&s, p, l3);

    muvuku_cell_t cc1 = muvuku_storage_retrieve(&s, p, l1);
    muvuku_cell_t cc2 = muvuku_storage_retrieve(&s, p, l2);
    muvuku_cell_t cc3 = muvuku_storage_retrieve(&s, p, l3);

    assert(c1 != INVALID_CELL, "Valid cell returned from write path");
    assert(c2 != INVALID_CELL, "Valid cell returned from write path");
    assert(c3 != INVALID_CELL, "Valid cell returned from write path");

    assert(c1 == cc1, "Proper cell returned from read path");
    assert(c2 == cc2, "Proper cell returned from read path");
    assert(c3 == cc3, "Proper cell returned from read path");

    assert(muvuku_pool_address(p, c1) != NULL, "Valid cell address");
    assert(muvuku_pool_address(p, c2) != NULL, "Valid cell address");
    assert(muvuku_pool_address(p, c3) != NULL, "Valid cell address");
}


extern void _prototype_progmem_write(void *dst, void *src);


/** @name test_prototype_progmem_write */

void test_prototype_progmem_write() {

    puts("[>] test_progmem_write");
    memset(&reserved, '\0', sizeof(reserved));

    unsigned int i = 0;
    unsigned char buf[MUVUKU_PAGE_SIZE] = { 0 };
    unsigned char *p = muvuku_align_page(&reserved, unsigned char, TRUE);

    for (i = 0; i < MUVUKU_PAGE_SIZE; ++i) {
        buf[i] = (i % 256);
    }
    
    _prototype_progmem_write(p, buf);
    
    for (i = 0; i < MUVUKU_PAGE_SIZE; ++i) {
        assert(p[i] == (i % 256), "Value matches expected");
    }

    puts("[<] test_progmem_write");
}


/** @name test_prototype_flash_zero */

void test_prototype_flash_zero() {

    puts("[>] test_prototype_flash_zero");
    memset(&reserved, '\0', sizeof(reserved));

    unsigned int i = 0;
    unsigned int len = (MUVUKU_PAGE_SIZE * 8);
    unsigned char *buf = muvuku_align_page(&reserved, unsigned char, TRUE);

    memset(buf, 0, len);

    for (i = 0; i < len; ++i) {
        buf[i] = (i % 256);
    }

    /* Zero between 10 and 501, inclusive */
    int l1 = 10, s1 = (MUVUKU_PAGE_SIZE * 2) - 20;
    muvuku_flash_zero(&buf[l1], s1);
 
    /* Zero between 1025 and 1535, inclusive */
    int l2 = (MUVUKU_PAGE_SIZE * 4) + 1, s2 = (MUVUKU_PAGE_SIZE * 2) - 1;
    muvuku_flash_zero(&buf[l2], s2);

    for (i = 0; i < len; ++i) {
        if ((i >= l1 && i < l1 + s1) || (i >= l2 && i < l2 + s2)) {
            assert(buf[i] == 0, "Value is zero");
        } else {
            assert(buf[i] == (i % 256), "Value matches expected");
        }
    }

    puts("[<] test_prototype_flash_zero");
}



/** @name test_flash_pool */

void test_flash_pool() {

    puts("[>] test_flash_pool");
    memset(&reserved, '\0', sizeof(reserved));

    unsigned int i = 0;
    unsigned char buf[MUVUKU_PAGE_SIZE] = { 0 };
    unsigned char *p = muvuku_align_page(&reserved, unsigned char, TRUE);

    muvuku_pool_t *pool = muvuku_pool_new(
        &muvuku_flash_allocator, sizeof(reserved) - MUVUKU_PAGE_SIZE, 16, p
    );

    puts("[<] test_flash_pool");
}


/** @name test_align_page*/

void test_align_page() {

    puts("[>] test_align_page");

    void *p1 = NULL;
    void *p2 = muvuku_align_page(p1, void, TRUE);
    assert(p1 == p2, "Aligned pointer left unchanged");

    puts("[<] test_align_page");
}

/** @name main */

int main(int argc, char *argv[]) {

    muvuku_subsystem_init_pool();

    test_align_page();
    test_prototype_progmem_write();
    test_prototype_flash_zero();

    test_simple_serialization();
    test_selective_serialization();
    test_date_serialization();

    test_eeprom_pool();
    test_stringlist_pool(&muvuku_eeprom_allocator, NULL);

    test_flash_pool();
    test_stringlist_pool(&muvuku_flash_allocator, &reserved);

    test_settings_storage_map();

    return 0;

};

