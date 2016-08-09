/**
 * JSON schema compiler for Muvuku
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
    jsdump = require('jsDump'),
    handlebars = require('handlebars');


/**
 * Default language:
 *  This setting controls the locale/language used for strings
 *  that do not contain any language or locale information. To
 *  explicitly specify translations for a string, use an object
 *  like `{ 'en': '', 'fr': '' }` instead of a plain string.
 */
var default_language = 'en';


/**
 * `CompilationError`:
 *    A simple exception class. An error stack is maintained
 *    via the `_parent` constructor option, which allows error
 *    backtraces to be easily generated via the `format` method.
 */
var CompilationError = function (_parent, _attrs) {

    this._parent = _parent;
    this._attrs = _attrs;

    return this;
};

CompilationError.prototype = {

    format: function () {

        return jsdump.parse(this) + '\n';
    }
};


/**
 * Make `_array` contiguous by removing undefined and/or null
 * member items. Returns an array no larger than the original.
 */
var compact = function (_array) {

    var rv = [];

    for (var i = 0, len = _array.length; i < len; ++i) {
        if (_array[i] !== null && _array[i] !== undefined) {
            rv.push(_array[i]);
        }
    }

    return rv;
};


/**
 */
var fatal = exports.fatal = function (_message, _err) {

    process.stderr.write(
        'error: ' + _message + '\n'
    );

    if (_err) {
        process.stderr.write(_err.format());
    }

    process.stderr.write(
        'fatal: Encountered an unrecoverable error; exiting\n'
    );

    return process.exit(127);
};


/**
 */
var normalize_toplevel = function (_forms) {

    var rv = _forms;

    if (!(rv instanceof Array)) {

        rv = [ rv ];

    } else if (typeof(rv) !== 'object') {

        throw new CompilationError(null, {
            path: _path,
            message: 'Top-level must be object or array of objects'
        });
    }

    return rv;
};


/**
 */
var normalize_language_code = function (_code) {

    switch (_code) {
        case 'ne':
            return 'un';
        default:
            break;
    }

    return _code;
};


/**
 */
var normalize_multilanguage_object = function (_object) {

    var rv = {};

    if (typeof(_object) === 'object') {

        for (var k in _object) {
            rv[normalize_language_code(k)] = _object[k];
        }

    } else {

        rv[default_language] = _object;
    }

    return rv;
};


/**
 */
var normalize_field_type = function (_type) {

    if (typeof(_type) !== 'string') {
        throw new CompilationError(null, {
            type: _type,
            message: 'Non-string field type specified'
        });
    }

    switch (_type) {
        case 'string': case 'phone': case 'date': case 'month':
        case 'integer': case 'numeric': case 'boolean':
            return _type;
    }

    throw new CompilationError(null, {
        type: _type,
        message: 'Unrecognized field type'
    });
};


/**
 */
var normalize_field_position = function (_position) {

    if (typeof(_position) !== 'number') {

        throw new CompilationError(null, {
            message: 'Position attribute required; must be numeric'
        });
    }

    /* Coerce to integer */
    return Math.round(_position);
};


/**
 */
var normalize_field_length = function (_length) {

    var length = _length;

    /* Apply default */
    if (length === null || length === undefined) {
        length = [ 1, 4 ];
    }

    /* Validate */
    var is_valid_length = (
        (typeof(length) == 'number') ||
            ((length instanceof Array) && length.length == 2)
    );

    if (!is_valid_length) {
        throw new CompilationError(null, {
            message: 'Length must be an integer or a two-item array'
        });
    }

    /* Coerce to array */
    if (typeof(length) == 'number') {
        length = [ length, length ];
    }

    /* Return object */
    return {
        lower: length[0], upper: length[1]
    };
};


/**
 */
