#define _POSIX_C_SOURCE 200809L

#include <ar.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>

#include "file_stat.h"
#include "myar.h"

#ifdef DEBUG
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug(M, ...)
#endif

#define BLOCKSIZE 1

#define AR_HDR_SIZE sizeof(struct ar_hdr)

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
        append(optind, argc, argv);
    }
    if (t_flag) {
        table_of_contents(optind, argc, argv);
    }
    if (x_flag) {
        extract(optind, argc, argv);
    }

    if (d_flag) {
        delete(optind, argc, argv);
    }

}

int append(int index, int argc, char **argv)
{
    int in_fd, ar_fd;
    struct stat st;
    char *file_name;

    if ((argc - index) < 2) {
        printf("Error, must provide at least 2 args\n");
        exit(-1);
    }

    // Increment our index because we will be working on the non archive files next.
    char *archive_name = argv[index]; index++;

    // Open the ar file and put the ARMAG string in if its a new file.
    ar_fd = open_archive(archive_name);
    if (ar_fd == -1) {
        perror("Error opening file");
    }
    // Now we are going to iterate through each file after the archive file
    while (index < argc) {
        file_name = argv[index];
        in_fd = open(file_name, O_RDONLY);
        if (fstat(in_fd, &st) == -1) {
            perror("Unable to stat file");
            exit(-1);
        }
        if (!S_ISREG(st.st_mode)) {
            printf("%s: File format not recognized\n", file_name);
            unlink(archive_name);
            exit(-1);
        }

        // We've got the stat struct, lets write the ar_hdr
        write_header(ar_fd, st, basename(file_name));
        write_contents(ar_fd, in_fd, st, archive_name);
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

void delete(int index, int argc, char **argv)
{
    read_archive(optind, argc, argv, 'd');
}

void table_of_contents(int index, int argc, char **argv)
{
    read_archive(optind, argc, argv, 't');
}

void extract(int index, int argc, char **argv)
{
    read_archive(index, argc, argv, 'x');
}

void print_table(struct ar_hdr header, int index, int argc, char **argv, int verbose)
{
    char buf[17];
    snprintf(buf, sizeof(header.ar_name), "%s", header.ar_name);
    printf("%s\n", buf);
}

int copy_file(int new_fd, int old_fd, struct ar_hdr header, char *file_name, int verbose)
{
    // write the header to the new archive
    int num_written = write(new_fd, &header, AR_HDR_SIZE);
    if (num_written == -1) {
        perror("Error write header when making new file");
        exit(-1);
    }
    return write_file(old_fd, new_fd, header, file_name);
}

int write_file(int in_fd, int out_fd, struct ar_hdr header, char* file_name)
{
    int num_read = 0, num_written = 0, copied = 0;
    int size;
    char buffer[BLOCKSIZE];
    size = atoi(header.ar_size);
    debug("Size: %d", size);
    while (copied < size) {
        num_read = read(in_fd, buffer, BLOCKSIZE);
        if (num_read == 0) // EOF
            return 0;
        num_written = write(out_fd, buffer, BLOCKSIZE);
        if (num_read != num_written) {
            perror("Error extracting file");
            unlink(file_name);
            exit(-1);

        }
        copied += num_written;
    }
    return copied;
}

int extract_file(int ar_fd, struct ar_hdr header, int verbose)
{
    char tmp_buffer[16];
    char name_buffer[16];
    char *name = name_buffer;
    int copied = 0;
    int flags = O_WRONLY | O_CREAT | O_TRUNC;
    struct utimbuf new_times;
    time_t mtime;
    mode_t mode;

    snprintf(name, 16, "%s", header.ar_name);
    trim(name);
    snprintf(tmp_buffer, 8, "%s", header.ar_mode);
    mode = strtol(tmp_buffer, NULL, 8);

    int out_fd = open(name, flags, mode);
    if (out_fd == -1) {
        perror("Error creating file");
        exit(-1);
    }
    copied = write_file(ar_fd, out_fd, header, name);
    debug("copied: %d", copied);
    // Created the file. Time to adjust some of the values
    if (fchown(out_fd, atoi(header.ar_uid), atoi(header.ar_gid)) == -1) {
        perror("Error setting owner/group");
        exit(-1);
    }
    if (close(out_fd) == -1) {
        perror("Error closing output file");
        exit(-1);
    }
    // Set timestamps on file
    mtime = atoi(header.ar_date);
    new_times.actime = mtime;
    new_times.modtime = time(NULL);
    if (utime(name, &new_times) == -1) {
        perror("Error setting time stamps on file");
        exit(-1);
    }

    return copied;
}

void read_archive(int index, int argc, char **argv, char flag)
{
    if ((argc - index) < 1) {
        printf("Error no archive file specified!\n");
        exit(-1);
    }
    char *archive_name = argv[index]; index++;
    int ar_fd, new_ar_fd;
    int num_read = 0, position = 0, offset = 0;
    int size;
    int deleted = 0;
    struct stat st;
    struct ar_hdr header;
    char buf[SARMAG];
    char name[16];

    ar_fd = open(archive_name, O_RDONLY);
    if (ar_fd == -1) {
        perror("Error");
        exit(-1);
    }
    // Read only up to SARMAG so we fill our buffer with ARMAG
    num_read = read(ar_fd, buf, SARMAG);
    if (num_read == -1) {
        perror("Error reading file");
        exit(-1);
    }
    // Check for our armag header in the buffer otherwise invalid file type
    if (strncmp(ARMAG, buf, SARMAG) != 0) {
        printf("%s: file format not recognized\n", archive_name);
        exit(-1);
    }
    if (fstat(ar_fd, &st) == -1) {
        perror("Error trying to stat file");
    }

    debug("archive size: %lu", st.st_size);
    while (position <= st.st_size-1) {
        offset = 0;
        // Read the header into our struct
        num_read = read(ar_fd, &header, AR_HDR_SIZE);
        debug("header num_read %d", num_read);
        #ifdef DEBUG
        position = lseek(ar_fd, 0, SEEK_CUR);
        debug("position: %d", position);
        #endif
        if (num_read == -1) {
            perror("Error reading file");
            exit(-1);
        }

        size = atoi(header.ar_size);

        if (size % 2)
            offset = 1;

        snprintf(name, 16, "%s", header.ar_name);
        debug("name: %s", name);

        // hax because i hate callbacks
        switch(flag) {
        case 'x':
            // Check the header to see if this is one of the files we are
            /*
             * if no args besides the options and archive are passed,
             * extract all files
             *
             * otherwise check if the file we're at in the archive is one of
             * the specified files to extract
             */
            if ((index == argc) || (is_in_args(name, index, argc, argv))) {
                debug("extracting file %s", name);
                offset = extract_file(ar_fd, header, 0);
                if (offset == 0) // EOF
                    return;
                else
                    position += offset;
                // Check if we are on even byte boundry
                // move forward 1 if we are
                if (position % 2) {
                    position = lseek(ar_fd, 1, SEEK_CUR);
                }
            } else {
                debug("skipping");
                // Even byte alignment check
                position = lseek(ar_fd, size+offset, SEEK_CUR);
            }
        break;

        case 't':
            print_table(header, index, argc, argv, 0);
            position = lseek(ar_fd, size+offset, SEEK_CUR);
        break;

        case 'd':
            // Only delete the archive once.
            if (deleted == 0) {
                if (unlink(archive_name) == -1) {
                    perror("Error unlinking archive");
                    exit(-1);
                }
                deleted = 1;
                new_ar_fd = open_archive(archive_name);
            }
            // If there were no args, so copy nothing
            if (index == argc) {
                debug("Deleting all");
                return;
            }
            // If its in args, then we *dont* want to keep it.
            if (is_in_args(name, index, argc, argv)) {
                debug("Deleting file");
                // Even byte alignment check
                position = lseek(ar_fd, size+offset, SEEK_CUR);
            } else {
                // Copy files not in args so we keep them
                debug("Copying file");
                offset = copy_file(new_ar_fd, ar_fd, header, archive_name, 0);
                if (offset == 0) // EOF
                    return;
                else
                    position += offset;
                if (position % 2 ) {
                    position = lseek(ar_fd, 1, SEEK_CUR);
                }
            }
        break;

        default:
        break;
            debug("position: %d", position);
        }
    }
}
struct ar_hdr ar_header(struct stat st, char *file_name)
{
    struct ar_hdr header;
    snprintf(header.ar_name, sizeof(header.ar_name), "%s", file_name);
    snprintf(header.ar_date, sizeof(header.ar_date), "%lu", st.st_mtime);
    snprintf(header.ar_uid, sizeof(header.ar_uid), "%u", st.st_uid);
    snprintf(header.ar_gid, sizeof(header.ar_gid), "%u", st.st_gid);
    snprintf(header.ar_mode, sizeof(header.ar_mode), "%o", st.st_mode);
    snprintf(header.ar_size, sizeof(header.ar_size), "%lu", st.st_size);
    snprintf(header.ar_fmag, sizeof(ARFMAG), "%s", ARFMAG);
    return header;
}

void write_header(int ar_fd, struct stat st, char* file_name)
{
    struct ar_hdr header;
    char buffer[AR_HDR_SIZE];
    int num_written;
    // Create the ar_header given the stat struct and file name.
    header = ar_header(st, file_name);

    // Store it in our buffer
    sprintf(buffer, "%-16s%-12s%-6s%-6s%-8s%-10s%-2s",
        header.ar_name, header.ar_date, header.ar_uid, header.ar_gid,
        header.ar_mode, header.ar_size, header.ar_fmag);

    // write our buffer to the archive file
    num_written = write(ar_fd, buffer, AR_HDR_SIZE);
    if (num_written == -1) {
        perror("Unable to write to archive file");
        unlink(file_name);
        exit(-1);
    }
}

int write_contents(int ar_fd, int in_fd, struct stat st, char *file_name)
{
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

int open_archive(char *archive_name)
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
    // Did it already exist? O_CREAT won't be set if it did
    if (flags & O_CREAT) {
        // If we created the file then we need to write the ARMAG string
        write_armag(ar_fd, archive_name);
    }
    return ar_fd;
}


void trim(char *str) {
    int i = strlen(str) - 1;
    while (i > 0) {
        if isspace(str[i])
            str[i] = '\0';
        else
            break;
        i--;
    }
}

int is_in_args(char *name, int index, int argc, char **argv)
{
    int found = 0;
    char buffer[16];
    while (index < argc) {
        // compare truncated names
        snprintf(buffer, 16, "%s", argv[index]);
        /*
         * haxxxx, I shouldn't need strlen here...  however for some
         * reason `name` has a space and is causing the same value to
         * return 32 (ASCII for whitespace)
        */
        if (strncmp(name, buffer, strlen(argv[index])) == 0) {
            found = 1;
            break;
        }
        index++;
    }
    return found;
}

