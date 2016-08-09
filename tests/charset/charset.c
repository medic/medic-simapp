
#include <bladox.h>
#include <string.h>
#include <stdlib.h>
#include <turbo/turbo.h>


/* Test case:
    Testing UCS-2 character sets via STK display_text. */

lc_char PROGMEM lc_test_str[] = {

    LC_UN("\x84\x08\x34\x80"
          "\x9\x38\x9\x3e\x9\x2e\x9\x41\x9\x26\x9\x3e"
          "\x9\x2f\x9\x3f\x9\x15\x0\x20\x9\x38\x9\x4d"
          "\x9\x35\x9\x3e\x9\x38\x9\x4d\x9\x25\x9\x4d"
          "\x9\x2f\x0\x20\x9\x38\x9\x30\x9\x4d\x9\x35\x9\x47")
    LC_END
};


const u8 PROGMEM lc_test_item[] =
    "\x84\x08\x34\x80"
    "\x9\x38\x9\x3e\x9\x2e\x9\x41\x9\x26\x9\x3e"
    "\x9\x2f\x9\x3f\x9\x15\x0\x20\x9\x38\x9\x4d"
    "\x9\x35\x9\x3e\x9\x38\x9\x4d\x9\x25\x9\x4d"
    "\x9\x2f\x0\x20\x9\x38\x9\x30\x9\x4d\x9\x35\x9\x47";


const u8 *PROGMEM l_test[] = {
    lc_test_item
};

const lc_list PROGMEM ll_test[] = {
    LC_UN_LIST((const u8 *) l_test)
    LC_END_LIST
};



lc_char PROGMEM lc_topmenu_name[] = {
    LC_UN("SIM Services")
    LC_END
};


lc_char PROGMEM lc_menu_top[] = {
    LC_UN("Charset Test")
    LC_END
};

lc_char PROGMEM lc_select_str[] = {
    LC_UN("Test Select")
    LC_END
};


u8 menu_top_ctx (SCtx *ctx, u8 action)
{
    return APP_OK;
}

SNodeP menu_top_n = { lc_test_str, menu_top_ctx };


u8 menu_test_ctx (SCtx *ctx, u8 action)
{
    if (action == APP_ENTER) {
        display_text(locale(lc_test_str), NULL);
    }

    return APP_OK;
}

SNodeP menu_test_n = {
    lc_test_str, menu_test_ctx
};


u8 menu_select_ctx (SCtx *ctx, u8 action)
{
    if (action == APP_ENTER) {
        select_item(
            1, (const u8 **) locale_list(ll_test),
                locale(lc_test_str), NULL, 1, Q_SELECT_ITEM_CHOICE
        );
    }

    return APP_OK;
}

SNodeP menu_select_n = {
    lc_select_str, menu_select_ctx
};


SEdgeP menu_edges_p[] = {

    { &menu_top_n, &menu_test_n },
    { &menu_top_n, &menu_select_n },
    { NULL, NULL }
};


void action_menu(void *data)
{
    SCtx *c = spider_init();

    c->n = (SNode *) &menu_top_n;
    c->eP = (SEdgeP *) &menu_edges_p;

    spider(c);
}


void turbo_handler(u8 action, void *data)
{
    switch (action)
    {
        case ACTION_INSERT_MENU:
            set_proc_8(PROC_8_LANGUAGE, LC_UNSPECIFIED);
            set_menu_alpha(locale(lc_topmenu_name));
            insert_menu(locale(lc_menu_top));
            break;

        case ACTION_MENU_SELECTION:
            stk_thread(action_menu, data);
            break;

        default:
            break;
    }
}

