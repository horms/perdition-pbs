/**********************************************************************
 * pbs_record.c                                                May 2002
 * Horms                                             horms@vergenet.net
 *
 * Perdition PBS
 * Pop Before SMTP Tools
 * Copyright (C) 2002 Horms
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <time.h>
#include <stdlib.h>

#include "pbs_log.h"
#include "pbs_record.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#define PBS_DATE_WIDTH 40
#define PBS_IP_WIDTH   15

int pbs_record_show_str(char *ip, char *str, size_t width) {
	printf("%-*s | %s\n", width, ip, str);
	return(0);
}

int pbs_record_show(char *ip, time_t time, size_t width) {
	struct tm tm;
	char buf[PBS_DATE_WIDTH];

	if(localtime_r(&time, &tm) == NULL) {
		VANESSA_LOGGER_DEBUG_ERRNO("localtime_r");
		return(-1);
	}
	strftime(buf, PBS_DATE_WIDTH-1, "%c", &tm);
	printf("%-*s | %s\n", width, ip, buf);
	return(0);
}

int pbs_record_hrule(size_t width) {
	size_t rule_width;
	char *buffer;

	rule_width = width + PBS_DATE_WIDTH + 2;

	buffer = malloc(rule_width+1);
	if(buffer == NULL) {
		VANESSA_LOGGER_DEBUG_ERRNO("malloc");
		return(-1);
	}

	memset(buffer, '-', rule_width);
	*(buffer+rule_width)='\0';
	*(buffer+width+1)='+';

	printf("%s\n", buffer);
	free(buffer);

	return(0);
}

int pbs_record_title(size_t width) {
	printf("%-*s | %s\n", width, "Key", "Expires");
	return(0);
}

int pbs_record_top(size_t width) {
	if(pbs_record_title(width) < 0) {
		VANESSA_LOGGER_DEBUG("pbs_record_title");
		return(-1);
	}
	if(pbs_record_hrule(width) < 0) {
		VANESSA_LOGGER_DEBUG("pbs_record_title");
		return(-1);
	}

	return(0);
}

size_t pbs_key_width(const char *prefix) {
	size_t width = PBS_IP_WIDTH;

	if(prefix != NULL) {
		width += strlen(prefix);
	}

	return(width);
}


char *pbs_record_prefix_key(const char *key, const char *prefix) {
	size_t prefix_len;
	size_t key_len;
	size_t new_key_len;
	char *new_key;


	prefix_len = strlen(prefix);
	key_len = strlen(key);
	new_key_len = key_len + prefix_len + 1;

	new_key = (char *)malloc(new_key_len);
	if(new_key == NULL) {
		VANESSA_LOGGER_DEBUG_ERRNO("realloc");
		return(NULL);
	}
	memset(new_key, 0, new_key_len);
	memcpy(new_key, prefix, prefix_len);
	memcpy(new_key+prefix_len, key, key_len);
	
	return(new_key);
}

const char *pbs_record_fix_key(const char *key, const char *prefix,
		char **buf, size_t *buf_len) {
	size_t prefix_len;
	size_t k_len;

	prefix_len = strlen(prefix);

	if(strncmp(key, prefix, prefix_len) == 0) {
		return(key);
	}

	k_len = strlen(key);
	if(*buf_len < prefix_len +k_len + 1) {
		*buf_len = prefix_len + k_len + 1;
		*buf = realloc(*buf, *buf_len);
		if(*buf == NULL) {
			VANESSA_LOGGER_DEBUG_ERRNO("realloc");
			return(NULL);
		}
	}
	memset(*buf, 0, *buf_len);
	memcpy(*buf, prefix, prefix_len);
	memcpy(*buf+prefix_len, key, k_len);
	
	return(*buf);
}
