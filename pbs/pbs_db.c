/**********************************************************************
 * pbs_db.c                                                    May 2002
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
#include <db.h>
#include <time.h>

#include "pbs_db.h"
#include "pbs_option.h"
#include "pbs_log.h"
#include "pbs_record.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

pbs_db_t *pbs_db_open(const char *filename, const int mode){
	DB *dbp;
	int status;

	status = db_create(&dbp, NULL, 0);
	if(status != 0) {
		VANESSA_LOGGER_DEBUG_DB("db_create", status);
		return(NULL);
	}
	

	status = dbp->open(dbp, 
#ifdef HAVE_BDB_4_1
		      	NULL,
#endif
	  		filename, NULL, DB_HASH, DB_CREATE, mode);
	if(status != 0) {
		VANESSA_LOGGER_DEBUG_DB("dbp->open", status);
		dbp->close(dbp, 0);
		return(NULL);
	}

	return((pbs_db_t *)dbp);
}

int pbs_db_close(pbs_db_t *db) {
	DB *dbp;
	int status;

	dbp = (DB *)db;
	status = dbp->close(dbp, 0);
	if(status != 0) {
		VANESSA_LOGGER_DEBUG_DB("dbp->close", status);
		return(-1);
	}

	return(0);
}

int pbs_db_sync(pbs_db_t *db) {
	DB *dbp;
	int status;

	dbp = (DB *)db;
	status = dbp->sync(dbp, 0);
	if(status != 0) {
		VANESSA_LOGGER_DEBUG_DB("dbp->sync", status);
		return(-1);
	}

	return(0);
}

int pbs_db_put(pbs_db_t *db, void *key, size_t key_len, void *data, 
		size_t data_len)
{
	DB *dbp;
	DBT keyp;
	DBT datap;
	int status;

	memset(&keyp, 0, sizeof(keyp));
	keyp.data = key;
	keyp.size = key_len;

	memset(&datap, 0, sizeof(datap));
	datap.data = data;
	datap.size = data_len;

	dbp = (DB *)db;

	status = dbp->put(dbp, NULL, &keyp, &datap, 0);
	if(status != 0) {
		VANESSA_LOGGER_DEBUG_DB("dbp->put", status);
		return(-1);
	}

	return(0);
}

int pbs_db_get(pbs_db_t *db, void *key, size_t key_len, 
		void **data, size_t *data_len)
{
	DB *dbp;
	DBT keyp;
	DBT datap;
	int status;

	memset(&keyp, 0, sizeof(keyp));
	keyp.data = key;
	keyp.size = key_len;

	memset(&datap, 0, sizeof(datap));

	dbp = (DB *)db;

	status = dbp->get(dbp, NULL, &keyp, &datap, 0);
	if(status != 0) {
		*data = NULL;
		*data_len = 0;
		VANESSA_LOGGER_DEBUG_DB("dbp->get", status);
		return(-1);
	}

	*data = datap.data;
	*data_len = datap.size;

	return(0);
}

int pbs_db_del(pbs_db_t *db, void *key, size_t key_len)
{
	DB *dbp;
	DBT keyp;
	int status;

	memset(&keyp, 0, sizeof(keyp));
	keyp.data = key;
	keyp.size = key_len;

	dbp = (DB *)db;

	status = dbp->del(dbp, NULL, &keyp, 0);
	if(status != 0) {
		VANESSA_LOGGER_DEBUG_DB("dbp->get", status);
		return(-1);
	}

	return(0);
}

int pbs_db_traverse(pbs_db_t *db, 
		int(*func)(void *db, void *key, size_t key_len,
			void *data, size_t data_len, void *func_data), 
		void *func_data)
{
	DB *dbp;
	DBC *cursorp;
	DBT keyp;
	DBT datap;
	int status;
	int return_status = 0;

	dbp = (DB *)db;

	status = dbp->cursor(dbp, NULL, &cursorp, 0);
	if(status != 0) {
		VANESSA_LOGGER_DEBUG_DB("db->cursor", status);
		return(-1);
	}


	while(1) {
		memset(&keyp, 0, sizeof(keyp));
		memset(&datap, 0, sizeof(datap));
		status = cursorp->c_get(cursorp, &keyp, &datap, DB_NEXT);
		if(status == DB_NOTFOUND) {
			break;
		}
		else if(status != 0) {
			VANESSA_LOGGER_DEBUG_DB("cursor->c_get", status);
			return_status = -1;
			break;
		}

		if(func(cursorp, keyp.data, keyp.size, datap.data, datap.size,
					func_data) < 0) {
			VANESSA_LOGGER_DEBUG("func");
			return_status=-1;
			break;
		}
	}

	status = cursorp->c_close(cursorp);
	if(status != 0) {
		VANESSA_LOGGER_DEBUG_DB("cursor->c_close", status);
		return_status = -1;
	}
	
	return(0);
}

int pbs_db_traverse_func_show_record(void *db, void *key, size_t key_len,
		void *data, size_t data_len, void *func_data) 
{
	time_t time;
	char *ip;
	int width;

	time=*(time_t *)data;
	ip=(char *)key;
	width=(func_data==NULL)?0:*(int *)func_data;

	if(pbs_record_show(ip, time, width) < 0) {
		VANESSA_LOGGER_DEBUG("pbs_record_show");
		return(-1);
	}

	return(0);
}


int pbs_db_traverse_func_expire_record(void *db, void *key, size_t key_len,
		void *data, size_t data_len, void *func_data) 
{
	DBC *cursorp;
	time_t expire;
	time_t now;
	char *ip;
	int status;

	cursorp = (DBC *)db;

	expire=*(time_t *)data;
	ip=(char *)key;
	now=*(time_t *)func_data;


	if(difftime(now, expire) > 0) {
		return(0);
	}

	status = cursorp->c_del(cursorp, 0);
	if(status != 0) {
		VANESSA_LOGGER_DEBUG_DB("cursor->c_del", status);
		return(-1);
	}

	VANESSA_LOGGER_INFO_UNSAFE("Expired %s\n", ip);

	return(0);
}


char *pbs_db_strerror(int error) {
	return(db_strerror(error));
}
