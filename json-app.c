#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <json-c/json.h>

/* allocates a new string from concatenation of str1 and str2
 */
char *str_new_append(char *str1, char *str2)
{
        char *str;
        int len1 = strlen(str1);
        int len2 = strlen(str2);
        str = calloc(len1+len2+1, sizeof(char));
        if (str) {
                strcpy(str, str1);
                strcat(str, str2);
        }
        return str;
}


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
        
static void parse_guess_access_list(struct json_object *guest_access_list)
{
        return;
}

static void parse_one_server(struct json_object *server, char *prefix)
{
        struct json_object *server_id;
        struct json_object *ip;
        struct json_object *secret;
        struct json_object *port;
        struct json_object *realm;

        server_id = json_object_object_get(server, "serverId");
        if (server_id)
                printf("%.serverId: %d\n", prefix, get_int_from_object(server));

        ip = json_object_object_get(server, "ip");
        if (ip)
                printf("%s.ip: %s\n", prefix, get_string_from_object(ip));

        secret = json_object_object_get(server, "secret");
        if (secret)
                printf("%s.secret: %s\n", prefix, get_string_from_object(secret));

        port = json_object_object_get(server, "port");
        if (port)
                printf("%s.port: %s\n", prefix, get_string_from_object(port));

        realm = json_object_object_get(server, "realm");
        if (realm)
                printf("%s.realm: %s\n", prefix, get_string_from_object(realm));

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
        struct json_object *created_by;
        struct json_object *last_modified_by;
        struct json_object *radius_server_id;
        struct json_object *wlan_id;
        struct json_object *servers;
        struct json_object *timeout;
        struct json_object *attempts;
        struct json_object *type;
        struct json_object *accounting_mode;
        struct json_object *accounting_status;
        struct json_object *intrim_update_time;
        struct json_object *create_date;
        struct json_object *last_modification_date;

        created_by = json_object_object_get(radius_server, "createdBy");
        if (created_by)
                printf("%s.createdBy: %s\n", prefix, get_string_from_object(created_by));

        last_modified_by = json_object_object_get(radius_server, "lastModifiedBy");
        if (last_modified_by)
                printf("%s.lastModifiedBy: %s\n", prefix, get_string_from_object(last_modified_by));

        radius_server_id = json_object_object_get(radius_server, "radiusServerId");
        if (radius_server_id)
                printf("%s.radiusServerId: %d\n", prefix, get_int_from_object(radius_server_id));

        wlan_id = json_object_object_get(radius_server, "wlanId");
        if (wlan_id)
                printf("%s.wlanId: %d\n", prefix, get_int_from_object(wlan_id));

        servers = json_object_object_get(radius_server, "servers");
        if (servers) {
                parse_servers(servers, prefix);
        }

        timeout = json_object_object_get(radius_server, "timeout");
        if (timeout)
                printf("%s.timeout: %d\n", prefix, get_int_from_object(timeout));

        attempts = json_object_object_get(radius_server, "attemps");
        if (attempts)
                printf("%s.attempts: %d\n", prefix, get_int_from_object(attempts));

        type = json_object_object_get(radius_server, "type");
        if (type)
                printf("%s.type: %s\n", prefix, get_string_from_object(type));

        accounting_mode = json_object_object_get(radius_server, "acctountingMode");
        if (accounting_mode)
                printf("%s.acctountingMode: %s\n", prefix, get_string_from_object(accounting_mode));

        accounting_status = json_object_object_get(radius_server, "acctountingStatus");
        if (accounting_status)
                printf("%s.acctountingStatus: %s\n", prefix, get_string_from_object(accounting_status));
        
        intrim_update_time = json_object_object_get(radius_server, "intrimUpdateTime");
        if (intrim_update_time)
                printf("%s.intrimUpdateTime: %d\n", prefix, get_int_from_object(intrim_update_time));

        create_date = json_object_object_get(radius_server, "createDate");
        if (create_date)
                printf("%s.createDate: %s\n", prefix, get_string_from_object(create_date));

        last_modification_date = json_object_object_get(radius_server, "lastModificationDate");
        if (last_modification_date)
                printf("%s.lastModificationDate: %s\n", prefix, get_string_from_object(last_modification_date));
        

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
        struct json_object *created_by;
        struct json_object *last_modified_by;
        struct json_object *wlan_id;
        struct json_object *wlan_name;
        struct json_object *ssid_name;
        struct json_object *status;
        struct json_object *vlan;
        struct json_object *security;
        struct json_object *passphrase;
        struct json_object *radios;
        struct json_object *create_date;
        struct json_object *last_modification_date;
        struct json_object *radius_server_list;
        struct json_object *guest_access_list;

        created_by = json_object_object_get(wlan, "createdBy");
        if (created_by)
                printf("%s.createdBy: %s\n", prefix, get_string_from_object(created_by));

        last_modified_by = json_object_object_get(wlan, "lastModifiedBy");
        if (last_modified_by)
                printf("%s.lastModifiedBy: %s\n", prefix, get_string_from_object(last_modified_by));

        wlan_id = json_object_object_get(wlan, "wlanId");
        if (wlan_id)
                printf("%s.wlanId: %d\n", prefix, get_int_from_object(wlan_id));

        wlan_name = json_object_object_get(wlan, "wlanName");
        if (wlan_name)
                printf("%s.wlanName: %s\n", prefix, get_string_from_object(wlan_name));

        ssid_name = json_object_object_get(wlan, "ssid_name");
        if (ssid_name)
                printf("%s.ssidName: %s\n", prefix, get_string_from_object(ssid_name));

        status = json_object_object_get(wlan, "status");
        if (status)
                printf("%s.status: %s\n", prefix, get_string_from_object(status));

        vlan = json_object_object_get(wlan, "vlan");
        if (vlan)
                printf("%s.vlan: %d\n", prefix, get_int_from_object(vlan));

        security = json_object_object_get(wlan, "security");
        if (security)
                printf("%s.security: %s\n", prefix, get_string_from_object(security));

        radios = json_object_object_get(wlan, "radios");
        if (radios)
                printf("%s.radios: %s\n", prefix, get_string_from_object(radios));
        
        create_date = json_object_object_get(wlan, "createDate");
        if (create_date)
                printf("%s.createDate: %s\n", prefix, get_string_from_object(create_date));

        last_modification_date = json_object_object_get(wlan, "lastModificationDate");
        if (last_modification_date)
                printf("%s.lastModificationDate: %s\n", prefix, get_string_from_object(last_modification_date));

        radius_server_list = json_object_object_get(wlan, "RadiusServerList");
        if (radius_server_list) {
                parse_radius_server_list(radius_server_list, prefix);
        }

        guest_access_list = json_object_object_get(wlan, "GuestAccessList");
        if (guest_access_list) {
                parse_guess_access_list(guest_access_list);
        }

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
        struct json_object *created_by;
        struct json_object *last_modified_by;
        struct json_object *wlan_grp_id;
        struct json_object *wlan_grp_name;
        struct json_object *wlan_grp_desc;
        struct json_object *location;
        struct json_object *placement;
        struct json_object *active;
        struct json_object *country;
        struct json_object *contact_person;
        struct json_object *contact_person_desc;
        struct json_object *wlans;
        struct json_object *create_date;
        struct json_object *last_modification_date;

        created_by = json_object_object_get(wlan_grp, "createdBy");
        if (created_by)
                printf("%s.createdBy: %s\n", prefix, get_string_from_object(created_by));

        last_modified_by = json_object_object_get(wlan_grp, "lastModifiedBy");
        if (last_modified_by)
                printf("%s.lastModifiedBy: %s\n", prefix, get_string_from_object(last_modified_by));
               
        wlan_grp_id = json_object_object_get(wlan_grp, "wlanGroupId");
        if (wlan_grp_id)
                printf("%s.wlanGroupId: %d\n", prefix, get_int_from_object(wlan_grp_id));

        wlan_grp_name = json_object_object_get(wlan_grp, "wlanGroupName");
        if (wlan_grp_name)
                printf("%s.wlanGroupName: %s\n", prefix, get_string_from_object(wlan_grp_name));

        wlan_grp_desc = json_object_object_get(wlan_grp, "wlanGroupDescription");
        if (wlan_grp_desc)
                printf("%s.wlanGroupDescription: %s\n", prefix, get_string_from_object(wlan_grp_desc));

        location = json_object_object_get(wlan_grp, "location");
        if (location)
                printf("%s.location: %s\n", prefix, get_string_from_object(location));

        placement = json_object_object_get(wlan_grp, "placement");
        if (placement)
                printf("%s.placement: %s\n", prefix, get_string_from_object(placement));

        active = json_object_object_get(wlan_grp, "status");
        if (active)
                printf("%s.active: %s\n", prefix, get_string_from_object(active));

        country = json_object_object_get(wlan_grp, "country");
        if (country)
                printf("%s.country: %s\n", prefix, get_string_from_object(country));

        contact_person = json_object_object_get(wlan_grp, "contactPerson");
        if (contact_person)
                printf("%s.contactPerson: %s\n", prefix, get_string_from_object(contact_person));

        contact_person_desc = json_object_object_get(wlan_grp, "contactPersonDescription");
        if (contact_person_desc)
                printf("%s.contactPersonDescription: %s\n", prefix, get_string_from_object(contact_person_desc));

        wlans = json_object_object_get(wlan_grp, "wlans");
        if (wlans) {
                parse_wlans(wlans, prefix);
        }

        create_date = json_object_object_get(wlan_grp, "createDate");
        if (create_date)
                printf("%s.createDate: %s\n", prefix, get_string_from_object(create_date));

        last_modification_date = json_object_object_get(wlan_grp, "lastModificationDate");
        if (last_modification_date)
                printf("%s.lastModificationDate: %s\n", prefix, get_string_from_object(last_modification_date));

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

