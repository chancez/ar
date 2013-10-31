#ifndef MYAR_H
#define MYAR_H

/*
 * Public functions
 * These call read_archive
 */
int append(int index, int argc, char **argv, int verbose);
void append_all(int index, int argc, char **argv, int verbose);
void extract(int index, int argc, char **argv, int verbose);
void table_of_contents(int index, int argc, char **argv, int verbose);
void delete(int index, int argc, char **argv, int verbose);

void read_archive(int index, int argc, char **argv, char flag, int verbose);
/* read_archive calls these */
int append_file(char *file_name, int ar_fd, char *archive_name, int verbose);
int extract_file(int ar_fd, struct ar_hdr header, int verbose);
int copy_file(int new_fd, int old_fd, struct ar_hdr, char *file_name, int verbose);
void print_table(struct ar_hdr header, int verbose);

/* Utils */
int open_archive(char *archive_name, int verbose);
struct ar_hdr ar_header(struct stat st, char *file_name);
int write_armag(int fd, char* filename);
int write_header(int ar_fd, struct stat st, char* file_name);
int write_contents(int ar_fd, int in_fd, struct stat st, char *file_name);
int write_file(int in_fd, int out_fd, struct ar_hdr header, char* file_name);

/* Helpers */
int is_in_args(char *name, int index, int argc, char **argv);
void trim(char *str);
int check_byte_alignment(int in_fd, int total_written, char *file_name);
void check_args(int index, int argc);
void usage();

#endif
