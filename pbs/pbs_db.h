/**********************************************************************
 * pbs_db.h                                                    May 2002
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

#ifndef __PBS_DB_H
#define __PBS_DB_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <db.h>

typedef DB pbs_db_t;

pbs_db_t *pbs_db_open(const char *filename, const int mode);

int pbs_db_close(pbs_db_t *db);

int pbs_db_sync(pbs_db_t *db);

int pbs_db_put(pbs_db_t *db, void *keyp, size_t key_len, void *datap, 
		size_t data_len);

int pbs_db_get(pbs_db_t *db, void *keyp, size_t key_len, 
		void **datap, size_t *data_len);

int pbs_db_del(pbs_db_t *db, void *key, size_t key_len);

int pbs_db_traverse(pbs_db_t *db,
	int(*func)(void *db, void *key, size_t key_len,
		void *data, size_t data_len, void *func_data),
	void *func_data);

int pbs_db_traverse_func_show_record(void *db, void *key, size_t key_len,
	void *data, size_t data_len, void *func_data);

int pbs_db_traverse_func_expire_record(void *db, void *key, size_t key_len,
	void *data, size_t data_len, void *func_data);

char *pbs_db_strerror(int error);

#endif
