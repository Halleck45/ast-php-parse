#pragma once
#ifdef __cplusplus
extern "C" {
#endif

int  ast_init(void);             // init SAPI embed (une seule fois)
void ast_shutdown(void);         // shutdown
// Retourne JSON alloué via malloc (Go fera C.free)
char* ast_parse_code_json(const char* code, const char* filename_hint, unsigned int ast_version, unsigned int flags);
char* ast_parse_file_json(const char* path, unsigned int ast_version, unsigned int flags);

#ifdef __cplusplus
}
#endif
