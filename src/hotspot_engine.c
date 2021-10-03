#include <stdio.h>
#include "json-app.h"
#include <unistd.h>
#include <sys/types.h>

/* truncate the entire /etc/config/hotspot file */
static void reset_hotspot_uci(struct jsonapp_parse_ctx *jctx)
{
        if (truncate("/etc/config/hotspot", 0) == -1) {
                jsonapp_die("unable to reset /etc/config/hotspot");
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

static struct jsonapp_parse_backend hotspot_parse_backend = {
        .init = hotspot_init_context,
        .process_json = NULL,
        .free_jsonapp_context = NULL,
};

static void __jsonapp_init__ hotspot_engine_init(void)
{
        printf("hotspot_engine\n");
        jsonapp_register_backend(&hotspot_parse_backend);
}

