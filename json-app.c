#include <stdio.h>
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

int main(int argc, char **argv)
{
        json_object *root;

        if (argc != 2)  {
                print_usage();
        }

        root = read_cfg(argv[1]);
        

        json_object_put(root);

        return 0;
}

