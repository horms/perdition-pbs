/**********************************************************************
 * pbs_record_db.c                                             May 2002
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

#ifndef _PBS_RECORD_DB_H
#define _PBS_RECORD_DB_H

#include "pbs_db.h"

int pbs_record_db_put(pbs_db_t *db, const char *ip, const char *prefix, 
		const time_t expire, const char *status);


int pbs_record_db_get(pbs_db_t *db, const char *ip, const char *prefix, 
		time_t *expire, char **status, size_t *status_len);

int pbs_record_db_del(pbs_db_t *db, const char *ip, const char *prefix);

#endif /* _PBS_RECORD_DB_H */
