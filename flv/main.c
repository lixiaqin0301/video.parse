#include "flvparser.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filepath>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "%s:%d %s open %s failed: %s\n", __FILE__, __LINE__, __FUNCTION__, argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("flv file path: %s\n\n", argv[1]);

    if (!parseFlvHeader(fp)) {
        fprintf(stderr, "%s:%d %s parseFlvHeader %s failed\n", __FILE__, __LINE__, __FUNCTION__, argv[0]);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    for (;;) {
        if (!parseFlvTag(fp)) {
            fprintf(stderr, "%s:%d %s parseFlvHeader %s failed\n", __FILE__, __LINE__, __FUNCTION__, argv[0]);
            fclose(fp);
            exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
}
