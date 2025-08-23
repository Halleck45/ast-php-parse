#include <sapi/embed/php_embed.h>
#include <zend_exceptions.h>
#include <zend_API.h>
#include <Zend/zend_smart_str.h>
#include <ext/standard/php_filestat.h>
#include <ext/json/php_json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mutex global si NTS (optionnel, selon build)
static int g_inited = 0;

static unsigned int ast_current_version_fallback(void);

int ast_init(void) {
    if (g_inited) return 1;

    // Optionnel: ini_entries en dur (opcache.enable_cli=1, etc.)
    php_embed_module.ini_entries =
        "display_errors=0\n"
        "log_errors=0\n"
        "opcache.enable_cli=1\n"
        "extension=ast\n"
        "opcache.jit=0\n";

    if (php_embed_init(0, NULL) == FAILURE) return 0;

    // Ne pas vérifier via zend_is_callable pour éviter des callbacks invalides sous certains builds.

    g_inited = 1;
    return 1;
}

void ast_shutdown(void) {
    if (!g_inited) return;
    php_embed_shutdown();
    g_inited = 0;
}


#include <Zend/zend_smart_str.h>
#include <ext/json/php_json.h>

#ifndef PHP_JSON_PARTIAL_OUTPUT_ON_ERROR
# define PHP_JSON_PARTIAL_OUTPUT_ON_ERROR (1<<12)
#endif
#ifndef PHP_JSON_UNESCAPED_SLASHES
# define PHP_JSON_UNESCAPED_SLASHES (1<<4)
#endif
#ifndef PHP_JSON_PRESERVE_ZERO_FRACTION
# define PHP_JSON_PRESERVE_ZERO_FRACTION (1<<6)
#endif

/* Fallback userland json_encode() si l'API C échoue */
static char* zval_to_json_alloc_via_json_encode(zval *zv) {
    zval fname, arg, retval;
    ZVAL_STRINGL(&fname, "json_encode", sizeof("json_encode") - 1);
    ZVAL_COPY(&arg, zv);

    if (call_user_function(EG(function_table), NULL, &fname, &retval, 1, &arg) == FAILURE) {
        zval_ptr_dtor(&fname);
        zval_ptr_dtor(&arg);
        return strdup("{\"error\":\"json_encode call failed\"}");
    }
    zval_ptr_dtor(&fname);
    zval_ptr_dtor(&arg);

    if (Z_TYPE(retval) == IS_STRING) {
        char *out = strndup(Z_STRVAL(retval), Z_STRLEN(retval));
        zval_ptr_dtor(&retval);
        return out;
    }
    zval_ptr_dtor(&retval);
    return strdup("{\"error\":\"json_encode returned non-string\"}");
}

static char* zval_to_json_alloc(zval *zv) {
    smart_str buf = {0};

    /* Options conseillés : sorties lisibles/robustes */
    int opts = PHP_JSON_UNESCAPED_SLASHES
             | PHP_JSON_PRESERVE_ZERO_FRACTION
             | PHP_JSON_PARTIAL_OUTPUT_ON_ERROR;
    /* Grande profondeur, l'AST peut être très imbriqué */
    int depth = 1<<20; /* ~1 million, suffisant */

    /* Utilise l'API C JSON (PHP 8+) */
#ifdef php_json_encode_ex
    if (php_json_encode_ex(&buf, zv, opts, depth) == FAILURE) {
        smart_str_free(&buf);
        return zval_to_json_alloc_via_json_encode(zv);
    }
#else
    /* Compat : anciennes versions n'ont pas _ex; tente simple + fallback */
    php_json_encode(&buf, zv, opts);
#endif

    smart_str_0(&buf);
    if (buf.s) {
        char *out = strndup(ZSTR_VAL(buf.s), ZSTR_LEN(buf.s));
        smart_str_free(&buf);
        return out;
    }

    /* Fallback final improbable */
    return zval_to_json_alloc_via_json_encode(zv);
}


/* ==== Helpers & cache de classe ast\Node ================================= */

