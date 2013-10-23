#define _POSIX_C_SOURCE 200809L

#include <ar.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define BLOCKSIZE 1

int ar_append(int index, int argc, char **argv)
{
    int in_fd;
    int flags = O_RDWR | O_CREAT;
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    char buf[BLOCKSIZE];
    struct stat st;
    int num_read;
    int total_read;

    while (index < argc) {
        in_fd = open(argv[index], flags, mode);
        if (in_fd == -1) {
            perror("Cannot open archive file");
            exit(-1);
        }
        fstat(in_fd, &st);
        total_read = 0;
        while (total_read < st.st_size) {
            num_read = read(in_fd, buf, BLOCKSIZE);
            if (num_read == -1) {
                perror("Error reading file");
                exit(-1);
            }
            total_read += num_read;
            printf("%c", buf[0]);
        }

        index++;
    }

    return 1;
}

int main(int argc, char **argv)
{
    extern char *optarg;
    extern int optind, opterr, optopt;

    int v_flag = 0;
    int q_flag = 0;
    int x_flag = 0;
    int t_flag = 0;
    int d_flag = 0;
    int A_flag = 0;
    int c;

    while ((c = getopt(argc, argv, "vqxtdA")) != -1) {
        switch (c)
        {
        case 'q':
            q_flag = 1;
            break;
        case 'v':
            v_flag = 1;
            break;
        case 'x':
            x_flag = 1;
            break;
        case 't':
            t_flag = 1;
            break;
        case 'd':
            d_flag = 1;
            break;
        case 'A':
            A_flag = 1;
            break;
        case '?':
            if (optopt == 'c')
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
            return 1;
        default:
            abort();
        }
    }

    if (q_flag) {
        ar_append(optind, argc, argv);
    }

}

