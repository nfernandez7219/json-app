#include <mosquitto.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
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

static void jsonapp_exit_backend(struct jsonapp_parse_backend *backend)
{
        struct jsonapp_parse_ctx *jctx = backend->jctx;
        backend->exit(jctx);
        return;
}

static int jsonapp_process_json(struct jsonapp_parse_backend *backend, char *json_msg)
{
        struct jsonapp_parse_ctx *jctx = backend->jctx;
        struct json_object *root = json_tokener_parse(json_msg);
        backend->process_json(jctx, root);
        return 0;
}

static int jsonapp_init_backend(struct jsonapp_parse_ctx *jctx, 
                                struct jsonapp_parse_backend *backend)
{
        backend->jctx = jctx;
        backend->init(jctx);
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

static void jsonapp_read_iface_mac(char *filepath, uint8_t *mac)
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
                        jsonapp_read_iface_mac(filepath, mac);
                        not_found = 0;
                        break;
                }
        }

        closedir(net_sysdir);
        return not_found;
}

static void jsonapp_generate_new_client_id(char *client_id, int len)
{
        pid_t pid = getpid();
        snprintf(client_id, len, "jsonapp%u",pid);
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

static void jsonapp_get_topic(struct jsonapp_mqtt_ctx *mctx, char *topic, int len)
{
        uint8_t *ptr = mctx->mac_address;
        snprintf(topic, len, "adopt/device/%.2x%.2x%.2x%.2x%.2x%.2x",
                 ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
        return;
}

static void jsonapp_mqtt_subscribe_cb(struct mosquitto *mosq,
                                      void *arg, int msg_id, int qos_count,
                                      const int *granted_qos)
{
        struct jsonapp_parse_ctx *jctx = arg;
        struct jsonapp_mqtt_ctx *mctx = &jctx->mqtt;
        char mqtt_topic[256];
        jsonapp_get_topic(mctx, mqtt_topic, sizeof mqtt_topic);
        printf("subscribed to topic: %s\n", mqtt_topic);
        return;
}

static void jsonapp_mqtt_msg_cb(struct mosquitto *mosq, void *arg,
                                const struct mosquitto_message *msg)
{
        struct jsonapp_parse_ctx *jctx = arg;
        struct jsonapp_parse_backend *backend;
        char *json_message = (char *)msg->payload;
        foreach_parse_backend(backend, backend_list) {
                jsonapp_init_backend(jctx, backend);
                jsonapp_process_json(backend, json_message);
                jsonapp_exit_backend(backend);
        }
        return;
}

static void jsonapp_print_mqtt_settings(struct jsonapp_mqtt_ctx *mqtt)
{
        fprintf(stderr, "network interface: %s\n", mqtt->iface_name);
        fprintf(stderr, "mqtt username: %s\n", mqtt->user);
        fprintf(stderr, "mqtt pssword: %s\n", mqtt->password);
        fprintf(stderr, "mqtt server host/ip address: %s\n", mqtt->host);
        return;
}

static void jsonapp_init_mqtt(struct jsonapp_parse_ctx *jctx)
{
        struct jsonapp_mqtt_ctx *mqtt;
        char mqtt_topic[256];

        mqtt = &jctx->mqtt;
        if (!mqtt->iface_name) {
                jsonapp_die("please give the net interface name to get the MAC address");
        }

        jsonapp_print_mqtt_settings(mqtt);

        if (jsonapp_get_mac(mqtt->iface_name, mqtt->mac_address)) {
                jsonapp_die("unable to get MAC address of %s\n", mqtt->iface_name);
        }

        mosquitto_lib_init();
        jsonapp_generate_new_client_id(mqtt->jsonapp_client_id, sizeof(mqtt->jsonapp_client_id));
        mqtt->mosq = mosquitto_new(mqtt->jsonapp_client_id, true, jctx);
        mosquitto_username_pw_set(mqtt->mosq, mqtt->user, mqtt->password);
        mosquitto_connect_callback_set(mqtt->mosq, jsonapp_mqtt_connect_cb);
        mosquitto_subscribe_callback_set(mqtt->mosq, jsonapp_mqtt_subscribe_cb);
        mosquitto_message_callback_set(mqtt->mosq, jsonapp_mqtt_msg_cb);
        int err = mosquitto_connect(mqtt->mosq, mqtt->host, 1883, 60);
        if (err != MOSQ_ERR_SUCCESS) {
                fprintf(stderr, "error connecting to mqtt server!");
        }
        jsonapp_get_topic(mqtt, mqtt_topic, sizeof mqtt_topic);
        mosquitto_subscribe(mqtt->mosq, NULL, mqtt_topic, 0);

        return;
}

static void jsonapp_init_mqtt_defaults(struct jsonapp_mqtt_ctx *mqtt)
{
        mqtt->user = "guest";
        mqtt->password = "guest";
        mqtt->host = "localhost";
        return;
}

static void jsonapp_exit_mqtt(struct jsonapp_parse_ctx *jctx)
{
        mosquitto_destroy(jctx->mqtt.mosq);
        mosquitto_lib_cleanup();
        return;
}

static struct jsonapp_parse_ctx 
*jsonapp_alloc_context(int argc, char **argv)
{
        static struct jsonapp_parse_ctx *jctx = NULL;
        struct jsonapp_mqtt_ctx *mqtt;
        int option;

        if (jctx)
                return jctx;

        if (!(jctx = malloc(sizeof *jctx))) {
                jsonapp_die("insufficient memory for json parse context");
        }
        memset(jctx, 0, sizeof *jctx);
        mqtt = &jctx->mqtt;
        jsonapp_init_mqtt_defaults(mqtt);
        while((option = getopt(argc, argv, "n:u:p:h:")) != -1) {
                switch(option) {
                case 'n': mqtt->iface_name = optarg; break;
                case 'u': mqtt->user = optarg; break;
                case 'p': mqtt->password = optarg; break;
                case 'h': mqtt->host = optarg; break;
                case '?':
                        if (optopt == 'n') {
                                jsonapp_die("-n expects a network interface name.");
                        } else if (optopt == 'u') {
                                fprintf(stderr, "-u expects a username. if not used, \"guest\" is used.");
                        } else if (optopt == 'p') {
                                fprintf(stderr, "-p expects a password. if not used, \"guest\" is used.");
                        } else if (optopt == 'h') {
                                fprintf(stderr, "-h expects a hotname or IP address. if not used, \"localhost\" is used.");
                        } else {
                                jsonapp_die("encountered illegal option");
                        }
                }
        }

        jsonapp_init_mqtt(jctx);
        if (!(jctx->uci_ctx = uci_alloc_context())){
                jsonapp_die("insufficient memory for uci context");
        }
        return jctx;
}

static void jsonapp_free_context(struct jsonapp_parse_ctx *jctx)
{
        uci_free_context(jctx->uci_ctx);
        jsonapp_exit_mqtt(jctx);
        free(jctx);
        return;
}

struct json_object *jsonapp_object_get_object_by_name(struct json_object *parent, char *name,
                                                      enum json_type expected_json_type)
{
        struct json_object *obj;
        obj = json_object_object_get(parent, name);
        if (json_object_get_type(obj) != expected_json_type) {
                jsonapp_die("%s formatting error.", name);
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

/* potentially non-async-signal safe procedure since functions called here 
 * _might_ use function like printf(), fprintf(), etc */
struct jsonapp_parse_ctx *jctx = NULL;
static void signal_handler(int sig)
{
        char topic[256];
        if (jctx) {
                jsonapp_get_topic(&jctx->mqtt, topic, sizeof topic);
                mosquitto_unsubscribe(jctx->mqtt.mosq, NULL, topic);
                mosquitto_disconnect(jctx->mqtt.mosq);
        }
        exit(-1);
        return;
}

struct json_object *jsonapp_get_wlangrp(struct json_object *root)
{
        return jsonapp_object_get_object_by_name(root, "WlanGroup", json_type_object);
}

struct json_object *jsonapp_get_wlans(struct json_object *wlangrp)
{
         return jsonapp_object_get_object_by_name(wlangrp, "wlans", json_type_array);       
}

struct json_object *jsonapp_get_radius_servers(struct json_object *wlans)
{
        return jsonapp_object_get_object_by_name(wlans, "radiusServerList", json_type_array);
}

struct json_object *jsonapp_get_servers(struct json_object *radius_server)
{
        return jsonapp_object_get_object_by_name(radius_server, "servers", json_type_array);
}

struct json_object *jsonapp_get_guest_acl_list(struct json_object *wlans)
{
        return jsonapp_object_get_object_by_name(wlans, "guestAccessList", json_type_array);
}

int main(int argc, char **argv)
{
        jctx = jsonapp_alloc_context(argc, argv);

        if (signal(SIGINT, signal_handler) == SIG_ERR) {
                jsonapp_die("jsonapp error trapping SIGINT");
        }
        mosquitto_loop_forever(jctx->mqtt.mosq, -1, 1);
        jsonapp_free_context(jctx);
        return 0;
}

