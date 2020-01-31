/*
 * Copyright 2009 Piotr Caban
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include <math.h>
#include <assert.h>
#include <wchar.h>

#include "jscript.h"
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static const WCHAR descriptionW[] = {'d','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR messageW[] = {'m','e','s','s','a','g','e',0};
static const WCHAR nameW[] = {'n','a','m','e',0};
static const WCHAR numberW[] = {'n','u','m','b','e','r',0};
static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};

/* ECMA-262 3rd Edition    15.11.4.4 */
static HRESULT Error_toString(script_ctx_t *ctx, vdisp_t *vthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    jsdisp_t *jsthis;
    jsstr_t *name = NULL, *msg = NULL, *ret = NULL;
    jsval_t v;
    HRESULT hres;

    static const WCHAR object_errorW[] = {'[','o','b','j','e','c','t',' ','E','r','r','o','r',']',0};

    TRACE("\n");

    jsthis = get_jsdisp(vthis);
    if(!jsthis || ctx->version < 2) {
        if(r) {
            jsstr_t *str;

            str = jsstr_alloc(object_errorW);
            if(!str)
                return E_OUTOFMEMORY;
            *r = jsval_string(str);
        }
        return S_OK;
    }

    hres = jsdisp_propget_name(jsthis, nameW, &v);
    if(FAILED(hres))
        return hres;

    if(!is_undefined(v)) {
        hres = to_string(ctx, v, &name);
        jsval_release(v);
        if(FAILED(hres))
            return hres;
    }

    hres = jsdisp_propget_name(jsthis, messageW, &v);
    if(SUCCEEDED(hres)) {
        if(!is_undefined(v)) {
            hres = to_string(ctx, v, &msg);
            jsval_release(v);
        }
    }

    if(SUCCEEDED(hres)) {
        unsigned name_len = name ? jsstr_length(name) : 0;
        unsigned msg_len = msg ? jsstr_length(msg) : 0;

        if(name_len && msg_len) {
            WCHAR *ptr;

            ret = jsstr_alloc_buf(name_len + msg_len + 2, &ptr);
            if(ret) {
                jsstr_flush(name, ptr);
                ptr[name_len] = ':';
                ptr[name_len+1] = ' ';
                jsstr_flush(msg, ptr+name_len+2);
            }else {
                hres = E_OUTOFMEMORY;
            }
        }else if(name_len) {
            ret = name;
            name = NULL;
        }else if(msg_len) {
            ret = msg;
            msg = NULL;
        }else {
            ret = jsstr_alloc(object_errorW);
        }
    }

    if(msg)
        jsstr_release(msg);
    if(name)
        jsstr_release(name);
    if(FAILED(hres))
        return hres;
    if(!ret)
        return E_OUTOFMEMORY;

    if(r)
        *r = jsval_string(ret);
    else
        jsstr_release(ret);
    return S_OK;
}

static HRESULT Error_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(ctx, JS_E_FUNCTION_EXPECTED, NULL);
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const builtin_prop_t Error_props[] = {
    {toStringW,                 Error_toString,                     PROPF_METHOD}
};

static const builtin_info_t Error_info = {
    JSCLASS_ERROR,
    {NULL, Error_value, 0},
    ARRAY_SIZE(Error_props),
    Error_props,
    NULL,
    NULL
};

static const builtin_info_t ErrorInst_info = {
    JSCLASS_ERROR,
    {NULL, Error_value, 0},
    0,
    NULL,
    NULL,
    NULL
};

static HRESULT alloc_error(script_ctx_t *ctx, jsdisp_t *prototype,
        jsdisp_t *constr, jsdisp_t **ret)
{
    jsdisp_t *err;
    HRESULT hres;

    err = heap_alloc_zero(sizeof(*err));
    if(!err)
        return E_OUTOFMEMORY;

    if(prototype)
        hres = init_dispex(err, ctx, &Error_info, prototype);
    else
        hres = init_dispex_from_constr(err, ctx, &ErrorInst_info,
            constr ? constr : ctx->error_constr);
    if(FAILED(hres)) {
        heap_free(err);
        return hres;
    }

    *ret = err;
    return S_OK;
}

