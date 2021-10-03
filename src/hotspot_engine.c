#include <stdio.h>
#include "json-app.h"
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

struct uci_package *hotspot_package;

/* truncate the entire /etc/config/hotspot file */
static void reset_hotspot_uci(struct jsonapp_parse_ctx *jctx)
{
        if (truncate("/etc/config/hotspot", 0) == -1) {
                jsonapp_die("unable to reset /etc/config/hotspot");
        }

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
         * if it does not exist, create it. 
         */
        do {
                has_config = jsonapp_has_config(jctx, "hotspot", reset_hotspot_uci, true);
        } while(!has_config);
        return jctx;
}

static void hotspot_process_wlangrp_obj(struct jsonapp_parse_ctx *jctx, 
                                         struct json_object *wg_obj,
                                         void *user_data)
{
        struct json_object *wlans_obj;
        struct json_object *one_wlan;
        struct json_object *rservers;
        struct json_object *rserver1;
        struct json_object *rserver2;
        struct json_object *servers;
        struct json_object *server;
        struct json_object *galist;
        struct json_object *one_ga;
        wlans_obj = jsonapp_object_get_object_by_name(wg_obj, "wlans", json_type_array);
        
        /* process only 1 wlan object. */
        one_wlan = json_object_array_get_idx(wlans_obj, 0);
        rservers = jsonapp_object_get_object_by_name(one_wlan, "RadiusServerList", json_type_array);
        
        rserver1 = json_object_array_get_idx(rservers, 0);
        rserver2 = json_object_array_get_idx(rservers, 1);

        struct uci_section *s = NULL;
        char tuple[128];
        struct uci_ptr ptr;
        struct json_object *obj;

        uci_add_section(jctx->uci_ctx, hotspot_package, "wifi", &s);
        uci_save(jctx->uci_ctx, hotspot_package);

        sprintf(tuple, "%s.%s", hotspot_package->e.name, s->e.name);
        uci_lookup_ptr(jctx->uci_ctx, &ptr, tuple, true);

        servers = jsonapp_object_get_object_by_name(rserver1, "servers", json_type_array);
        server = json_object_array_get_idx(servers, 0);
        jsonapp_set_new_option(jctx, server, "ip", json_type_string,
                               hotspot_package, s, "HS_RADIUS", NULL);
        jsonapp_set_new_option(jctx, server, "secret", json_type_string,
                               hotspot_package, s, "HS_RADSECRET", NULL);       
        jsonapp_set_new_option(jctx, server, "ip", json_type_string,
                               hotspot_package, s, "HS_UAMALLOW", NULL);
        jsonapp_set_new_option(jctx, server, "port", json_type_string,
                               hotspot_package, s, "HS_PORT", NULL);  

        servers = jsonapp_object_get_object_by_name(rserver2, "servers", json_type_array);       
        server = json_object_array_get_idx(servers, 0);
        jsonapp_set_new_option(jctx, server, "ip", json_type_string,
                               hotspot_package, s, "HS_RADIUS2", NULL);  

        galist = jsonapp_object_get_object_by_name(one_wlan, "GuestAccessList", json_type_array);
        one_ga = json_object_array_get_idx(galist, 0);
        jsonapp_set_new_option(jctx, one_ga, "portalUrl", json_type_string,
                               hotspot_package, s, "HS_UAMHOMEPAGE", NULL);  

        uci_save(jctx->uci_ctx, hotspot_package);
        return;
}

static int hotspot_process_json(struct jsonapp_parse_ctx *jctx)
{
        struct json_object *wlan_grp_obj;
        wlan_grp_obj = jsonapp_object_get_object_by_name(jctx->root, "WlanGroupList", json_type_array);
        jsonapp_process_array(jctx, wlan_grp_obj, NULL, hotspot_process_wlangrp_obj);
        uci_commit(jctx->uci_ctx, &hotspot_package, true);
        return 0;
}

static void hotspot_exit_context(struct jsonapp_parse_ctx *jctx)
{
        /* we did not do any specific allocation in hotspot_init_context()
         * so no need to do cleanup here. just return immediately. */
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

