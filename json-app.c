#include <stdio.h>
#include <json-c/json.h>

static void print_usage(void)
{
        printf("Usage: json-app [FILE]\n");
        printf("Process one FILE which is a JSON type file.\n");
        exit(-1);
        return;
}

int main(int argc, char **argv)
{
        FILE *fd;

        if (argc != 2)  {
                print_usage();
        }

        fd = fopen(argv[1], "r");
        if (!fd) {
                fprintf(stderr, "error opening %s\n", argv[1]);
                exit(-1);
        }
        

        fclose(fd);

        return 0;
}