static HRESULT create_error(script_ctx_t *ctx, jsdisp_t *constr,
        UINT number, jsstr_t *msg, jsdisp_t **ret)
{
    jsdisp_t *err;
    HRESULT hres;

    hres = alloc_error(ctx, NULL, constr, &err);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(err, numberW, PROPF_WRITABLE | PROPF_CONFIGURABLE,
                                       jsval_number((INT)number));
    if(FAILED(hres)) {
        jsdisp_release(err);
        return hres;
    }

    hres = jsdisp_define_data_property(err, messageW,
                                       PROPF_WRITABLE | PROPF_ENUMERABLE | PROPF_CONFIGURABLE,
                                       jsval_string(msg));
    if(SUCCEEDED(hres))
        hres = jsdisp_define_data_property(err, descriptionW, PROPF_WRITABLE | PROPF_CONFIGURABLE,
                                           jsval_string(msg));
    if(FAILED(hres)) {
        jsdisp_release(err);
        return hres;
    }

    *ret = err;
    return S_OK;
}

static HRESULT error_constr(script_ctx_t *ctx, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r, jsdisp_t *constr) {
    jsdisp_t *err;
    UINT num = 0;
    jsstr_t *msg = NULL;
    HRESULT hres;

    if(argc) {
        double n;

        hres = to_number(ctx, argv[0], &n);
        if(FAILED(hres)) /* FIXME: really? */
            n = NAN;
        if(isnan(n))
            hres = to_string(ctx, argv[0], &msg);
        if(FAILED(hres))
            return hres;
        num = n;
    }

    if(!msg) {
        if(argc > 1) {
            hres = to_string(ctx, argv[1], &msg);
            if(FAILED(hres))
                return hres;
        }else {
            msg = jsstr_empty();
        }
    }

    switch(flags) {
    case INVOKE_FUNC:
    case DISPATCH_CONSTRUCT:
        hres = create_error(ctx, constr, num, msg, &err);
        jsstr_release(msg);
        if(FAILED(hres))
            return hres;

        if(r)
            *r = jsval_obj(err);
        else
            jsdisp_release(err);
        return S_OK;

    default:
        if(msg)
            jsstr_release(msg);
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

static HRESULT ErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, r, ctx->error_constr);
}

static HRESULT EvalErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, r, ctx->eval_error_constr);
}

static HRESULT RangeErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, r, ctx->range_error_constr);
}

static HRESULT ReferenceErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, r, ctx->reference_error_constr);
}

static HRESULT RegExpErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, r, ctx->regexp_error_constr);
}

static HRESULT SyntaxErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, r, ctx->syntax_error_constr);
}

static HRESULT TypeErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, r, ctx->type_error_constr);
}

static HRESULT URIErrorConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");
    return error_constr(ctx, flags, argc, argv, r, ctx->uri_error_constr);
}

