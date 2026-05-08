#include <stdio.h>
#include <string.h>

#include "tests.h"

static void usage(
    const char *prog
){
    fprintf(
        stderr,
        "usage:\n"
        "  %s create\n"
        "  %s parse\n"
        "  %s view <file>\n",
        prog,
        prog,
        prog
    );
}

int main(
    int argc,
    char **argv
){
    if (argc < 2){
        usage(argv[0]);
        return 1;
    }

    if (
        strcmp(argv[1], "create") == 0
    ){
        return test_create();
    }

    if (
        strcmp(argv[1], "parse") == 0
    ){
        return test_parse();
    }

    if (
        strcmp(argv[1], "view") == 0
    ){
        if (argc < 3){
            fprintf(
                stderr,
                "missing file path\n"
            );

            return 1;
        }

        return test_view(argv[2]);
    }

    usage(argv[0]);

    return 1;
}