static zend_class_entry *ce_ast_node = NULL;

/* Déclare/initialise ce_ast_node une seule fois */
static int ensure_ast_node_ce(void) {
    if (ce_ast_node) return 1;
    zend_string *cn = zend_string_init("ast\\Node", sizeof("ast\\Node") - 1, 0);
    ce_ast_node = zend_lookup_class(cn);
    zend_string_release(cn);
    return ce_ast_node != NULL;
}

/* Appelle ast\get_kind_name($kind) et renvoie une zend_string* (ownership transféré) */
static zend_string* ast_kind_name_zstr(zend_long kind) {
    zval fname, arg, ret;
    ZVAL_STRINGL(&fname, "ast\\get_kind_name", sizeof("ast\\get_kind_name") - 1);
    ZVAL_LONG(&arg, kind);

    if (call_user_function(EG(function_table), NULL, &fname, &ret, 1, &arg) == SUCCESS
        && Z_TYPE(ret) == IS_STRING) {
        zend_string *name = zend_string_copy(Z_STR(ret));
        zval_ptr_dtor(&ret);
        zval_ptr_dtor(&fname);
        return name; /* caller fera add_assoc_str() qui prend ownership */
    }

    /* Fallback */
    zval_ptr_dtor(&fname);
    if (Z_TYPE(ret) != IS_UNDEF) {
        zval_ptr_dtor(&ret);
    }
    return zend_string_init("UNKNOWN", sizeof("UNKNOWN") - 1, 0);
}

/* Normalisation récursive d’un zval vers un zval tableau/scalar (dst).
   - ast\Node  -> array via node_to_array (récursion)
   - array     -> array avec normalisation de chaque élément
   - scalar    -> copy
*/
static void normalize_zval(zval *src, zval *dst);

/* ==== Implémentation de node_to_array ==================================== */
static int node_to_array(zval *return_value, zval *node) {
    if (!ensure_ast_node_ce()) {
        array_init(return_value);
        add_assoc_string(return_value, "error", "ast_node_class_not_found");
        return FAILURE;
    }

    if (Z_TYPE_P(node) != IS_OBJECT || !instanceof_function(Z_OBJCE_P(node), ce_ast_node)) {
        ZVAL_COPY(return_value, node);
        return SUCCESS;
    }

    array_init(return_value);

    zval rv_kind, rv_flags, rv_lineno, rv_children;
    zval *z_kind = zend_read_property(ce_ast_node, Z_OBJ_P(node), ZEND_STRL("kind"), 1, &rv_kind);
    zval *z_flags = zend_read_property(ce_ast_node, Z_OBJ_P(node), ZEND_STRL("flags"), 1, &rv_flags);
    zval *z_lineno = zend_read_property(ce_ast_node, Z_OBJ_P(node), ZEND_STRL("lineno"), 1, &rv_lineno);
    zval *z_children = zend_read_property(ce_ast_node, Z_OBJ_P(node), ZEND_STRL("children"), 1, &rv_children);

    zend_long kind  = zval_get_long(z_kind);
    zend_long flags = zval_get_long(z_flags);
    zend_long line  = zval_get_long(z_lineno);

    zend_string *kind_name = ast_kind_name_zstr(kind);
    add_assoc_str(return_value, "nodeType", kind_name); // ownership transféré
    add_assoc_long(return_value, "kind",  kind);
    add_assoc_long(return_value, "flags", flags);
    add_assoc_long(return_value, "lineno", line);

    zval arr_children;
    array_init(&arr_children);

    if (z_children && Z_TYPE_P(z_children) == IS_ARRAY) {
        zval *val;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_children), val) {
            zval norm;
            normalize_zval(val, &norm);
            add_next_index_zval(&arr_children, &norm);
        } ZEND_HASH_FOREACH_END();
    } else if (z_children && Z_TYPE_P(z_children) != IS_UNDEF) {
        zval norm;
        normalize_zval(z_children, &norm);
        add_next_index_zval(&arr_children, &norm);
    }

    add_assoc_zval(return_value, "children", &arr_children);
    return SUCCESS;
}