HRESULT init_error_constr(script_ctx_t *ctx, jsdisp_t *object_prototype)
{
    static const WCHAR ErrorW[] = {'E','r','r','o','r',0};
    static const WCHAR EvalErrorW[] = {'E','v','a','l','E','r','r','o','r',0};
    static const WCHAR RangeErrorW[] = {'R','a','n','g','e','E','r','r','o','r',0};
    static const WCHAR ReferenceErrorW[] = {'R','e','f','e','r','e','n','c','e','E','r','r','o','r',0};
    static const WCHAR RegExpErrorW[] = {'R','e','g','E','x','p','E','r','r','o','r',0};
    static const WCHAR SyntaxErrorW[] = {'S','y','n','t','a','x','E','r','r','o','r',0};
    static const WCHAR TypeErrorW[] = {'T','y','p','e','E','r','r','o','r',0};
    static const WCHAR URIErrorW[] = {'U','R','I','E','r','r','o','r',0};
    static const WCHAR *names[] = {ErrorW, EvalErrorW, RangeErrorW,
        ReferenceErrorW, RegExpErrorW, SyntaxErrorW, TypeErrorW, URIErrorW};
    jsdisp_t **constr_addr[] = {&ctx->error_constr, &ctx->eval_error_constr,
        &ctx->range_error_constr, &ctx->reference_error_constr, &ctx->regexp_error_constr,
        &ctx->syntax_error_constr, &ctx->type_error_constr,
        &ctx->uri_error_constr};
    static builtin_invoke_t constr_val[] = {ErrorConstr_value, EvalErrorConstr_value,
        RangeErrorConstr_value, ReferenceErrorConstr_value, RegExpErrorConstr_value,
        SyntaxErrorConstr_value, TypeErrorConstr_value, URIErrorConstr_value};

    jsdisp_t *err;
    unsigned int i;
    jsstr_t *str;
    HRESULT hres;

    for(i=0; i < ARRAY_SIZE(names); i++) {
        hres = alloc_error(ctx, i==0 ? object_prototype : NULL, NULL, &err);
        if(FAILED(hres))
            return hres;

        str = jsstr_alloc(names[i]);
        if(!str) {
            jsdisp_release(err);
            return E_OUTOFMEMORY;
        }

        hres = jsdisp_define_data_property(err, nameW, PROPF_WRITABLE | PROPF_CONFIGURABLE,
                                           jsval_string(str));
        jsstr_release(str);
        if(SUCCEEDED(hres))
            hres = create_builtin_constructor(ctx, constr_val[i], names[i], NULL,
                    PROPF_CONSTR|1, err, constr_addr[i]);

        jsdisp_release(err);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static jsstr_t *format_error_message(HRESULT error, const WCHAR *arg)
{
    size_t len, arg_len = 0;
    const WCHAR *res, *pos;
    WCHAR *buf, *p;
    jsstr_t *r;

    if(!is_jscript_error(error))
        return jsstr_empty();

    len = LoadStringW(jscript_hinstance, HRESULT_CODE(error), (WCHAR*)&res, 0);

    pos = wmemchr(res, '|', len);
    if(pos && arg)
        arg_len = lstrlenW(arg);
    r = jsstr_alloc_buf(len + arg_len - (pos ? 1 : 0), &buf);
    if(!r)
        return jsstr_empty();

    p = buf;
    if(pos > res) {
        memcpy(p, res, (pos - res) * sizeof(WCHAR));
        p += pos - res;
    }
    pos = pos ? pos + 1 : res;
    if(arg_len) {
        memcpy(p, arg, arg_len * sizeof(WCHAR));
        p += arg_len;
    }
    if(pos != res + len)
        memcpy(p, pos, (res + len - pos) * sizeof(WCHAR));
    return r;
}

static HRESULT throw_error(script_ctx_t *ctx, HRESULT error, const WCHAR *str, jsdisp_t *constr)
{
    jsdisp_t *err;
    jsstr_t *msg;
    HRESULT hres;

    if(!is_jscript_error(error))
        return error;

    msg = format_error_message(error, str);
    if(!msg)
        return E_OUTOFMEMORY;

    WARN("%s\n", debugstr_jsstr(msg));

    hres = create_error(ctx, constr, error, msg, &err);
    jsstr_release(msg);
    if(FAILED(hres))
        return hres;

    reset_ei(ctx->ei);
    ctx->ei->valid_value = TRUE;
    ctx->ei->value = jsval_obj(err);
    return error;
}

HRESULT throw_reference_error(script_ctx_t *ctx, HRESULT error, const WCHAR *str)
{
    return throw_error(ctx, error, str, ctx->reference_error_constr);
}

HRESULT throw_regexp_error(script_ctx_t *ctx, HRESULT error, const WCHAR *str)
{
    return throw_error(ctx, error, str, ctx->regexp_error_constr);
}

HRESULT throw_syntax_error(script_ctx_t *ctx, HRESULT error, const WCHAR *str)
{
    return throw_error(ctx, error, str, ctx->syntax_error_constr);
}

HRESULT throw_type_error(script_ctx_t *ctx, HRESULT error, const WCHAR *str)
{
    return throw_error(ctx, error, str, ctx->type_error_constr);
}

HRESULT throw_uri_error(script_ctx_t *ctx, HRESULT error, const WCHAR *str)
{
    return throw_error(ctx, error, str, ctx->uri_error_constr);
}

jsdisp_t *create_builtin_error(script_ctx_t *ctx)
{
    jsdisp_t *constr = ctx->error_constr, *r;
    jsexcept_t *ei = ctx->ei;
    HRESULT hres;

    assert(FAILED(ei->error) && ei->error != DISP_E_EXCEPTION);

    if(is_jscript_error(ei->error)) {
        switch(ei->error) {
        case JS_E_SYNTAX:
        case JS_E_MISSING_SEMICOLON:
        case JS_E_MISSING_LBRACKET:
        case JS_E_MISSING_RBRACKET:
        case JS_E_EXPECTED_IDENTIFIER:
        case JS_E_EXPECTED_ASSIGN:
        case JS_E_INVALID_CHAR:
        case JS_E_UNTERMINATED_STRING:
        case JS_E_MISPLACED_RETURN:
        case JS_E_INVALID_BREAK:
        case JS_E_INVALID_CONTINUE:
        case JS_E_LABEL_REDEFINED:
        case JS_E_LABEL_NOT_FOUND:
        case JS_E_EXPECTED_CCEND:
        case JS_E_DISABLED_CC:
        case JS_E_EXPECTED_AT:
            constr = ctx->syntax_error_constr;
            break;

        case JS_E_TO_PRIMITIVE:
        case JS_E_INVALIDARG:
        case JS_E_OBJECT_REQUIRED:
        case JS_E_INVALID_PROPERTY:
        case JS_E_INVALID_ACTION:
        case JS_E_MISSING_ARG:
        case JS_E_FUNCTION_EXPECTED:
        case JS_E_DATE_EXPECTED:
        case JS_E_NUMBER_EXPECTED:
        case JS_E_OBJECT_EXPECTED:
        case JS_E_UNDEFINED_VARIABLE:
        case JS_E_BOOLEAN_EXPECTED:
        case JS_E_VBARRAY_EXPECTED:
        case JS_E_INVALID_DELETE:
        case JS_E_JSCRIPT_EXPECTED:
        case JS_E_ENUMERATOR_EXPECTED:
        case JS_E_ARRAY_EXPECTED:
        case JS_E_NONCONFIGURABLE_REDEFINED:
        case JS_E_NONWRITABLE_MODIFIED:
        case JS_E_PROP_DESC_MISMATCH:
        case JS_E_INVALID_WRITABLE_PROP_DESC:
            constr = ctx->type_error_constr;
            break;

        case JS_E_SUBSCRIPT_OUT_OF_RANGE:
        case JS_E_FRACTION_DIGITS_OUT_OF_RANGE:
        case JS_E_PRECISION_OUT_OF_RANGE:
        case JS_E_INVALID_LENGTH:
            constr = ctx->range_error_constr;
            break;

        case JS_E_ILLEGAL_ASSIGN:
            constr = ctx->reference_error_constr;
            break;

        case JS_E_REGEXP_SYNTAX:
            constr = ctx->regexp_error_constr;
            break;

        case JS_E_INVALID_URI_CODING:
        case JS_E_INVALID_URI_CHAR:
            constr = ctx->uri_error_constr;
            break;
        }
    }

    hres = create_error(ctx, constr, ei->error, jsstr_empty(), &r);
    return SUCCEEDED(hres) ? r : NULL;
}
