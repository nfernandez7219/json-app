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
        int (*process_json)(struct jsonapp_parse_ctx *jctx);
        void (*exit)(struct jsonapp_parse_ctx *jctx);
};

struct jsonapp_parse_ctx {
        struct jsonapp_parse_backend *backend;
        struct json_object *root;
        struct uci_context *uci_ctx;
};

void jsonapp_die(const char *fmt, ...);

void jsonapp_register_backend(struct jsonapp_parse_backend *backend);
int jsonapp_has_config(struct jsonapp_parse_ctx *jctx, char *name, 
                       void (*reset_uci)(struct jsonapp_parse_ctx *jctx),
                       bool create);
static inline struct json_object *jsonapp_get_root(struct jsonapp_parse_ctx *jctx)
{
        return jctx->root;
}

struct json_object *jsonapp_object_get_object_by_name(struct json_object *parent, char *name,
                                                      enum json_type expected_json_type);

void jsonapp_process_array(struct jsonapp_parse_ctx *jctx,
                           struct json_object *obj_arr,
                           void *user_data,
                           void (*process_member)(struct jsonapp_parse_ctx *jctx,
                                                  struct json_object *obj,
                                                  void *user_data));

void jsonapp_set_new_option(struct jsonapp_parse_ctx *jctx,
                            struct json_object *obj,
                            char *obj_member,
                            enum json_type expected_type,
                            struct uci_package *pkg, 
                            struct uci_section *section,
                            char *option, char *value);

#define foreach_parse_backend(_backend, _backend_list) \
        for ((_backend) = (_backend_list); _backend; _backend = _backend->next)

#endif /* __JSON_APP_H__ */

