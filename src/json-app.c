#include <mosquitto.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <unistd.h>
#include <dirent.h>
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

static void read_iface_mac(char *filepath, uint8_t *mac)
{
        char address_path[128];
        char mac_str[32];
        FILE *f;
        char *eptr;
        char *sptr;
        int i;

        sprintf(address_path, "%s/address", filepath);
        if (!(f = fopen(address_path, "r"))) {
                jsonapp_die("sysdir error");
        }
        fread(mac_str, 1, sizeof mac_str, f);
        sptr = mac_str;
        for (i=0; i<6; i++) {
                mac[i] = strtol(sptr, &eptr, 16) & 0xff;
                sptr = eptr + 1;
        }
        fclose(f);
        return;
}

static int jsonapp_get_mac(char *iface, uint8_t *mac)
{
        DIR *net_sysdir;
        struct dirent *file_entry;
        static const char sysdir[] = "/sys/class/net/";
        int not_found = -1;
        char filepath[64];
        
        /* search for iface in /sys/class/net/ if it exists. */
        net_sysdir = opendir(sysdir);
        if (!net_sysdir) {
                /* this is considered a gross error as every linux system
                 * should have /sys/ directory at least */
                jsonapp_die("cannot open /sys/class/net/ directory");
        }

        while ((file_entry = readdir(net_sysdir))) {
                if (strcmp(iface, file_entry->d_name) == 0) {
                        sprintf(filepath, "%s%s", sysdir, iface);
                        read_iface_mac(filepath, mac);
                        not_found = 0;
                        break;
                }
        }

        closedir(net_sysdir);
        return not_found;
}

static void jsonapp_get_topic(struct jsonapp_mqtt_ctx *mctx, char *topic, int len)
{
        uint8_t *ptr = mctx->mac_address;
        snprintf(topic, len, "adopt/device/%.2x%.2x%.2x%.2x%.2x%.2x",
                 ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
        return;
}

static void jsonapp_generate_new_client_id(char *client_id, int len)
{
        pid_t pid = getpid();
        snprintf(client_id, len, "jsonapp%u",pid);
        return;
}

static void jsonapp_mqtt_subscribe_cb(struct mosquitto *mosq,
                                      void *arg, int msg_id, int qos_count,
                                      const int *granted_qos)
{
        struct jsonapp_mqtt_ctx *mctx = arg;
        char mqtt_topic[256];
        jsonapp_get_topic(mctx, mqtt_topic, sizeof mqtt_topic);
        printf("subscribed to topic: %s\n", mqtt_topic);
        return;
}

static void jsonapp_mqtt_connect_cb(struct mosquitto *mosq, void *arg, int rc)
{
        if (rc != 0) {
                jsonapp_die("failed to connect to mqtt server");
        }
        printf("connected to mqtt server!\n");

        return;
}

static void jsonapp_mqtt_msg_cb(struct mosquitto *mosq, void *arg,
                                const struct mosquitto_message *msg)
{
        printf("message: %s\n", (char *)msg->payload);
        return;
}

int main(int argc, char **argv)
{
        struct jsonapp_parse_backend *backend;
        struct jsonapp_parse_ctx *jctx;
        int option;
        struct jsonapp_mqtt_ctx mqtt_ctx = {0};
        char mqtt_topic[256];

        while((option = getopt(argc, argv, "n:")) != -1) {
                switch(option) {
                case 'n':
                        mqtt_ctx.iface_name = optarg;
                        break;
                case '?':
                        if (optopt == 'n') {
                                jsonapp_die("-n expects a network interface name.");
                        } else {
                                jsonapp_die("encountered illegal option");
                        }
                }
        }

        if (!mqtt_ctx.iface_name) {
                jsonapp_die("please give the net interface name to get the MAC address");
        }

        if (jsonapp_get_mac(mqtt_ctx.iface_name, mqtt_ctx.mac_address)) {
                jsonapp_die("unable to get MAC address of %s\n", mqtt_ctx.iface_name);
        }

        mosquitto_lib_init();
        jsonapp_generate_new_client_id(mqtt_ctx.jsonapp_client_id, sizeof(mqtt_ctx.jsonapp_client_id));
        mqtt_ctx.mosq = mosquitto_new(mqtt_ctx.jsonapp_client_id, true, &mqtt_ctx);
        mosquitto_username_pw_set(mqtt_ctx.mosq, "guest", "guest");
        mosquitto_connect_callback_set(mqtt_ctx.mosq, jsonapp_mqtt_connect_cb);
        mosquitto_subscribe_callback_set(mqtt_ctx.mosq, jsonapp_mqtt_subscribe_cb);
        mosquitto_message_callback_set(mqtt_ctx.mosq, jsonapp_mqtt_msg_cb);
        int err = mosquitto_connect(mqtt_ctx.mosq, "localhost", 1883, 60);
        if (err != MOSQ_ERR_SUCCESS) {
                fprintf(stderr, "error connecting to mqtt server!");
        }
        jsonapp_get_topic(&mqtt_ctx, mqtt_topic, sizeof mqtt_topic);
        mosquitto_subscribe(mqtt_ctx.mosq, NULL, mqtt_topic, 0);
        mosquitto_loop_forever(mqtt_ctx.mosq, -1,1);
        mosquitto_lib_cleanup();
        return 0;
        foreach_parse_backend(backend, backend_list) {
                jctx = alloc_jsonapp_context_from_file(backend, argv[1]);
                jsonapp_process_json(jctx);
                free_jsonapp_context(jctx);
        }
        return 0;
}

