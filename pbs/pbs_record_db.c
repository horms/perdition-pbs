/**********************************************************************
 * pbs_record_db.c                                             May 2002
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

#include <stdlib.h>

#include "pbs_db.h"
#include "pbs_record.h"
#include "pbs_log.h"

#define PBS_TIME_PREFIX "TIME:"

int pbs_record_db_put(pbs_db_t *db, const char *ip, const char *prefix, 
		const time_t expire, const char *status) {
	const char *ip_fixed;
	char *time_ip = NULL;
	size_t buf_len = 0;
	char *buf = NULL;
	int exit_status = -1;

	ip_fixed = pbs_record_fix_key(ip, prefix, &buf, &buf_len);
	if(ip_fixed == NULL) {
		PBS_DEBUG("pbs_record_fix_key");
		goto leave;
	}

	time_ip = pbs_record_prefix_key(ip_fixed, PBS_TIME_PREFIX);
	if(time_ip == NULL) {
		PBS_DEBUG("pbs_record_prefix_key");
		goto leave;
	}

	if(pbs_db_put(db, (char *)ip_fixed, strlen(ip_fixed)+1,
				(char *)status, strlen(status)+1) < 0) {
		PBS_DEBUG("pbs_db_put");
		goto leave;
	}

	if(pbs_db_put(db, (void *)time_ip, strlen(time_ip)+1,
				(void *)&expire, sizeof(expire)) < 0) {
		PBS_DEBUG("pbs_db_put");
		pbs_db_del(db, (char *)ip_fixed, strlen(ip_fixed)+1);
		goto leave;
	}

	exit_status = 0;
leave:
	if(buf != NULL) {
		free(buf);
	}
	if(time_ip != NULL) {
		free(time_ip);
	}

	return(exit_status);
}


int pbs_record_db_get(pbs_db_t *db, const char *ip, const char *prefix, 
		time_t *expire, char **status, size_t *status_len) {
	const char *ip_fixed;
	char *time_ip = NULL;
	size_t buf_len = 0;
	size_t dummy;
	char *buf = NULL;
	int exit_status = -1;

	ip_fixed = pbs_record_fix_key(ip, prefix, &buf, &buf_len);
	if(ip_fixed == NULL) {
		PBS_DEBUG("pbs_record_fix_key");
		goto leave;
	}

	time_ip = pbs_record_prefix_key(ip_fixed, PBS_TIME_PREFIX);
	if(time_ip == NULL) {
		PBS_DEBUG("pbs_record_prefix_key");
		goto leave;
	}

	if(pbs_db_get(db, (void *)ip_fixed, strlen(ip_fixed)+1,
				(void **)status, status_len) < 0) {
		PBS_DEBUG("pbs_db_get");
		goto leave;
	}
	if(*status_len > 0) {
		*status_len=*status_len-1;
	}

	if(pbs_db_get(db, (void *)time_ip, strlen(time_ip)+1,
				(void **)expire, &dummy) < 0) {
		PBS_DEBUG("pbs_db_get");
		goto leave;
	}

	exit_status = 0;
leave:
	if(buf != NULL) {
		free(buf);
	}
	if(time_ip != NULL) {
		free(time_ip);
	}

	return(exit_status);
}


int pbs_record_db_del(pbs_db_t *db, const char *ip, const char *prefix) {
	const char *ip_fixed;
	char *time_ip = NULL;
	size_t buf_len = 0;
	char *buf = NULL;
	int exit_status = -1;

	ip_fixed = pbs_record_fix_key(ip, prefix, &buf, &buf_len);
	if(ip_fixed == NULL) {
		PBS_DEBUG("pbs_record_fix_key");
		goto leave;
	}

	time_ip = pbs_record_prefix_key(ip_fixed, PBS_TIME_PREFIX);
	if(time_ip == NULL) {
		PBS_DEBUG("pbs_record_prefix_key");
		goto leave;
	}

	if(pbs_db_del(db, (char *)ip_fixed, strlen(ip_fixed)+1) < 0) {
		PBS_DEBUG("pbs_db_get");
		goto leave;
	}

	if(pbs_db_del(db, (char *)time_ip, strlen(time_ip)+1) < 0) {
		PBS_DEBUG("pbs_db_get");
		goto leave;
	}

	exit_status = 0;
leave:
	if(buf != NULL) {
		free(buf);
	}
	if(time_ip != NULL) {
		free(time_ip);
	}

	return(exit_status);
}
