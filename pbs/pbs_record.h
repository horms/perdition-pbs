#ifndef __PBS_TIME_H
#define __PBS_TIME_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <time.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

int pbs_record_show_str(char *ip, char *str, size_t width);
int pbs_record_show(char *ip, time_t time, size_t width);
int pbs_record_hrule(size_t width);
int pbs_record_title(size_t width);
int pbs_record_top(size_t width);
size_t pbs_key_width(const char *prefix);
const char *pbs_record_fix_key(const char *key, const char *prefix,
		                char **buf, size_t *buf_len);

#endif /* __PBS_TIME_H */
