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
