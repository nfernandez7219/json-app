#include <stdio.h>
#include "json-app.h"
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

struct uci_package *hotspot_package;

static void reset_hotspot_uci(struct jsonapp_parse_ctx *jctx)
{
        if (uci_load(jctx->uci_ctx, "hotspot", &hotspot_package) != UCI_OK) {
                jsonapp_die("error loading /etc/config/hotspot");
        }

        return;
}

static struct jsonapp_parse_ctx *hotspot_init_context(struct jsonapp_parse_ctx *jctx)
{
        int has_config;

        /* find /etc/config/hotspot.
         *
         * if it exists, reset it completely. we will replace all contents 
         * using the input json file.
         *
         * if it does not exist, error gross. this is assumed to exist!
         */
        has_config = jsonapp_has_config(jctx, "hotspot", reset_hotspot_uci, false);
        if (!has_config)
                jsonapp_die("cannot find /etc/config/hotspot");
        return jctx;
}

void hotspot_set_option(struct jsonapp_parse_ctx *jctx,
                        struct json_object *obj,
                        char *obj_member,
                        char *option_name,
                        enum json_type expected_type)
{
        char tuple[512];
        struct json_object *member;
        struct uci_ptr ptr;
        member = jsonapp_object_get_object_by_name(obj, obj_member, expected_type);
        sprintf(tuple, "hotspot.@wifi[0].%s=%s", option_name,
                       json_object_get_string(member));
        uci_lookup_ptr(jctx->uci_ctx, &ptr, tuple, true);
        uci_set(jctx->uci_ctx, &ptr);
        return;
}

static int hotspot_process_json(struct jsonapp_parse_ctx *jctx, struct json_object *root)
{
        struct json_object *wlan;
        struct json_object *radius;
        struct json_object *guest_acl;
        struct json_object *server;

        /* process only 1 wlan object */
        wlan = jsonapp_get_wlans(jsonapp_get_wlangrp(root));
        wlan = json_object_array_get_idx(wlan, 0);

        /* process server0 in radiusserver0 */
        radius = json_object_array_get_idx(jsonapp_get_radius_servers(wlan), 0);
        server = json_object_array_get_idx(jsonapp_get_servers(radius), 0);
        hotspot_set_option(jctx, server, "ip", "HS_RADIUS", json_type_string);
        hotspot_set_option(jctx, server, "secret", "HS_RADSECRET", json_type_string);
        hotspot_set_option(jctx, server, "ip", "HS_UAMALLOW", json_type_string);
        hotspot_set_option(jctx, server, "port", "HS_PORT", json_type_string);

        /* process server0 in radiusserver1 */
        radius = json_object_array_get_idx(jsonapp_get_radius_servers(wlan), 1);
        server = json_object_array_get_idx(jsonapp_get_servers(radius), 0);
        hotspot_set_option(jctx, server, "ip", "HS_RADIUS2", json_type_string);      

        /* guest access list */
        guest_acl = jsonapp_get_guest_acl_list(wlan);
        guest_acl = json_object_array_get_idx(guest_acl, 0);
        hotspot_set_option(jctx, guest_acl, "portalUrl", "HS_UAMHOMEPAGE", json_type_string);
        
        uci_save(jctx->uci_ctx, hotspot_package);
        uci_commit(jctx->uci_ctx, &hotspot_package, true);
        return 0;
}

static void hotspot_exit_context(struct jsonapp_parse_ctx *jctx)
{
        uci_unload(jctx->uci_ctx, hotspot_package);
        return;
}

static struct jsonapp_parse_backend hotspot_parse_backend = {
        .init = hotspot_init_context,
        .process_json = hotspot_process_json,
        .exit = hotspot_exit_context,
};

static void __jsonapp_init__ hotspot_engine_init(void)
{
        jsonapp_register_backend(&hotspot_parse_backend);
}

