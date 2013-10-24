#define _POSIX_C_SOURCE 200809L

#include <ar.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "file_stat.h"

#define BLOCKSIZE 1

#define AR_HDR_SIZE sizeof(struct ar_hdr)

void ar_contents(int index, int argc, char **argv)
{
    if ((argc - index) < 1) {
        printf("Error no archive file specified!\n");
        exit(-1);
    }
    char *archive_name = argv[index];
    int ar_fd;
    int num_read;

    char buf[AR_HDR_SIZE];

    ar_fd = open(archive_name, O_RDONLY);
    if (ar_fd == -1) {
        perror("Error");
        exit(-1);
    }
    num_read = read(ar_fd, buf, SARMAG);
    if (strncmp(ARMAG, buf, SARMAG) != 0) {
        printf("%s: file format not recognized\n", archive_name);
        exit(-1);
    }
}

void make_ar_hdr(struct ar_hdr *header, struct stat st, char *file_name)
{
    strncpy(header->ar_name, file_name, sizeof(header->ar_name)/sizeof(char));
    snprintf(header->ar_date, sizeof(header->ar_date)/sizeof(char), "%lu", st.st_mtime);
    snprintf(header->ar_uid, sizeof(header->ar_uid)/sizeof(char), "%u", st.st_uid);
    snprintf(header->ar_gid, sizeof(header->ar_gid)/sizeof(char), "%u", st.st_gid);
    snprintf(header->ar_mode, sizeof(header->ar_mode)/sizeof(char), "%o", st.st_mode);
    snprintf(header->ar_size, sizeof(header->ar_size)/sizeof(char), "%lu", st.st_size);
    snprintf(header->ar_fmag, sizeof(ARFMAG)/sizeof(char), "%s", ARFMAG);
}

void write_ar_header(int ar_fd, struct stat st, char* file_name)
{
    struct ar_hdr *header = (struct ar_hdr*)malloc(AR_HDR_SIZE);
    // Need space for NULL
    char buffer[AR_HDR_SIZE+1];
    int num_written;
    // Create the ar_header given the stat struct and file name.
    make_ar_hdr(header, st, file_name);
    // Store it in our buffer
    sprintf(buffer, "%-15s%-12s%-6s%-6s%-8s%-10s%-2s",
        header->ar_name, header->ar_date, header->ar_uid, header->ar_gid,
        header->ar_mode, header->ar_size, header->ar_fmag);

    // write our buffer to the archive file
    num_written = write(ar_fd, buffer, AR_HDR_SIZE-1); // Dont write NULL
    if (num_written == -1) {
        perror("Unable to write to archive file");
        unlink(file_name);
        exit(-1);
    }
    free(header);
}

int write_ar_content(int ar_fd, int in_fd, struct stat st, char *file_name) {
    int total_read = 0, total_written = 0, num_written = 0, num_read = 0;
    char buf[BLOCKSIZE];

    while (total_read < st.st_size) {
        num_read = read(in_fd, buf, BLOCKSIZE);
        num_written = write(ar_fd, buf, BLOCKSIZE);
        if (num_read != num_written) {
            perror("Error writing file");
            unlink(file_name);
            exit(-1);
        }
        total_read += num_read;
        total_written += num_written;
    }
    if (total_written % 2) // Even byte alignment
        write(ar_fd, "\n", 1);
    return 0;
}

void write_armag(int fd, char* filename) {
    char buf[] = ARMAG;
    int written;
    written = write(fd, buf, SARMAG);
    if (written != SARMAG) {
        printf("Error while writing ARMAG header");
        unlink(filename);
        exit(-1);
    }
}

int ar_open_append(char *archive_name)
{
    int ar_fd;
    int flags = O_RDWR | O_APPEND;
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    ar_fd = open(archive_name, flags, mode);
    if (ar_fd == -1) {
        // Doesn't exist, lets re-open it with the create flag
        if (errno == ENOENT) {
            flags |= O_CREAT;
            ar_fd = open(archive_name, flags, mode);
        }
        if (ar_fd == -1) {
            perror("Error, opening archive file");
            exit(-1);
        }
    }
    // Did it already exist? O_CREAT won't be set if it did;w
    if (flags & O_CREAT) {
        // If we created the file then we need to write the ARMAG string
        write_armag(ar_fd, archive_name);
    }
    return ar_fd;
}

int ar_append(int index, int argc, char **argv)
{
    int in_fd, ar_fd;
    struct stat st;

    if ((argc - index) < 2) {
        printf("Error, must provide at least 2 args\n");
        exit(-1);
    }

    char *archive_name = argv[index];
    // Increment our index because we will be working on the non archive files next.
    index++;

    // Open the ar file and put the ARMAG string in if its a new file.
    ar_fd = ar_open_append(archive_name);
    if (ar_fd == -1) {
        perror("Error opening file");
    }
    // Now we are going to iterate through each file after the archive file
    while (index < argc) {
        in_fd = open(argv[index], O_RDONLY);
        if (fstat(in_fd, &st) == -1) {
            perror("Unable to stat file");
            exit(-1);
        }
        // We've got the stat struct, lets write the ar_hdr
        write_ar_header(ar_fd, st, argv[index]);
        write_ar_content(ar_fd, in_fd, st, argv[index]);
        index++;
        if (close(in_fd) == -1) {
            perror("Unable to close input file");
            exit(-1);
        }
    }
    if (close(ar_fd) == -1) {
        perror("Unable to close archive file");
        exit(-1);
    }

    return 0;
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
    if (t_flag) {
        ar_contents(optind, argc, argv);
    }

}

