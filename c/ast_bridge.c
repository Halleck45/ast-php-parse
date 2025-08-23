#include <sapi/embed/php_embed.h>
#include <zend_exceptions.h>
#include <zend_API.h>
#include <ext/standard/php_filestat.h>
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
        "opcache.jit=0\n";

    if (php_embed_init(0, NULL) == FAILURE) return 0;

    // Vérifier que la fonction ast\parse_code existe
    zval func;
    ZVAL_STRING(&func, "ast\\parse_code");
    zval retval;
    if (!zend_is_callable(&func, 0, NULL)) {
        zval_ptr_dtor_str(&func);
        php_embed_shutdown();
        return 0;
    }
    zval_ptr_dtor_str(&func);

    g_inited = 1;
    return 1;
}

void ast_shutdown(void) {
    if (!g_inited) return;
    php_embed_shutdown();
    g_inited = 0;
}

static char* zval_to_json_alloc(zval *zv) {
    // Utiliser l’extension json de PHP pour sérialiser en JSON :
    // php_json_encode(&str, zv, 0) sur PHP>=8 (headers json)
    // Pour rester bref ici, tu peux aussi appeler json_encode via zend_call_function.
    // Retourne un char* malloué (strdup) que Go libérera avec C.free().
    // ... (implémentation à compléter dans le repo)
    return strdup("{\"error\":\"json_not_implemented_in_snippet\"}");
}

static int node_to_array(zval *return_value, zval *node) {
    // Convertir récursivement ast\Node -> array { nodeType, kind, flags, lineno, children[] }
    // Lire propriétés via zend_read_property(...)
    // ... (implémentation dans ton repo)
    return SUCCESS;
}

static char* do_parse_code(const char* code, const char* filename, unsigned int ast_version, unsigned int flags) {
    zval fname, args[3], retval;

    unsigned int version_to_use = ast_version ? ast_version : ast_current_version_fallback();

    ZVAL_STRING(&fname, "ast\\parse_code");
    ZVAL_STRING(&args[0], code);
    ZVAL_LONG(&args[1], (long)version_to_use);
    ZVAL_LONG(&args[2], (long)flags);

    if (call_user_function(EG(function_table), NULL, &fname, &retval, 3, args) == FAILURE) {
        zval_ptr_dtor(&fname);
        zval_ptr_dtor(&args[0]);
        // args[1]/[2] sont des scalars
        return strdup("{\"error\":{\"message\":\"call_user_function failed\"}}");
    }
    zval_ptr_dtor(&fname);
    zval_ptr_dtor(&args[0]);

    // retval est ast\Node ou array
    zval rootArr;
    array_init(&rootArr);
    // root -> array normalisée
    // version -> {php, ast_ext, ast_version}
    zval norm;
    if (node_to_array(&norm, &retval) == SUCCESS) {
        add_assoc_zval(&rootArr, "root", &norm);
    } else {
        add_assoc_string(&rootArr, "error", "normalize_failed");
    }
    zval_ptr_dtor(&retval);

    // Ajoute version info (à implémenter)
    zval ver;
    array_init(&ver);
    add_assoc_string(&ver, "engine", "embed");
    add_assoc_zval(&rootArr, "version", &ver);

    char* out = zval_to_json_alloc(&rootArr);
    zval_ptr_dtor(&rootArr);
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

    // 2) Sinon, ast\get_supported_versions() et on prend le max
    zval fname, retval;
    ZVAL_STRINGL(&fname, "ast\\get_supported_versions", sizeof("ast\\get_supported_versions") - 1);
    if (call_user_function(EG(function_table), NULL, &fname, &retval, 0, NULL) == SUCCESS
        && Z_TYPE(retval) == IS_ARRAY) {

        unsigned int maxv = 0;
        zval *zv_ver;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL(retval), zv_ver) {
            if (Z_TYPE_P(zv_ver) == IS_LONG) {
                unsigned int v = (unsigned int) Z_LVAL_P(zv_ver);
                if (v > maxv) maxv = v;
            }
        } ZEND_HASH_FOREACH_END();

        zval_ptr_dtor(&retval);
        zval_ptr_dtor(&fname);
        if (maxv) return maxv;
    } else {
        zval_ptr_dtor(&fname);
    }

    // 3) Dernier recours : valeur récente connue
    return 120;
}
