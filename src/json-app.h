#ifndef __JSON_APP_H__
#define __JSON_APP_H__
#include <stdarg.h>
#include <stdbool.h>
#include <json-c/json.h>
#include <uci.h>

#define __jsonapp_init__\
        __attribute__((constructor))

struct jsonapp_parse_ctx;

struct jsonapp_parse_backend {
        struct jsonapp_parse_backend *next;
        struct jsonapp_parse_ctx *(*init)(struct jsonapp_parse_ctx *jctx);
        int (*jsonapp_process_json)(struct jsonapp_parse_ctx *jctx);
        void (*free_jsonapp_context)(struct jsonapp_parse_ctx *jctx);
};

struct jsonapp_parse_ctx {
        struct jsonapp_parse_backend *backend;
        struct json_object *root;
        struct uci_context *uci_ctx;
#if 0
        json_object *root;
        struct uci_context *uci;
        struct uci_package *package;
        int print_uci;
        char package_name[]; /* this must always be last */
#endif
};

void jsonapp_die(const char *fmt, ...);

void jsonapp_register_backend(struct jsonapp_parse_backend *backend);
int jsonapp_has_config(struct jsonapp_parse_ctx *jctx, char *name, 
                       void (*reset_uci)(struct jsonapp_parse_ctx *jctx),
                       bool create);

#define foreach_parse_backend(_backend, _backend_list) \
        for ((_backend) = (_backend_list); _backend; _backend = _backend->next)

#endif /* __JSON_APP_H__ */

