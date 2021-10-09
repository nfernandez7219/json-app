#ifndef __JSON_APP_H__
#define __JSON_APP_H__
#include <stdarg.h>
#include <stdbool.h>
#include <json-c/json.h>
#include <uci.h>
#include <mosquitto.h>

#define __jsonapp_init__\
        __attribute__((constructor))

struct jsonapp_parse_ctx;

struct jsonapp_parse_backend {
        struct jsonapp_parse_backend *next;
        struct jsonapp_parse_ctx *jctx;
        struct jsonapp_parse_ctx *(*init)(struct jsonapp_parse_ctx *jctx);
        int (*process_json)(struct jsonapp_parse_ctx *jctx, struct json_object *root);
        void (*exit)(struct jsonapp_parse_ctx *jctx);
};

struct jsonapp_mqtt_ctx {
        char *iface_name;
        char *user;
        char *password;
        char *host;
        uint8_t mac_address[6];
        struct mosquitto *mosq;
        char jsonapp_client_id[64];
};

struct jsonapp_parse_ctx {
        struct jsonapp_parse_backend *backend;
        struct uci_context *uci_ctx;
        struct jsonapp_mqtt_ctx mqtt;
};

void jsonapp_die(const char *fmt, ...);

void jsonapp_register_backend(struct jsonapp_parse_backend *backend);
int jsonapp_has_config(struct jsonapp_parse_ctx *jctx, char *name, 
                       void (*reset_uci)(struct jsonapp_parse_ctx *jctx),
                       bool create);

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

struct json_object *jsonapp_get_wlangrp(struct json_object *root);
struct json_object *jsonapp_get_wlans(struct json_object *wlangrp);
struct json_object *jsonapp_get_radius_servers(struct json_object *wlans);
struct json_object *jsonapp_get_guest_acl_list(struct json_object *wlans);
struct json_object *jsonapp_get_servers(struct json_object *radius_server);

#define foreach_parse_backend(_backend, _backend_list) \
        for ((_backend) = (_backend_list); _backend; _backend = _backend->next)

#endif /* __JSON_APP_H__ */

