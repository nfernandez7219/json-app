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
        return;
}

static int jsonapp_process_json(struct jsonapp_parse_ctx *jctx)
{
        return 0;
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

        return backend->init(jctx);
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

