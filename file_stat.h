#ifndef FILE_STAT_H
#define FILE_STAT_H

#include <sys/types.h>

#define FP_SPECIAL 1

char * file_perm_string(mode_t perm, int flags);

#endif