var normalize_list_options = function (_options) {

    var rv = [];

    if (!(_options instanceof Array)) {
        throw new CompilationError(null, {
            message: 'Non-array selection list encountered'
        });
    }

    /* For every option:
        Transform and validate, throwing an error if unrecoverable */

    for (var i = 0, len = _options.length; i < len; ++i) {

        var c = _options[i];

        /* Three available formats:
            Array of length two (i.e. numeric key, then text caption),
            ordinary unlocalized string, or a multilanguage object.
            The latter maps language codes (like `en`) to strings. */

        if (c instanceof Array) {

            if (c.length !== 2) {
                throw new CompilationError(null, {
                    index: i,
                    message: 'List option must be two-member array'
                });
            }

            rv.push([
                parseInt(c[0], 10), encode(
                    normalize_multilanguage_object(c[1])
                )
            ]);

        } else if (typeof(c) === 'string' || typeof(c) === 'object') {

            rv.push([
                i, encode(normalize_multilanguage_object(c))
            ]);

        } else {

            throw new CompilationError(null, {
                index: i,
                message: 'List item has invalid type'
            });
        }
    }

    return rv;
};


/**
 */
var normalize_condition_options = function (_conditions) {

    var rv = {};

    if (_conditions === null || _conditions === undefined) {

        return rv;

    } else if (typeof(_conditions) == 'object') {

        for (var k in _conditions) {

            if (typeof(k) !== 'string') {
                throw new CompilationError(null, {
                    message: 'Condition left-hand-side must be field name'
                });
            }

            if (typeof(_conditions[k]) !== 'number') {
                throw new CompilationError(null, {
                    message: 'Condition right-hand-side must be an integer'
                });
            }

            rv[k] = parseInt(_conditions[k], 10);
        }
    }

    return rv;
};


/**
 */
var normalize_condition_operator = function (_conditions_operator) {

    return (_conditions_operator === '||' ? true : false);
};


/**
 */
var normalize_validation_options = function (_validations) {

    var rv = {};

    if (_validations === null || _validations === undefined) {

        return rv;

    } else if (_validations instanceof Array) {

        _validations.forEach(function (k) {
            rv[k] = true;
        });

    } else if (typeof(_validations) === 'object') {

        for (var k in _validations) {
            rv[k] = true;
        }

    } else {

        throw new CompilationError(null, {
            message: 'List of validation rules is invalid'
        });
    }

    return rv;
};


/**
 */
var normalize_flags = function (_flags) {

    var rv = {};

    if (_flags === null || _flags === undefined) {

        return rv;

    } else if (typeof(_flags) === 'object') {

        for (var k in _flags) {
            rv[k] = true;
        }

    } else {

        throw new CompilationError(null, {
            message: 'List of field flags is invalid'
        });
    }

    return rv;
};


/**
 */
var require_type = function (_variable, _type, _attr_name) {

    if (!_variable || typeof(_variable) !== _type) {
        throw new CompilationError(null, {
            name: _variable,
            attribute: _attr_name, type: _type,
            message: 'Missing and/or invalid attribute'
        });
    }
};


/**
 */
var validate_field_name = function (_field_name) {

    if (typeof(_field_name) !== 'string') {
        throw new CompilationError(null, {
            field: _field_name,
            message: 'Field name must be a string'
        });
    }

    if (!_field_name.match(/^\w[\d\w]+/)) {
        throw new CompilationError(null, {
            field: _field_name,
            message: 'Field name violates naming rules'
        });
    }
};


/**
 */
var validate_form_code = function (_form_code) {

    if (typeof(_form_code) !== 'string') {
        throw new CompilationError(null, {
            code: _form_code,
            message: 'Form code must be a string'
        });
    }

    if (!_form_code.match(/^\w{1,9}$/)) {
        throw new CompilationError(null, {
            code: _form_code,
            message: 'Form code must be 1 to 9 characters'
        });
    }
};


/**
 */
var encode = function (_string_or_object) {

    if (typeof(_string_or_object) === 'object') {

        var rv = {};

        for (var k in _string_or_object) {
            rv[k] = encode_string(_string_or_object[k]);
        }

        return rv;

    } else {

        return encode_string(_string_or_object);
    }
};


/**
 */
