/**********************************************************************
 * pbs_option.h                                                May 2002
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

#ifndef __PBS_OPTIONS_H
#define __PBS_OPTIONS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#define PBS_LOG_LEVEL_QUIET  LOG_ERR
#define PBS_LOG_LEVEL_NORMAL LOG_INFO
#define PBS_LOG_LEVEL_DEBUG  LOG_DEBUG

#define PBS_MODE_DAEMON     0x01
#define PBS_MODE_DAEMON_STR "daemon"
#define PBS_MODE_LIST       0x02
#define PBS_MODE_LIST_STR   "list"
#define PBS_MODE_INSERT     0x04
#define PBS_MODE_INSERT_STR "insert"
#define PBS_MODE_REMOVE     0x08
#define PBS_MODE_REMOVE_STR "remove"
#define PBS_MODE_PURGE      0x10
#define PBS_MODE_PURGE_STR "purge"
#define PBS_MODE_SETENV      0x20
#define PBS_MODE_SETENV_STR "setenv"

#ifndef PBS_DEFAULT_LOG_FILENAME
#define PBS_DEFAULT_LOG_FILENAME "/var/log/mail.log"
#endif
#ifndef PBS_DEFAULT_DB_FILENAME
#define PBS_DEFAULT_DB_FILENAME "/etc/mail/popauth.db"
#endif
#ifndef PBS_DEFAULT_PREFIX
#define PBS_DEFAULT_PREFIX "POP:"
#endif
#define PBS_DEFAULT_REGEX "perdition.*Auth: ([^-]+)->[^-]+ user=\"([^\"]+)\" "
#define PBS_DEFAULT_TIMEOUT 3600 /* In seconds */
#define PBS_DEFAULT_MODE PBS_MODE_DAEMON
#define PBS_DEFAULT_LOG_LEVEL PBS_LOG_LEVEL_NORMAL
#define PBS_DEFAULT_LOG_FACILITY "mail"


typedef struct {
	const char *log_filename;
	const char *db_filename;
	int no_daemon;
	const char *regex;
	const char *prefix;
	time_t timeout;
	const char *log_facility;
	int log_level;
	int mode;
	const char **leftover;
} pbs_options_t;

pbs_options_t *pbs_options_parse(int argc, char **argv);
void pbs_usage(int exit_status);

int pbs_mode_int(const char *mode);
const char *pbs_mode_str(const int mode);
void pbs_options_log(pbs_options_t *opt);

#endif /* __PBS_OPTIONS_H */
