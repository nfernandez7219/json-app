#include <json-c/json_object.h>
#include <stdio.h>
#include <string.h>
#include "json-app.h"

struct uci_package *wireless_package;

/* just remove all wifi-iface sections */
static void reset_wireless_uci(struct jsonapp_parse_ctx *jctx)
{
        struct uci_element *e;
        struct uci_element *tmp;
        struct uci_ptr ptr;
        char tuple[128];

        if (uci_load(jctx->uci_ctx, "wireless", &wireless_package) != UCI_OK) {
                jsonapp_die("error loading /etc/config/wireless");
        }

        uci_foreach_element_safe(&wireless_package->sections, tmp, e) {
                struct uci_section *s = uci_to_section(e);
                sprintf(tuple, "%s.%s", wireless_package->e.name, s->e.name);
                if (uci_lookup_ptr(jctx->uci_ctx, &ptr, tuple, true) != UCI_OK) {
                        jsonapp_die("error looking section: %s", s->e.name);
                }

                if (strcmp(ptr.s->type, "wifi-iface") != 0) {
                        continue;
                }
               
                //fprintf(stderr, "deleting section: %s.%s\n", wireless_package->e.name, s->e.name);
                if (uci_delete(jctx->uci_ctx, &ptr) != UCI_OK) {
                        jsonapp_die("error deleting section: %s", s->e.name);
                }
        }

        return;
}

static struct jsonapp_parse_ctx *wireless_init_context(struct jsonapp_parse_ctx *jctx)
{
        int has_config;

        /* find /etc/config/wireless.
         *
         * if it exists, reset it by removing the current wifi-iface sections.
         * we will replace all the wifi-iface sections using the input json file.
         *
         * if it does not exist, stop further processing and just warn user that
         * wireless config does not exist.
         */
        has_config = jsonapp_has_config(jctx, "wireless", reset_wireless_uci, false);
        if (!has_config) {
                jsonapp_die("gross error: /etc/config/wireless does not exist!");
        }
        return jctx;
}

static void wireless_exit_context(struct jsonapp_parse_ctx *jctx)
{
        uci_unload(jctx->uci_ctx, wireless_package);
        return;
}

static void wireless_new_iface(struct jsonapp_parse_ctx *jctx, 
                               struct json_object *wlan_obj,
                               bool five_ghz,
                               struct uci_section **section)
{
        struct json_object *obj;
        struct uci_ptr sptr;
        struct uci_ptr ptr;
        char tuple[128];
        char radio_name[64];
        
        memset(&sptr, 0, sizeof(sptr));
        obj = jsonapp_object_get_object_by_name(wlan_obj, "wlanName", json_type_string);
        sptr.package = wireless_package->e.name;
        sptr.value = "wifi-iface";

        sprintf(radio_name, "%s%s", json_object_get_string(obj), five_ghz ? "5GHz": "2_5GHz");
        sptr.section = radio_name;
        uci_set(jctx->uci_ctx, &sptr);
        uci_save(jctx->uci_ctx, wireless_package);
        sprintf(tuple, "%s.%s", sptr.package, sptr.section);
        uci_lookup_ptr(jctx->uci_ctx, &ptr, tuple, true);
        *section = ptr.s;
        return;
}

static void wireless_set_ssid(struct jsonapp_parse_ctx *jctx,
                              struct json_object *wlan_obj,
                              struct uci_section *section,
                              bool five_ghz)
{
        struct uci_ptr optr;
        struct json_object *member_obj;
        char ssid[64];

        memset(&optr, 0, sizeof(optr));
        optr.package = wireless_package->e.name;
        optr.section = section->e.name;
        optr.option = "ssid";
        member_obj = jsonapp_object_get_object_by_name(wlan_obj, "ssidName", json_type_string);
        sprintf(ssid, "%s%s", json_object_get_string(member_obj),
                five_ghz ? "5GHz" : "2_5GHz");
        optr.value = ssid;
        uci_set(jctx->uci_ctx, &optr);
}

static void wireless_create_new_iface_section(struct jsonapp_parse_ctx *jctx, 
                                              struct json_object *wlan_obj,
                                              bool five_ghz)
{
        struct json_object *obj;
        struct uci_section *s;
        char if_name[16];
        static int if_idx = 0;

        wireless_new_iface(jctx, wlan_obj, five_ghz, &s);

        jsonapp_set_new_option(jctx, wlan_obj, "device", 0,
                               wireless_package, s, "device", 
                               five_ghz ? "radio0" : "radio1");

        sprintf(if_name, "wlan%d", if_idx++);
        jsonapp_set_new_option(jctx, wlan_obj, NULL, 0,
                        wireless_package, s, "ifname", if_name);
        jsonapp_set_new_option(jctx, wlan_obj, NULL, 0,
                               wireless_package, s, "network", "lan");
        jsonapp_set_new_option(jctx, wlan_obj, NULL, 0,
                               wireless_package, s, "mode", "ap");

        wireless_set_ssid(jctx, wlan_obj, s, five_ghz);

        /* status is handled a bit differently */
        obj = jsonapp_object_get_object_by_name(wlan_obj, "status", json_type_string);
        char *disabled = json_object_get_string(obj) ? "0" : "1";
        jsonapp_set_new_option(jctx, wlan_obj, "status", 0, 
                               wireless_package, s, "disabled", disabled);
        //jsonapp_set_new_option(jctx, wlan_obj, "security", json_type_string,
        //                       wireless_package, s, "encryption", NULL);
        jsonapp_set_new_option(jctx, wlan_obj, "security", json_type_string,
                               wireless_package, s, "encryption", "none");
        jsonapp_set_new_option(jctx, wlan_obj, "passphrase", json_type_string, 
                               wireless_package, s, "key", NULL);
        uci_save(jctx->uci_ctx, wireless_package);
        return;
}

static void wireless_process_wlan_obj (struct jsonapp_parse_ctx *jctx,
                                       struct json_object *wlan_obj,
                                       void *user_data)
{
        const char *radio_str;
        struct json_object *obj;
        int create_radio0;
        int create_radio1;
        
        obj = jsonapp_object_get_object_by_name(wlan_obj, "radios", json_type_string);
        radio_str = json_object_get_string(obj);
        create_radio0 = strstr(radio_str, "5 GHz") ? 1 : 0;
        create_radio1 = strstr(radio_str, "2.5 GHz") ? 1 : 0;
        if (create_radio0)
                wireless_create_new_iface_section(jctx, wlan_obj, true);
        if (create_radio1)
                wireless_create_new_iface_section(jctx, wlan_obj, false);
        return;
}

static int wireless_process_json(struct jsonapp_parse_ctx *jctx, struct json_object *root)
{
        struct json_object *obj;
        obj = jsonapp_get_wlans(jsonapp_get_wlangrp(root));
        jsonapp_process_array(jctx, obj, NULL, wireless_process_wlan_obj);
        uci_commit(jctx->uci_ctx, &wireless_package, true);       
        return 0;
}

static struct jsonapp_parse_backend wlan_parse_backend = {
        .init = wireless_init_context,
        .process_json = wireless_process_json,
        .exit = wireless_exit_context,
};

static void __jsonapp_init__ wlan_engine_init(void)
{
        jsonapp_register_backend(&wlan_parse_backend);
}