var encode_string = exports.encode_string = function (_string) {

    return (
        is_ascii(_string) ?
            ascii_to_gsm(_string) : ucs2_to_slv(_string)
    );
};


/**
 * @name ucs2_to_slv:
 *  Convert a javascript string (represented internally as UCS-2)
 *  in to a GSM SLV-encoded string. An SLV string consists of a
 *  two byte escape/magic value, followed by a one byte string size
 *  (read: a byte count, not a character count), followed by one byte
 *  of padding, followed by a series of `size / 2` UCS-2 codepoints
 *  stored as concatenated 16-bit big-endian integer values.
 */
var ucs2_to_slv = exports.ucs2_to_slv = function (_string) {

    var len = _string.length;

    if (len > 0xff) {
        return false; /* Too long for SLV encoding */
    }

    var rv = '\\x84\\x08';                          /* Escape */
    rv += ('\\x' + (2 * (len + 1)).toString(16));   /* Length */
    rv += '\\x80';                                  /* Padding */

    for (var i = 0; i < len; ++i) {

        /* High byte */
        rv += ('\\x' + ((_string.charCodeAt(i) & 0xff00) >> 8).toString(16));
        /* Low byte */
        rv += ('\\x' + (_string.charCodeAt(i) & 0xff).toString(16));
    }

    return rv + '\\x0\\x0';
};


/**
 * @name is_ascii:
 *   Returns true if the provided string is limited to simple eight-bit
 *   ASCII/GSM compatible characters; false is UCS-2 in SLV is required.
 */
var is_ascii = exports.is_ascii = function (_string) {

    for (var i = 0, len = _string.length; i < len; ++i) {

        if (_string.charCodeAt(i) > 255) {
            return false;
        }
    }

    return true;
};


/**
 */
var ascii_to_gsm = exports.ascii_to_gsm = function (_string) {

    var rv = _string;

    var table = {
        '\u00e9': '\\5',
        '\u00c9': '\\37',
        '\u00e8': '\\4',
        '\u00e7': '\\11',
        '\u00e0': '\\177',
        '\u00e1': '\\177',
        '\u0027': '\\47',
        '\u00f2': '\\10',
        '\u00f3': '\\10',
        '\u00ed': '\\07'
    };

    if (rv === null || rv === undefined) {
        return rv;
    }

    for (var src in table) {
        rv = rv.replace(new RegExp(src, 'g'), table[src]);
    }

    return rv;
};


/**
 */
var compile_forms = exports.compile_forms = function (_forms, _path) {

    return _forms.map(function (_form, _i) {
        return compile_form(_form, _path, _i);
    });
};


/**
 */
var compile_form = exports.compile_form = function (_form, _path, _i) {

    var rv = {};

    var meta = _form.meta;
    var fields = _form.fields;

    require_type(fields, 'object', 'meta');
    require_type(fields, 'object', 'fields');

    validate_form_code(meta.code);

    rv.meta = {};
    rv.meta.code = meta.code.toLowerCase();
    rv.meta.length = { lower: 7, upper: 7 }; /* `1!PSMS!`... */

    rv.meta.label = encode(
        normalize_multilanguage_object(meta.label)
    );

    try {

        rv.flags = {};
        rv.lists = {};
        rv.fields = [];
        rv.strings = {};
        rv.fieldmap = {};
        rv.conditions = {};

        for (var field_name in fields) {

            try {

                /* Compile a single field */
                var length = compile_field(rv, fields[field_name], {
                    field: field_name, code: rv.meta.code
                });

                /* Add length to per-form count */
                for (var k in length) {
                    rv.meta.length[k] += length[k];
                }

                /* Make array contiguous */
                rv.fields = compact(rv.fields);

                /* Fix position */
                for (var i = 0, len = rv.fields.length; i < len; ++i) {
                    rv.fields[i].position = i;
                }

                /* High-level field count */
                rv.meta.count = rv.fields.length;

                /* Low-level field count */
                rv.meta.count_expanded = rv.meta.count;

                for (var i = 0, len = rv.fields.length; i < len; ++i) {
                    if (rv.fields[i].type == 'date') {
                        rv.meta.count_expanded += 2;
                    }
                }

            } catch (e) {

                throw new CompilationError(e, {
                    message: 'Error while compiling field',
                    field: field_name
                });
            }
        }

        /* Field name to offset transform */
        rv = transform_global_conditions(rv);

    } catch (e) {

        throw new CompilationError(e, {
            message: 'Error while compiling fields',
            offset: _i, path: _path, code: rv.meta.code
        });
    }

    return rv;
};


