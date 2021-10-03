#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "json-app.h"

static struct jsonapp_parse_backend *backend_list;

void jsonapp_die(const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
        exit(-1);
        return;
}

void jsonapp_register_backend(struct jsonapp_parse_backend *backend)
{
        static struct jsonapp_parse_backend *tail = NULL;

        if (backend) {
                backend->next = NULL;
                if (tail) {
                        tail->next = backend;
                } else {
                        backend_list = backend;
                }
                tail = backend;
        }
        return;
}

static void free_jsonapp_context(struct jsonapp_parse_ctx *jctx)
{
        if (!jctx && jctx->backend->exit) {
                jctx->backend->exit(jctx);
        }

        uci_free_context(jctx->uci_ctx);

        /* we did not get any additional reference count for json root so don't 
         * do json_object_put(jctx->root) */

        free(jctx);
        jctx = NULL;
        return;
}

static int jsonapp_process_json(struct jsonapp_parse_ctx *jctx)
{
        if (jctx && jctx->backend->process_json) {
                jctx->backend->process_json(jctx);
                return 0;
        }
        jsonapp_die("cannot process json file");
        return -1;
}

int jsonapp_has_config(struct jsonapp_parse_ctx *jctx, char *name, 
                       void (*reset_uci)(struct jsonapp_parse_ctx *jctx),
                       bool create)
{
        char **configs = NULL;
        char **p = NULL;
        int found = 0;

        while (!found) {
                uci_list_configs(jctx->uci_ctx, &configs);
                if (configs) {
                        for (p = configs; *p; p++) {
                                if (strcmp(*p, name) == 0) {
                                        if (reset_uci)
                                                reset_uci(jctx);
                                        found = 1;
                                        break;
                                }
                        }
                }
                free(configs);

                if (!found && create) {
                        char new_config[256];
                        int fd;
                        const mode_t config_perm = S_IRUSR | S_IWUSR;
                        fprintf(stderr, "%s does not currently exist. creating it...\n", name);
                        snprintf(new_config, sizeof(new_config), "/etc/config/%s", name);
                        fd = openat(AT_FDCWD, new_config, O_WRONLY|O_CREAT|O_NONBLOCK, config_perm);
                        if (fd == -1) {
                                perror("error");
                                jsonapp_die("make sure jsonapp has correct permission to access config directory");
                        }
                        close(fd);
                        continue;
                }

                break;
        }

        return found;
}

static struct jsonapp_parse_ctx 
*alloc_jsonapp_context_from_file(struct jsonapp_parse_backend *backend,
                                 char *filename)
{
        struct jsonapp_parse_ctx *jctx;

        if (!(jctx = malloc(sizeof *jctx))) {
                jsonapp_die("insufficient memory for json parse context");
        }

        if (!(jctx->root = json_object_from_file(filename))) {
                jsonapp_die("cannot initialize json root object");
        }

        if (!(jctx->uci_ctx = uci_alloc_context())){
                jsonapp_die("insufficient memory for uci context");
        }

        jctx->backend = backend;
        return backend->init(jctx);
}

struct json_object *jsonapp_object_get_object_by_name(struct json_object *parent, char *name,
                                                      enum json_type expected_json_type)
{
        struct json_object *obj;
        obj = json_object_object_get(parent, name);
        if (json_object_get_type(obj) != expected_json_type) {
                jsonapp_die("%s formatting error.");
        }
        return obj;
}

void jsonapp_process_array(struct jsonapp_parse_ctx *jctx,
                           struct json_object *obj_arr,
                           void *user_data,
                           void (*process_member)(struct jsonapp_parse_ctx *jctx,
                                                  struct json_object *obj,
                                                  void *user_data))
{
        int i;
        int n;

        n = json_object_array_length(obj_arr);
        for (i=0; i<n; i++) {
                struct json_object *member_obj = json_object_array_get_idx(obj_arr, i);
                if (!process_member)
                        jsonapp_die("no processor for member defined");
                process_member(jctx, member_obj, user_data);
        }
        return;
}

void jsonapp_set_new_option(struct jsonapp_parse_ctx *jctx,
                            struct json_object *obj,
                            char *obj_member,
                            enum json_type expected_type,
                            struct uci_package *pkg, 
                            struct uci_section *section,
                            char *option, char *value)
{
        struct uci_ptr optr;
        struct json_object *member_obj;

        memset(&optr, 0, sizeof(optr));
        optr.package = pkg->e.name;
        optr.section = section->e.name;
        optr.option = option;
        if (value) {
                optr.value = value;
        } else {
                member_obj = jsonapp_object_get_object_by_name(obj, obj_member, expected_type);
                optr.value = json_object_get_string(member_obj);
        }
        uci_set(jctx->uci_ctx, &optr);
        return;
}

int main(int argc, char **argv)
{
        struct jsonapp_parse_backend *backend;
        struct jsonapp_parse_ctx *jctx;
        foreach_parse_backend(backend, backend_list) {
                jctx = alloc_jsonapp_context_from_file(backend, argv[1]);
                jsonapp_process_json(jctx);
                free_jsonapp_context(jctx);
        }
        return 0;
}

