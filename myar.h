#ifndef MYAR_H
#define MYAR_H

/*
 * Public functions
 * These call read_archive
 */
int append(int index, int argc, char **argv, int verbose);
void extract(int index, int argc, char **argv, int verbose);
void table_of_contents(int index, int argc, char **argv, int verbose);
void delete(int index, int argc, char **argv, int verbose);

void read_archive(int index, int argc, char **argv, char flag, int verbose);
/* read_archive calls these */
int extract_file(int ar_fd, struct ar_hdr header, int verbose);
int copy_file(int new_fd, int old_fd, struct ar_hdr, char *file_name, int verbose);
void print_table(struct ar_hdr header, int verbose);

/* Utils */
int open_archive(char *archive_name);
struct ar_hdr ar_header(struct stat st, char *file_name);
void write_armag(int fd, char* filename);
void write_header(int ar_fd, struct stat st, char* file_name);
int write_contents(int ar_fd, int in_fd, struct stat st, char *file_name);
int write_file(int in_fd, int out_fd, struct ar_hdr header, char* file_name);

/* Helpers */
int is_in_args(char *name, int index, int argc, char **argv);
void trim(char *str);

#endif
