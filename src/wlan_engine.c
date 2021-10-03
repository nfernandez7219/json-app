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
        char tuple[64];

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
               
                fprintf(stderr, "deleting section: %s.%s\n", wireless_package->e.name, s->e.name);
                if (uci_delete(jctx->uci_ctx, &ptr) != UCI_OK) {
                        jsonapp_die("error deleting section: %s", s->e.name);
                }
        }

        return;
}

static struct jsonapp_parse_ctx *wlan_init_context(struct jsonapp_parse_ctx *jctx)
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

static struct jsonapp_parse_backend wlan_parse_backend = {
        .init = wlan_init_context,
        .jsonapp_process_json = NULL,
        .free_jsonapp_context = NULL,
};

static void __jsonapp_init__ wlan_engine_init(void)
{
        printf("wlan_engine\n");
        jsonapp_register_backend(&wlan_parse_backend);
}

