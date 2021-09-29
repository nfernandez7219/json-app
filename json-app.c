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

static void parse_wlan_group(json_object *wlan_grp, char *prefix)
{
        json_object *created_by;
        json_object *last_modified_by;
        json_object *wlan_grp_id;
        json_object *wlan_grp_name;
        json_object *wlan_grp_desc;
        json_object *location;
        json_object *placement;
        json_object *active;
        json_object *country;
        json_object *contact_person;
        json_object *contact_person_desc;
        json_object *create_date;
        json_object *last_modification_date;

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
                parse_wlan_group(obj, new_prefix);
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

