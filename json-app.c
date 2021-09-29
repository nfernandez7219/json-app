#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <json-c/json.h>

static void print_usage(void)
{
        printf("Usage: json-app [FILE]\n");
        printf("Process one FILE which is a JSON type file.\n");
        exit(-1);
        return;
}

static json_object *read_cfg(char *filepath)
{
        return json_object_from_file(filepath);
}

const char *get_string_from_object(struct json_object *obj)
{
        char *str = NULL;
        if (json_object_get_type(obj) == json_type_string) {
                return json_object_get_string(obj);
        }
        return str;
}

int get_int_from_object(struct json_object *obj)
{
        int ret = -1;
        if (json_object_get_type(obj) == json_type_int) {
                return json_object_get_int(obj);
        }
        return ret;
}

static void process_string_value(struct json_object *parent, char *prefix, char *attribute)
{
        struct json_object *obj;
        obj = json_object_object_get(parent, attribute);
        if (obj)
                printf("%s.%s: %s\n", prefix, attribute, get_string_from_object(obj));
        return;
}

static void process_int_value(struct json_object *parent, char *prefix, char *attribute)
{
        struct json_object *obj;
        obj = json_object_object_get(parent, attribute);
        if (obj)
                printf("%s.%s: %d\n", prefix, attribute, get_int_from_object(obj));
        return;
}

static void parse_one_whitelist_url(struct json_object *whitelist_url, char *prefix)
{
        process_string_value(whitelist_url, prefix, "createdBy");
        process_string_value(whitelist_url, prefix, "lastModifiedBy");
        process_int_value(whitelist_url, prefix, "whitelistId");
        process_string_value(whitelist_url, prefix, "whitelistUrl");
        process_string_value(whitelist_url, prefix, "createDate");
        process_string_value(whitelist_url, prefix, "lastModificationDate");
        return;
}

static void parse_whitelist_urls(struct json_object *whitelist_urls, char *prefix)
{
        int n;
        int i;
        struct json_object *obj;
        char new_prefix[256];
        
        /* guest access list list must be an array */
        if (json_object_get_type(whitelist_urls) != json_type_array) {
                fprintf(stderr, "whitelist Urls list wrong format!\n");
                return;
        }

        n = json_object_array_length(whitelist_urls);
        for (i = 0; i < n; i++) {
                obj = json_object_array_get_idx(whitelist_urls, i);
                sprintf(new_prefix, "%s.whitelistUrl[%d]", prefix, i);
                parse_one_whitelist_url(obj, new_prefix);
        }
        return;
        return;
}

static void  parse_one_guest_access(struct json_object *guest_access, char *prefix)
{
        struct json_object *whitelist_urls;

        process_string_value(guest_access, prefix, "createdBy");
        process_string_value(guest_access, prefix, "lastModifiedBy");
        process_int_value(guest_access, prefix, "guestAccessId");
        process_int_value(guest_access, prefix, "wlanId");
        process_string_value(guest_access, prefix, "status");
        process_string_value(guest_access, prefix, "portalMode");
        process_string_value(guest_access, prefix, "portalType");
        process_string_value(guest_access, prefix, "portalUrl");
        process_string_value(guest_access, prefix, "successAction");
        process_string_value(guest_access, prefix, "successRedirectUrl");
        whitelist_urls = json_object_object_get(guest_access, "whitelistUrls");
        if (whitelist_urls)
                parse_whitelist_urls(whitelist_urls, prefix);
        return;
}

static void parse_guest_access_list(struct json_object *guest_access_list, char *prefix)
{
        int n;
        int i;
        struct json_object *obj;
        char new_prefix[256];
        
        /* guest access list list must be an array */
        if (json_object_get_type(guest_access_list) != json_type_array) {
                fprintf(stderr, "guest access list wrong format!\n");
                return;
        }

        n = json_object_array_length(guest_access_list);
        for (i = 0; i < n; i++) {
                obj = json_object_array_get_idx(guest_access_list, i);
                sprintf(new_prefix, "%s.GuestAccess[%d]", prefix, i);
                parse_one_guest_access(obj, new_prefix);
        }
        return;
}

static void parse_one_server(struct json_object *server, char *prefix)
{
        process_int_value(server, prefix, "serverId");
        process_string_value(server, prefix, "ip");
        process_string_value(server, prefix, "secret");
        process_string_value(server, prefix, "port");
        process_string_value(server, prefix, "realm");
        return;
}

static void parse_servers(struct json_object *servers, char *prefix)
{
        int n;
        int i;
        struct json_object *obj;
        char new_prefix[256];
        
        /* server list must be an array */
        if (json_object_get_type(servers) != json_type_array) {
                fprintf(stderr, "server list wrong format!\n");
                return;
        }

        n = json_object_array_length(servers);
        for (i = 0; i < n; i++) {
                obj = json_object_array_get_idx(servers, i);
                sprintf(new_prefix, "%s.server[%d]", prefix, i);
                parse_one_server(obj, new_prefix);
        }
        return;
}