/* ==== Implémentation normalize_zval ====================================== */
static void normalize_zval(zval *src, zval *dst) {
    if (Z_TYPE_P(src) == IS_OBJECT && ensure_ast_node_ce()
        && instanceof_function(Z_OBJCE_P(src), ce_ast_node)) {
        // ast\Node -> array
        array_init(dst);
        if (node_to_array(dst, src) != SUCCESS) {
            // Échec: construire un petit objet d'erreur en place
            array_init(dst);
            add_assoc_string(dst, "error", "node_normalize_failed");
        }
        return;
    }

    if (Z_TYPE_P(src) == IS_ARRAY) {
        array_init(dst);
        zval *val;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(src), val) {
            zval sub;
            normalize_zval(val, &sub);
            add_next_index_zval(dst, &sub);
        } ZEND_HASH_FOREACH_END();
        return;
    }

    // Scalars / null / resources -> copie
    ZVAL_COPY(dst, src);
}
static char* do_parse_code(const char* code, const char* filename, unsigned int ast_version, unsigned int flags) {
    // 1) Déterminer la version AST à utiliser
    unsigned int version_to_use = ast_version ? ast_version : ast_current_version_fallback();

    // 2) Appeler ast\parse_code($code, $version, $flags)
    zval fn_parse, args_parse[3], ret_node;
    ZVAL_STRINGL(&fn_parse, "ast\\parse_code", sizeof("ast\\parse_code") - 1);
    ZVAL_STRING(&args_parse[0], code);
    ZVAL_LONG(&args_parse[1], (zend_long)version_to_use);
    ZVAL_LONG(&args_parse[2], (zend_long)flags);

    if (call_user_function(EG(function_table), NULL, &fn_parse, &ret_node, 3, args_parse) == FAILURE) {
        zval_ptr_dtor(&fn_parse);
        zval_ptr_dtor(&args_parse[0]); // string
        return strdup("{\"error\":{\"message\":\"ast\\\\parse_code call failed\"}}");
    }
    zval_ptr_dtor(&fn_parse);
    zval_ptr_dtor(&args_parse[0]); // string

    // 3) Simplification maximale: retourner directement json_encode($ret_node)
    // Évite toute normalisation manuelle potentiellement fragile.
    char* out = zval_to_json_alloc_via_json_encode(&ret_node);
    zval_ptr_dtor(&ret_node);
    return out;
}

char* ast_parse_code_json(const char* code, const char* filename_hint, unsigned int ast_version, unsigned int flags) {
    if (!g_inited) return strdup("{\"error\":\"not_initialized\"}");
    return do_parse_code(code, filename_hint ? filename_hint : "-", ast_version, flags);
}

char* ast_parse_file_json(const char* path, unsigned int ast_version, unsigned int flags) {
    FILE *f = fopen(path, "rb");
    if (!f) return strdup("{\"error\":\"cannot_open\"}");

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return strdup("{\"error\":\"seek_failed\"}"); }
    long n = ftell(f);
    if (n < 0) { fclose(f); return strdup("{\"error\":\"tell_failed\"}"); }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return strdup("{\"error\":\"seek_failed\"}"); }

    char *buf = (char*)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return strdup("{\"error\":\"oom\"}"); }

    size_t rd = fread(buf, 1, (size_t)n, f);
    fclose(f);
    if (rd != (size_t)n) { free(buf); return strdup("{\"error\":\"read_failed\"}"); }

    buf[n] = 0;
    char* out = do_parse_code(buf, path, ast_version, flags);
    free(buf);
    return out;

}

static unsigned int ast_current_version_fallback(void) {
    // 1) Essayer la constante AST_CURRENT
    zval *c = zend_get_constant_str("AST_CURRENT", sizeof("AST_CURRENT") - 1);
    if (c && Z_TYPE_P(c) == IS_LONG) {
        return (unsigned int) Z_LVAL_P(c);
    }

    // 2) Dernier recours : valeur récente connue
    return 120;
}
