/**********************************************************************
 * pbs_record.h                                                May 2002
 *
 * Perdition PBS
 * Pop Before SMTP Tools
 * Copyright (C) 2002 Simon Horman <horms@verge.net.au>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 *
 **********************************************************************/

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
char *pbs_record_prefix_key(const char *key, const char *prefix);
const char *pbs_record_fix_key(const char *key, const char *prefix,
		                char **buf, size_t *buf_len);

#endif /* __PBS_TIME_H */
