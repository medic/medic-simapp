/**
 * Parser/converter for legacy Muvuku forms
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

var fs = require('fs'),
    fspath = require('path'),
    jsdump = require('jsdump');


/** Parse rules: **/

var raw_string_regex =
    /(?:(?:u8|PROGMEM)\s+)+(\w+)(?:\[\])?\s*=\s*"([^"]*)"\s*;/g;

var str_begin_regex =
    /(?:(?:u8\s*\*|lc_char|PROGMEM)\s+)+(\w+)(?:\[\])?\s*=\s*{([^}]*)}\s*;/g;

var str_regex =
    /LC_([A-Z]{2})\s*\(\s*"([^"]*)"\)/g;

var str_end_regex =
    /(?:^|\s+)LC_END(?:\s+|$)/;

var list_regex =
    /LC_([A-Z]{2})_LIST\s*\((?:\([^\)]*\))?\s*(\w+)\s*\)/g;

var list_end_regex =
    /(?:^|\s+)LC_END_LIST(?:\s+|$)/;

var form_regex =
    /SCHEMA_BEGIN((?:.|[\r\n])+?)SCHEMA_END\(\s*\)/g;

var form_args_regex = 
    /^\s*\(\s*(\w+)\s*,\s*(\w+)\s*,\s*(\d+)\s*\)\s*;/;