static void parse_one_radius(struct json_object *radius_server, char *prefix)
{
        struct json_object *servers;

        process_string_value(radius_server, prefix, "createdBy");
        process_string_value(radius_server, prefix, "lastModifiedBy");
        process_int_value(radius_server, prefix, "radiusServerId");
        process_int_value(radius_server, prefix, "wlanId");
        servers = json_object_object_get(radius_server, "servers");
        if (servers) {
                parse_servers(servers, prefix);
        }
        process_int_value(radius_server, prefix, "timeout");
        process_int_value(radius_server, prefix, "attempts");
        process_string_value(radius_server, prefix, "type");
        process_string_value(radius_server, prefix, "acctountingMode");
        process_string_value(radius_server, prefix, "acctountingStatus");
        process_int_value(radius_server, prefix, "intrimUpdateTime");
        process_string_value(radius_server, prefix, "createDate");
        process_string_value(radius_server, prefix, "lastModificationDate");
        return;
}

static void parse_radius_server_list(struct json_object *radius_server_list,
                                     char *prefix)
{
        int n;
        int i;
        struct json_object *obj;
        char new_prefix[256];
        
        /* radius server list must be an array */
        if (json_object_get_type(radius_server_list) != json_type_array) {
                fprintf(stderr, "radius server list wrong format!\n");
                return;
        }

        n = json_object_array_length(radius_server_list);
        for (i = 0; i < n; i++) {
                obj = json_object_array_get_idx(radius_server_list, i);
                sprintf(new_prefix, "%s.RadiusServer[%d]", prefix, i);
                parse_one_radius(obj, new_prefix);
        }
        return;
}

static void parse_one_wlan(struct json_object *wlan, char *prefix)
{
        struct json_object *radius_server_list;
        struct json_object *guest_access_list;

        process_string_value(wlan, prefix, "createdBy");
        process_string_value(wlan, prefix, "lastModifiedBy");
        process_int_value(wlan ,prefix, "wlandId");
        process_string_value(wlan, prefix, "wlanName");
        process_string_value(wlan, prefix, "ssid_name");
        process_string_value(wlan, prefix, "status");
        process_int_value(wlan, prefix, "vlan");
        process_string_value(wlan, prefix, "security");
        process_string_value(wlan, prefix, "radios");
        process_string_value(wlan, prefix, "createDate");
        process_string_value(wlan, prefix, "lastModificationDate");
        
        radius_server_list = json_object_object_get(wlan, "RadiusServerList");
        if (radius_server_list) 
                parse_radius_server_list(radius_server_list, prefix);
        
        guest_access_list = json_object_object_get(wlan, "GuestAccessList");
        if (guest_access_list)
                parse_guest_access_list(guest_access_list, prefix);
        return;
}

static void parse_wlans(struct json_object *wlans, char *prefix)
{
        int n;
        int i;
        struct json_object *obj;
        char new_prefix[256];
        
        /* wlan group list must be an array */
        if (json_object_get_type(wlans) != json_type_array) {
                fprintf(stderr, "wlan list wrong format!\n");
                return;
        }

        n = json_object_array_length(wlans);
        for (i = 0; i < n; i++) {
                obj = json_object_array_get_idx(wlans, i);
                sprintf(new_prefix, "%s.wlan[%d]", prefix, i);
                parse_one_wlan(obj, new_prefix);
        }
        return;
}

static void parse_one_wlan_group(json_object *wlan_grp, char *prefix)
{
        struct json_object *wlans;

        process_string_value(wlan_grp, prefix, "createdBy");
        process_string_value(wlan_grp, prefix, "lastModifiedBy");
        process_int_value(wlan_grp, prefix, "wlanGroupId");
        process_string_value(wlan_grp, prefix, "wlanGroupName");
        process_string_value(wlan_grp, prefix, "wlanGroupDescriptor");
        process_string_value(wlan_grp, prefix, "location");
        process_string_value(wlan_grp, prefix, "placement");
        process_string_value(wlan_grp, prefix, "status");
        process_string_value(wlan_grp, prefix, "country");
        process_string_value(wlan_grp, prefix, "contactPerson");
        process_string_value(wlan_grp, prefix, "contactpersonDescription");
        
        wlans = json_object_object_get(wlan_grp, "wlans");
        if (wlans)
                parse_wlans(wlans, prefix);

        process_string_value(wlan_grp, prefix, "createDate");
        process_string_value(wlan_grp, prefix, "lastModificationDate");
        return;
}


static void parse_wlan_group_list(json_object *wlan_group_list, char *prefix)
{
        int n;
        int i;
        json_object *obj;
        char new_prefix[256];
        
        /* wlan group list must be an array */
        if (json_object_get_type(wlan_group_list) != json_type_array) {
                fprintf(stderr, "wlan group list wrong format!\n");
                return;
        }

        n = json_object_array_length(wlan_group_list);
        for (i = 0; i < n; i++) {
                obj = json_object_array_get_idx(wlan_group_list, i);
                sprintf(new_prefix, "%s.wlan_group[%d]", prefix, i);
                parse_one_wlan_group(obj, new_prefix);
        }

        return;
}

static void parse_root(json_object *root)
{
        json_object *wlan_group_list;
        json_object *time_stamp;
        json_object *status;

        wlan_group_list = json_object_object_get(root, "WlanGroupList");
        if (wlan_group_list) {
                parse_wlan_group_list(wlan_group_list, "root");
        }

        time_stamp = json_object_object_get(root, "timestamp");
        if (time_stamp) {
                /* process timestamp in root here */
        }

        status = json_object_object_get(root, "status");
        if (status) {
                /* process status in root here */
        }

        return;
}

int main(int argc, char **argv)
{
        json_object *root;

        if (argc != 2)  {
                print_usage();
        }

        root = read_cfg(argv[1]);
        parse_root(root);
        json_object_put(root);

        return 0;
}