/**
 */
var compile_field = function (_rv, _field, _names) {

    var map = {};

    /* Initialization:
        Validate some of the field's key attributes before continuing. */

    validate_field_name(_names.field);
    require_type(_field, 'object', _names.field);

    require_type(
        _field.labels, 'object', [ _names.field, 'labels' ].join('.')
    );

    var type = normalize_field_type(_field.type);
    var position = normalize_field_position(_field.position);

    /* Phase one:
        Build a symbol table, mapping fields to localized strings. */

    var string_name = [ 'lc', _names.code, _names.field ].join('_');

    var label = (_field.labels.short || _names.field);
    var labels = normalize_multilanguage_object(label);

    /* Helper:
        Create a string named `_name` for language `_lang`.
        Set its value to whatever is specified in `_value`. */

    var add_string = function (_name, _lang, _value) {

        var string_name_localized = [ _name, _lang ].join('_');

        if (!_rv.strings[_name]) {
            _rv.strings[_name] = { localized: true, list: [] };
        }

        _rv.strings[_name].list.push({
            name: string_name_localized,
            language: _lang.toLowerCase(),
            value: encode(_value)
        });
    };

    /* Create strings:
        Create primary label for each language, plus some
        type-specific labels if the data type requires them. */

    for (var lang in labels) {

        var v = labels[lang];
        add_string(string_name, lang, v);

        /* FIXME:
            We need to localize the year/month/day label prefix. */

        if (_field.type === 'date') {
            add_string(string_name + '_day', lang, 'Day: ' + v);
            add_string(string_name + '_month', lang, 'Month: ' + v);
            add_string(string_name + '_year', lang, 'Year: ' + v);
        }
    }

    /* Phase two:
        For fields that have selection lists associated, process
        the list of options, add each option to the return value. */

    if (_field.list) {
        type = 'select';
        _rv = compile_list_options(_rv, _field, _names);
    }

    /* Phase three:
        For fields that have skip rules/conditions associated, process
        the list of conditions, and add each option to the return value. */

    var conditions = {};

    var conditions_operator_is_or = normalize_condition_operator(
        _field['conditions-operator']
    );

    if (_field.conditions) {
        _rv = compile_condition_options(_rv, conditions, _field, _names);
    }

    /* Phase four:
        For fields with named validation rules, process them here. */

    var validations = {};

    if (_field.validations) {
        validations = normalize_validation_options(_field.validations);
    }

    /* Phase five:
        Build a list of fields, normalizing values along the way. */

    var flags = normalize_flags(_field.flags);
    var length = normalize_field_length(_field.length);

    var list = _rv.lists[_names.field];
    var comment = labels[default_language].replace(/(\\r|\\n)+/, ': ');

    _rv.fieldmap[_names.field] = _rv.fields[position] = {
        flags: flags,
        validations: validations,
        conditions: conditions,
        is_date_type: (type === 'date'),
        is_month_type: (type === 'month'),
        is_integer_type: (type === 'integer'),
        name: _names.field, comment: comment,
        list: list, list_ref: (list || {}).name,
        ref: string_name, type: type, length: length,
        conditions_operator_is_or: conditions_operator_is_or
    };

    return length;
};


/**
 */