var form_split_regex = 
    /SCHEMA_ITEM(?=\()/;

var form_item_label_regex = 
    /^\s*\(\s*(?:locale\(\s*(\w+)\s*\))|(\w+)/

var form_item_choices_regex = 
    /SCHEMA_ITEM_SELECT\((?:.|[\r\n])+?locale_list\(\s*(\w+)\s*\)/

var form_item_args_regex = 
    /,\s*(\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)/;

var form_item_validate_regex = 
    /SCHEMA_ITEM_VALIDATE\((\w+)\)/g;

var form_item_flags_regex = 
    /SCHEMA_ITEM_FLAGS\((\w+)\)/g;


/** @namespace exports */


/**
 * read_file:
 */

exports.read_file = function (_path, _callback) {
    fs.readFile(_path, 'utf-8', function (_err, _text) {
        if (_err) {
            throw _err;
        }
        if (_callback) {
            _callback(_err, _text);
        }
    });
};


/**
 * trim:
 */

var trim = function (_string) {

    return _string.replace(/^\s+/, '').replace(/\s+$/, '');
};


/**
 * fixup_string:
 */

var fixup_string = function (_string) {

    var rv = _string;

    var table = [
        [ /\\4/g, '\u00e8' ],
        [ /\\5/g, '\u00e9' ],
        [ /\\11/g, '\u00e7' ],
        [ /\\177/g, '\u00e0' ]
    ];

    if (rv === null || rv === undefined) {
        return rv;
    }

    for (var i = 0, len = table.length; i < len; ++i) {
        rv = rv.replace(table[i][0], table[i][1]);
    }

    return rv;
};


/**
 * localize:
 */
var localize = function (_locale, _value) {

    var rv = {};

    rv[_locale.toLowerCase()] = _value;
    return rv;
}


/**
 * build_symtab:
 *  Pull all of the "interesting" strings out of a legacy
 *  Muvuku form (i.e. the C-based muvuku-data file).
 */

exports.build_symtab = function (_text) {

    var symtab = {};
    var block, match, m;

    while ((block = raw_string_regex.exec(_text)) !== null) {

        /* Ordinary string */
        symtab[trim(block[1])] = {
            type: 'string', value: fixup_string(block[2])
        };
    }

    while ((block = str_begin_regex.exec(_text)) !== null) {

        var symbol = { values: {} };
        var block_name = trim(block[1]);

        if (str_end_regex.exec(block[2]) !== null) {

            /* Fully-localized string */
            while ((match = str_regex.exec(block[2])) !== null) {
                symbol.values[trim(match[1])] = trim(match[2]);
            }

            symbol.type = 'lc_string';
            symtab[block_name] = symbol;

        } else if (list_end_regex.exec(block[2]) !== null) {

            /* Fully-localized string list */
            while ((m = list_regex.exec(block[2])) !== null) {
                symbol.values[m[1]] = (symtab[m[2]] || {}).values;
            }

            symbol.type = 'lc_list';
            symtab[block_name] = symbol;

        } else {

            /* Ordinary string list */
            symbol.values = trim(block[2]).split(/\s*,\s*/).map(
                function (_symbol) {
                    return (symtab[_symbol] || {}).value;
                }
            );

            symbol.type = 'list';
            symtab[block_name] = symbol;
        }
    }

    return symtab;
};

/**
 * build_forms:
 */

exports.build_forms = function (_symtab, _text, _locale) {

    var retval = [];
    var block, match;

    var locale = (_locale || '').toUpperCase();

    var type_map = {
        TS_PHONE: 'phone',
        TS_STRING: 'string', 
        TS_SELECT: 'integer', 
        TS_INTEGER: 'integer',
        TS_BOOLEAN: 'boolean'
    };

    /* For every form... */
    while ((block = form_regex.exec(_text)) !== null) {

        var rv = { meta: {}, fields: {} };
        var form_args = form_args_regex.exec(block[1]);
        var items = block[1].split(form_split_regex).slice(1);

        var fields_seen = 0;
        var fields_expected = parseInt(form_args[1], 10);

        if (form_args.length - 1 < 3) {
            continue;
        }

        rv.meta.code = (_symtab[form_args[2]] || {}).value;

        if (rv.meta.code === 'MUVU') {
            continue;
        }

        /* For every field... */
        for (var i = 0, len = items.length; i < len; ++i) {

            var item = items[i];

            /* Phase one:
                Figure out the name/args for the current field. */

            var label_symbol = form_item_label_regex.exec(item);
            label_symbol = (label_symbol[1] || label_symbol[2]);

            if (!label_symbol) {
                continue;
            }

            var label = _symtab[label_symbol];
            var item_args = form_item_args_regex.exec(item);
            var ext_label_symbol = label_symbol.replace(/^lc_/, '')

            if (item_args.length - 1 < 3) {
                continue;
            }

            /* Phase two:
                Is there a reference list of options? If so, capture
                the symbol, then look up the actual list with the symbol. */

            var choices_symbol = form_item_choices_regex.exec(item);

            var choices = (
                choices_symbol ?
                    (_symtab[choices_symbol[1]] || {}).values[locale] : null
            );

            var type = item_args[1];
            var min_chars = parseInt(item_args[2], 10);
            var max_chars = parseInt(item_args[3], 10);

            /* Phase three:
                Capture validation and flags directives. */

            var v, validate = {};

            while ((v = form_item_validate_regex.exec(item)) !== null) {
                validate[v[1].toLowerCase()] = true;
            }

            var f, flags = {};

            while ((f = form_item_flags_regex.exec(item)) !== null) {
                flags[f[1].toLowerCase().replace(/^fl_/, '')] = true;
            }


            /* Phase four:
                We have the field data; emit it as JSON. */

            rv.fields[ext_label_symbol] = {
                labels: {
                    long: null,
                    description: null,
                    short: localize(
                        locale, fixup_string(label.values[locale])
                    )
                },
                position: i,
                type: type_map[type],
                length: [ min_chars, max_chars ],
                validate: validate,
                flags: flags
            };

            if (choices) {
                
                rv.fields[ext_label_symbol].choices = choices.map(function (c, i) {
                    return [ i + 1, localize(locale, c) ];
                });
            }

        }

        retval.push(rv);
    }

    return retval;
};


/**
 * fatal:
 */
exports.fatal = function (_error) {

    process.stderr.write('fatal: ' + _err.message + '\n');
    process.exit(1);
};


/**
 * main:
 */
exports.main = function (_paths, _locale) {

    var rv = {};

    /* Process multiple files:
        All output will occur on standard out. */

    _paths.forEach(function (_path, _i) {
        exports.read_file(_path, function (_error, _text) {
            var forms = exports.build_forms(
                exports.build_symtab(_text), _text, _locale
            );
            process.stdout.write(jsdump.parse(forms));
        });
    });

    return this;
};


/** @namespace :: */
exports.main.call(
    this, process.argv.slice(3), process.argv[2]
);

