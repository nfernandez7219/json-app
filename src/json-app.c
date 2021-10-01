#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <json-c/json.h>
#include <uci.h>
#include <unistd.h>
#include <sys/types.h>


/* notes regarding json-app
 *
 * 1) it processes input files in json format only.
 * 2) uci config file sections can be named or unamed:
 *
 *      -> this is a named section:
 *      config interface 'named_section'
 *              option ...
 *              option ...
 *
 *      -> this is an unnamed section:
 *      config interface
 *              option ...
 *              option ...              
 *
 *   the parser does not deal with unamed sections in the json file for now.
 *   therefore, a special option "name" must be inserted when defining 
 *   a section in a json input.
 *
 *   an example in hotspot.json is shown below. this only has 1 section.
 *
 *   {
 *           "network": {
 *                 "name": "wifi",
 *                 ....
 *           }
 *   }
 *
 *   this corresponds to the uci /etc/config/hotspot config file as:
 *
 *              config network 'wifi'
 *                      option ...
 *
 *
 *   another example is found in wireless.json where multiple sections are defined:
 *
 *   {
 *              "wifi-device":[
 *                      {
 *                              "name": "radio0",
 *                              ...
 *                      },
 *                      {
 *                              "name": "radio1",
 *                              ...
 *                      }
 *              ],
 *   }
 *
 *   this corresponds to the uci /etc/config/wireless config file as:
 *
 *              config wifi-device 'radio0'
 *                      option ...
 *
 *              config wifi-device 'radio1'
 *                      option ...
 *
 */

/* enabling this macro makes libuci save in ~/uci_test/ by default. if disabled
 * libuci will work with /etc/config/ instead. */
#define UCI_DEBUG       1

struct json_parse_ctx {
        json_object *root;
        struct uci_context *uci;
        struct uci_package *package;
        char package_name[]; /* this must always be last */
};

static void print_usage(void)
{
        printf("Usage: json-app [FILE]\n");
        printf("Process one FILE which is a JSON type file.\n");
        exit(-1);
        return;
}

static void get_package_name(char *path, char *package_name, int len)
{
        char *iter;
        int found = 0;
        char *package = basename(path);
        for (iter = package; *iter; iter++) {
                if (*iter == '.') {
                        found = 1;
                        break;
                }
        }

        if (found) *iter = '\0';
        strncpy(package_name, package, len);
        if (found) *iter = '.';
}

/* traverse all the sections in the package and delete them. */
static void reset_uci_config(struct json_parse_ctx *jctx)
{
        struct uci_element *e;
        struct uci_element *tmp;
        struct uci_package *p;
        struct uci_ptr ptr;
        char tuple[64];

        printf("reset uci_conf\n");
        if (uci_load(jctx->uci, jctx->package_name, &jctx->package) != UCI_OK) {
                fprintf(stderr, "error opening uci config file\n");
                exit(-1);
        }

        p = jctx->package;
        uci_foreach_element_safe(&p->sections, tmp, e) {
                struct uci_section *s = uci_to_section(e);
                sprintf(tuple, "%s.%s", p->e.name, s->e.name);
                printf("deleting section: %s\n", tuple);
                if (uci_lookup_ptr(jctx->uci, &ptr, tuple, true) != UCI_OK) {
                        fprintf(stderr, "error looking up section\n");
                        exit(1);
                }
                if (uci_delete(jctx->uci, &ptr) != UCI_OK) {
                        fprintf(stderr, "error in delete\n");
                        exit(-1);
                }
        }

#if 0 // only for debugging 
        if (uci_commit(jctx->uci, &p, false) != UCI_OK) {
                fprintf(stderr, "error in commit\n");
                exit(-1);
        }
#endif 

        return;
}

/* the package we will use is taken from the filename.
 * e.g. if we are parsing for a wireless uci config, the corresponding filename
 * will be "wireless.json". if we are parsing for a hotspot uci config, it will
 * be "hotspot.json".
 *
 * we will be using the filename basename mwith the .json extension removed as
 * the config file in /etc/config/. examples are below:
 *
 * /some/path/wireless.json => /etc/config/wireless
 * /some/path/hotspot.json => /etc/config/hotspot
 */

static struct json_parse_ctx *new_json_parse_context(int argc, char **argv)
{
        char *filename;
        int size;
        struct json_parse_ctx *new_ctx;
        char package_name[256];

        if (argc != 2) {
                print_usage();
        }
        filename = argv[1];
        get_package_name(filename, package_name, sizeof(package_name));
        size = (strlen(package_name) + 1) + sizeof(struct json_parse_ctx);
        new_ctx = malloc(size);
        if (!new_ctx) {
                fprintf(stderr, "out of memory error\n");
                exit(-1);
        }

        new_ctx->root = json_object_from_file(filename);
        if (!new_ctx->root) {
                fprintf(stderr, "cannot process input file %s\n", filename);
                exit(-1);
        }

        /* our package name */
        /* this can also be accessed via package->e.name */
        strcpy(new_ctx->package_name, package_name);

