#define _POSIX_C_SOURCE 200809L

#include <ar.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define BLOCKSIZE 1

int write_header(int ar_fd, int in_fd) {
    struct stat st;
    fstat(in_fd, &st);

}

int write_armag(int fd, char* filename) {
    char buf[] = ARMAG;
    int written;
    written = write(fd, buf, SARMAG);
    if (written != SARMAG) {
        perror("Error while writing ARMAG header");
        unlink(filename);
        exit(-1);
    }
}

int ar_append(int index, int argc, char **argv)
{
    int in_fd, ar_fd;
    int flags = O_RDWR;
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    char buf[BLOCKSIZE];
    struct stat st;
    int num_read, num_written;
    int total_read;
    int result;

    if ((argc - index) < 2)
        printf("Error, must provide at least 2 args\n");

    char *archive_name = argv[index];
    index++;
    // try opening without creation
    ar_fd = open(archive_name, flags, mode);
    if (ar_fd == -1) {
        // Doesn't exist, lets re-open it with the create flag
        if (errno == ENOENT) {
            flags |= O_CREAT;
            ar_fd = open(archive_name, flags, mode);
        }
        if (ar_fd == -1) {
            printf("Error, unable to open or create archive file %s.", archive_name);
            exit(-1);
        }
    } else {
        // Does exist, lets set the append flag.
        if (fcntl(ar_fd, F_SETFL, O_APPEND) == -1) {
            perror("Error setting O_APPEND flag on file\n");
            exit(-1);
        }
    }
    //flags = fcntl(ar_fd, F_GETFL);
    printf("flags=%d\n", flags & O_CREAT);
    if (flags == -1) {
        perror("Error checking flags on archive file\n");
    } else if (flags & O_CREAT) {
        printf("O_CREATE was set\n");
        // Didnt already exist, so we need to write the ARMAG header
        write_armag(ar_fd, archive_name);
    }
    return 0;
    while (index < argc) {
        in_fd = open(argv[index], O_RDONLY);
        fstat(in_fd, &st);
        total_read = 0;
        while (total_read < st.st_size) {
            num_read = read(in_fd, buf, BLOCKSIZE);
            if (num_read == -1) {
                perror("Error reading file");
                exit(-1);
            }
            result = write_header(ar_fd, in_fd);
            total_read += num_read;
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

