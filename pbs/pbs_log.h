/**********************************************************************
 * pbs_log.h                                                   May 2002
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

#ifndef SYSLOG_BERT
#define SYSLOG_BERT

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <vanessa_logger.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#include "pbs_db.h"

#define LOG_IDENT "perdition-pbs"

extern vanessa_logger_t *pbs_vl;
extern int errno;


/*
 * Hooray for format string problems!
 *
 * Each of the logging macros have two versions. The UNSAFE version will
 * accept a format string. You should _NOT_ use the UNSAFE versions if the
 * first argument, the format string, is derived from user input. The safe
 * versions (versions that do not have the "_UNSAFE" suffix) do not accept
 * a format string and only accept one argument, the string to log. These
 * should be safe to use with user derived input.
 */

#define PBS_LOG_UNSAFE(priority, fmt, args...) \
  vanessa_logger_log(pbs_vl, priority, fmt, ## args)

#define PBS_LOG(priority, str) \
  vanessa_logger_log(pbs_vl, priority, "%s", str)

#define PBS_INFO_UNSAFE(fmt, args...) \
  vanessa_logger_log(pbs_vl, LOG_INFO, fmt, ## args)

#define PBS_INFO(str) \
  vanessa_logger_log(pbs_vl, LOG_INFO, "%s", str)

#define PBS_ERR_UNSAFE(fmt, args...) \
  vanessa_logger_log(pbs_vl, LOG_ERR, fmt, ## args)

#define PBS_ERR(str) \
  vanessa_logger_log(pbs_vl, LOG_ERR, "%s", str)

#define PBS_DEBUG_UNSAFE(fmt, args...) \
  vanessa_logger_log(pbs_vl, LOG_DEBUG, __FUNCTION__ ": " fmt, ## args)

#define PBS_DEBUG(str) \
  vanessa_logger_log(pbs_vl, LOG_DEBUG, __FUNCTION__ ": %s", str)

#define PBS_DEBUG_ERRNO(s) \
  vanessa_logger_log(pbs_vl, LOG_DEBUG, "%s: %s: %s", \
    __FUNCTION__, s, strerror(errno))

#define PBS_DEBUG_DB(s, err) \
  vanessa_logger_log(pbs_vl, LOG_DEBUG, "%s: %s: %s", \
    __FUNCTION__, s, pbs_db_strerror(err))

#endif