var compile_list_options = function (_rv, _field, _names) {

    var rv = { name: null, map: {} };
    var options = normalize_list_options(_field.list);

    /* For each possible option... */
    for (var i = 0, len = options.length; i < len; ++i) {

        var option = options[i];
        var option_languages = option[1];

        /* For every language... */
        for (var lang in option_languages) {

            var select_name = [
                'll', _names.code, _names.field, lang
            ].join('_');

            var option_name = [
                'lc', _names.code, _names.field, lang, i + 1
            ].join('_');

            /* Add string to string table */
            _rv.strings[option_name] = {
                localized: false, list: []
            };

            _rv.strings[option_name].list.push({
                name: option_name,
                language: lang.toLowerCase(),
                value: encode(option_languages[lang])
            });

            /* Add option to selection-list table */
            if (!rv.map[lang]) {
                rv.map[lang] = {
                    list: [],
                    name: select_name,
                    language: lang.toLowerCase()
                };
            }

            rv.map[lang].list.push({
                name: option_name, ref: option_name,
                value: encode(option[1])
            });
        }
    }

    /* Generate name/size of list */
    rv.size = options.length;
    rv.name = [ 'l', _names.code, _names.field ].join('_');

    /* Save selection list */
    _rv.lists[_names.field] = rv;

    return _rv;
};


/**
 */
var compile_condition_options = function (_rv, _conditions, _field, _names) {

    var rv = { name: null, equal: {} };

    rv.name = [ 'cf', _names.code, _names.field ].join('_');
    rv.equal = normalize_condition_options(_field.conditions);

    rv.operator_is_logical_or = (
        _field['conditions-operator'] == '||' ? true : false
    );

    _conditions[rv.name] = true;
    _rv.conditions[rv.name] = rv;

    return _rv;
};


/**
 * Field-name to field-position transformation for global conditions
 * list. Look up each field name referenced from a condition, and
 * replace the field name with a field offset that's usable by Muvuku.
 */

var transform_global_conditions = function (_rv) {

    for (var k in _rv.conditions) {

        var equal = {};
        var condition = _rv.conditions[k];

        for (var lhs in condition.equal) {
            var field = _rv.fieldmap[lhs];

            if (!field) {
                throw new CompilationError(null, {
                    message: 'Condition refers to non-existent field',
                    field: lhs
                });
            }

            /* Replace field name with field offset */
            equal[field.position] = condition.equal[lhs];
        }

        condition.equal = equal;
    }

    return _rv;
};


/**
 */
var process_json = function (_paths, _callback) {

    var rv = [];
    var paths_processed = 0;
    var paths_total = _paths.length;

    _paths.forEach(function (_path, _i) {
        fs.readFile(_path, 'utf-8', function (_err, _text) {

            if (_err) {
                return _callback(new CompilationError(_err, {
                    path: _path,
                    message: 'Unable to read JSON file'
                }));
            }

            try {
                forms = JSON.parse(_text);
            } catch (e) {
                return _callback(new CompilationError(e, {
                    path: _path,
                    message: 'Failure while parsing JSON'
                }));
            }

            try {
                rv = rv.concat(
                    compile_forms(
                        normalize_toplevel(forms), _path
                    )
                );
            } catch (e) {
                return _callback(new CompilationError(e, {
                    path: _path,
                    message: 'Error while compiling forms'
                }));
            }

            if (++paths_processed >= paths_total) {
                return _callback(null, rv);
            }
        });
    });

    return rv;
};


/**
 */
var compile_templates = function (_template_paths, _callback) {

    var rv = {};
    var template_names = Object.keys(_template_paths);

    var paths_processed = 0;
    var paths_total = template_names.length;

    template_names.forEach(function (_name) {

        fs.readFile(_template_paths[_name], 'utf-8', function (_err, _data) {
            if (_err) {
                return _callback(new CompilationError(_err, {
                    path: _template_paths[_name],
                    message: 'Unable to read template file'
                }));
            }

            rv[_name] = handlebars.compile(_data);

            if (++paths_processed >= paths_total) {
                return _callback(null, rv);
            }
        });
    });

    return rv;
};


/**
 */