        new_ctx->uci = uci_alloc_context();
        if (!new_ctx->uci) {
                fprintf(stderr, "error initializing uci context\n");
                exit(-1);
        }

#ifdef UCI_DEBUG
        char uci_conf_dir[128];
        const char *homedir = getenv("HOME");
        sprintf(uci_conf_dir, "%s/uci_test", homedir);
        uci_set_confdir(new_ctx->uci, uci_conf_dir);
#else
        char uci_conf_dir[128];
        sprintf(uci_conf_dir, "%s", new_ctx->uci->confdir);
#endif

        char **configs = NULL;
        char **p;

        uci_list_configs(new_ctx->uci, &configs);
        for (p = configs; *p; p++) {
                if (strcmp(*p, package_name)==0) {

                        /* truncate the config file in /etc/config prior to updating. */
                        reset_uci_config(new_ctx);

                        break;
                } 
        }
        if (!(*p)) {
                /* libuci can only udpate existing configs.
                 * there must be an existing config file package before we can 
                 * use libuci. 
                 *
                 * there is no option or command in libuci to create a 
                 * non-existing config file.
                 * */
                fprintf(stderr, "config file non-existent. libuci cannot work with non-existent config files\n");
                exit(-1);
        }

        /* now we are ready to write a new config file thru libuci ... */

        free(configs);
        return new_ctx;
}

static void free_json_parse_context(struct json_parse_ctx *ctx)
{
        uci_free_context(ctx->uci);
        free(ctx);
        return;
}

static void parse_option_list(struct json_object *obj)
{
        fprintf(stderr, "array not supported for now.\n");
        return;
}

/* sections must have a name */
static void parse_one_section(struct json_parse_ctx *ctx, char *type_name,
                              struct json_object *obj)
{
        struct json_object *name;
        struct uci_ptr sptr;
        struct uci_ptr optr;
        char string_val[256];

        memset(&sptr, 0, sizeof(sptr));

        /* a section can be named or nameless, lets process that here. */
        name = json_object_object_get(obj, "name");
        if (name) {
                printf("config %s '%s'", type_name, json_object_get_string(name));
                
                sptr.package = ctx->package->e.name;
                sptr.value = type_name; /* type */
                sptr.section = json_object_get_string(name); /* name */
                uci_set(ctx->uci, &sptr);
        } else {
                /* for now, we don't really process a "nameless" section. */
                fprintf(stderr, "nameless section detected in json file.\n");
                exit(-1);
        } 
        printf("\n");

        json_object_object_foreach(obj, key, val) {
                struct json_object *option;

                /* skip name because we processed it already */
                if (strcmp(key, "name") == 0) {
                        continue;
                }

                memset(&optr, 0, sizeof(optr));
                
                /* according to uci standard, we can have a string, boolean, int
                 * or a list as option. we figure it out here so we now what 
                 * processing to do.
                 */
                switch(json_object_get_type(val))
                {
                case json_type_string:
                        printf("\toption %s '%s'", key, json_object_get_string(val));
                        optr.package = ctx->package->e.name;
                        optr.section = json_object_get_string(name);
                        optr.option = key;
                        sprintf(string_val, "%s", json_object_get_string(val));
                        optr.value = string_val;
                        break;
                case json_type_int:
                        printf("\toption %s %d", key, json_object_get_int(val));
                        optr.package = ctx->package->e.name;
                        optr.section = json_object_get_string(name);
                        optr.option = key;
                        sprintf(string_val, "%d", json_object_get_int(val));
                        optr.value = string_val;
                        break;
                case json_type_array:
                        parse_option_list(val);
                }
                uci_set(ctx->uci, &optr);
                printf("\n");
        }
        printf("\n");

        return;
}

static void parse_section(struct json_parse_ctx *ctx, char *key, struct json_object *section)
{
        int n;
        int i;
        struct json_object *obj;

        /* we can have an array or an object here */
        if (json_object_get_type(section) == json_type_array) {
              n = json_object_array_length(section);  
              for (i=0; i<n; i++) {
                        obj = json_object_array_get_idx(section, i);
                        parse_one_section(ctx, key, obj);
              }
        } else {
                parse_one_section(ctx, key, section);
        }

        return;
}

static void parse_root(struct json_parse_ctx *ctx)
{
        json_object_object_foreach(ctx->root, key, val) {
                parse_section(ctx, key, val);
        }
        return;
}

static void parse_commit(struct json_parse_ctx *ctx)
{
        if (uci_commit(ctx->uci, &ctx->package, false) != UCI_OK) {
                fprintf(stderr, "error in commit\n");
                exit(-1);
        }
        return;
}


int main(int argc, char **argv)
{
        struct json_parse_ctx *parse_ctx;

        parse_ctx = new_json_parse_context(argc, argv);
        parse_root(parse_ctx);
        parse_commit(parse_ctx);
        free_json_parse_context(parse_ctx);
        return 0;
}

