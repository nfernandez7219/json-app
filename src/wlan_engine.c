#include <stdio.h>
#include "json-app.h"


/* just remove all wifi-iface sections */
static void reset_wireless_uci(struct jsonapp_parse_ctx *jctx)
{
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