var write_forms = function (_forms, _templates, _callback) {

    var forms_processed = 0;
    var forms_total = _forms.length;

    /* Process individual forms:
        This produces one or more files in `output/forms`. */

    _forms.forEach(function (_form) {

        /* Render per-form template content */
        var rendered = _templates.form.call(this, _form);

        /* Generate path to single-form output file */
        var path = 'output/forms/' + _form.meta.code + '.c';

        /* Write single-form output file */
        fs.writeFile(path, rendered, function (_e) {
            if (_e) {
                return _callback(new CompilationError(_e, {
                    message: 'Unable to open file for writing',
                    path: path, code: _form.meta.code
                }));
            }

            if (++forms_processed >= forms_total) {
                return _callback(null, _forms);
            }
        });
    });
};


/**
 */
var write_main = function (_forms, _templates, _callback) {

    /* Process main template:
        This produces exactly one file, `output/main/main.c`. */

    var path = 'output/main/main.c';

    var rendered = _templates.main.call(this, {
        forms: _forms, meta: { count: _forms.length }
    });

    fs.writeFile(path, rendered, function (_err) {
        if (_err) {
            return _callback(new CompilationError(_err, {
                message: 'Unable to write template output',
                path: path
            }));
        }
        _callback(null);
    });
};


/**
 */
var register_helpers = exports.register_helpers = function () {

    /* Template helper:
        Allows iteration over the keys/values in an object. */

    handlebars.registerHelper('eachProperty', function (_ctx, _options) {

        var rv = '';

        for (var k in _ctx) {
            rv += _options.fn({
                property: k, value: _ctx[k]
            });
        }

        return rv;
    });

    /* Template helper:
        Returns an upper-cased version of a string. */

    handlebars.registerHelper('toUpper', function (_value) {

        return (_value || '').toUpperCase();
    });
};


/**
 */
var main = exports.main = function (_paths, _callback) {

    /* Compile templates:
        These files are C source with templating instructions. */

    var template_names = {
        main: './templates/muvuku.c.template',
        form: './templates/muvuku-data.c.template'
    };

    /* Helpers:
        This adds a couple useful functions to handlebars.js. */

    register_helpers();


    /* Start building:
        We need to read and compile each of the templates specified
        above, read and parse the JSON input file, and then use the
        compiled template functions to write out two types of files. */

    compile_templates(template_names, function (_e1, _templates) {

        if (_e1) {
            return _callback(new CompilationError(_e1, {
                message: 'Template compilation failed'
            }));
        }

        /* JSON processing:
            Build an in-memory representation of all forms from each
            file in `_paths`. Unconditionally calls `_callback`; errors
            are handled in the node.js style (non-null first argument). */

        process_json(_paths, function (_e2, _forms) {

            if (_e2) {
                return _callback(new CompilationError(_e2, {
                    message: 'Failure while processing JSON'
                }));
            }

            try {

                /* Emit individual form files in `output/forms`... */
                write_forms(_forms, _templates, function (_e3) {
                    if (_e3) {
                        return _callback(new CompilationError(_e3, {
                            message: 'Failure while generating forms'
                        }));
                    }

                    /* ...then program driver code in `output/main` */
                    write_main(_forms, _templates, function (_e4) {
                        if (_e4) {
                            return _callback(new CompilationError(_e4, {
                                message: 'Failure while generating `main`'
                            }));
                        }
                        return _callback(null, _forms);
                    });
                });

            } catch (_e) {

                return _callback(new CompilationError(_e, {
                    message: 'Failure while rendering template'
                }));
            }
        });
    });

    return this;
};


main.call(
    this, process.argv.slice(2), function (err, forms) {

        if (err) {
            return fatal('Compilation failure', err);
        }

        forms.forEach(function (_form, _i) {
            if (_form.meta.length.upper > 160) {
                return fatal(
                    'Maximum message length is > 160 characters', {
                        length: _form.meta.length
                    }
                );
            }
        });

        process.stderr.write(
            'Completed successfully: Wrote ' + forms.length + ' files\n'
        );
    }
);

