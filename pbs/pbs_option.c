/**********************************************************************
 * pbs_option.c                                                May 2002
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

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>
#include <signal.h>
#include <time.h>
#include <popt.h>
#include <vanessa_logger.h>
#include <vanessa_socket.h>

#include "pbs_log.h"
#include "pbs_option.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#define STR_NULL "(null)"
#define STR_NULL_SAFE(string) (string==NULL)?STR_NULL:string
#define BIN_OPT_STR(opt) ((opt)?"on":"off")


/**********************************************************************
 * str_basename
 * Find the filename of a fully qualified path to a file
 * pre: filename: name of file to find basename of
 * post: basename of filename is returned
 * return: NULL if filename is NULL
 *         pointer within filename pointing to basename of filename
 *
 * Not 8 bit clean
 **********************************************************************/

const char *str_basename(const char *filename){
    char *result;

    if(filename==NULL){
      return(NULL);
    }
    
    result=strrchr(filename, '/');

    return((result==NULL)?filename:result+1);
}

pbs_options_t *pbs_options_parse(int argc, char **argv) {
	char c;
	pbs_options_t *opt;
	poptContext context;
	const char *basename;

	static struct poptOption options[] =
	{
		{"db_file",       'D', POPT_ARG_STRING, NULL, 'D'},
		{"debug",        'd',  POPT_ARG_NONE,   NULL, 'd'},
		{"log_facility", 'F',  POPT_ARG_NONE,   NULL, 'F'},
		{"help",         'h',  POPT_ARG_NONE,   NULL, 'h'},
		{"log_file",      'L', POPT_ARG_STRING, NULL, 'L'},
		{"mode",          'm', POPT_ARG_STRING, NULL, 'm'},
		{"no_daemon",     'n', POPT_ARG_NONE,   NULL, 'n'},
		{"perfix",        'p', POPT_ARG_STRING, NULL, 'p'},
		{"quiet",         'q', POPT_ARG_NONE,   NULL, 'q'},
		{"regex",         'r', POPT_ARG_STRING, NULL, 'r'},
		{"user",          'u', POPT_ARG_STRING, NULL, 'u'},
		{"group",         'g', POPT_ARG_STRING, NULL, 'g'},
		{NULL,             0,  0,               NULL,  0 }
	};

	opt = malloc(sizeof(pbs_options_t));
	if(opt == NULL) {
		PBS_DEBUG_ERRNO("malloc");	
		return(NULL);
	}

	memset(opt, 0, sizeof(pbs_options_t));

	/* Set defaults */
	opt->log_filename = PBS_DEFAULT_LOG_FILENAME;
	opt->db_filename = PBS_DEFAULT_DB_FILENAME;
	opt->log_facility = PBS_DEFAULT_LOG_FACILITY;
	opt->regex = PBS_DEFAULT_REGEX;
	opt->timeout = PBS_DEFAULT_TIMEOUT;
	opt->prefix = PBS_DEFAULT_PREFIX;
	opt->log_level = PBS_DEFAULT_LOG_LEVEL;
	opt->user = PBS_DEFAULT_USERNAME;
	opt->group = PBS_DEFAULT_GROUP;
	opt->mode = PBS_DEFAULT_MODE;

  	if(argc==0 || argv==NULL) return(opt);

	basename = str_basename(argv[0]);

	if(strcmp("perdition-pbs-setenv", basename) == 0) {
		opt->mode = PBS_MODE_SETENV;
	}
	else if(strcmp("perdition-pbs-list", basename) == 0) {
		opt->mode = PBS_MODE_LIST;
	}
	else if(strcmp("perdition-pbs-insert", basename) == 0) {
		opt->mode = PBS_MODE_INSERT;
	}
	else if(strcmp("perdition-pbs-remove", basename) == 0) {
		opt->mode = PBS_MODE_REMOVE;
	}
	else if(strcmp("perdition-pbs-setenv", basename) == 0) {
		opt->mode = PBS_MODE_REMOVE;
	}
	else if(strcmp("perdition-pbs-purge", basename) == 0) {
		opt->mode = PBS_MODE_PURGE;
	}
	
  	context = poptGetContext(LOG_IDENT, argc, (const char **)argv, 
			options, 0);

	while ((c=poptGetNextOpt(context)) >= 0){
		optarg=(char *)poptGetOptArg(context);
		switch (c){
			case 'd':
				opt->log_level = PBS_LOG_LEVEL_DEBUG;
				break;
			case 'D':
				opt->db_filename = optarg;
				break;
			case 'F':
				opt->log_facility = optarg;
				break;
			case 'g':
				opt->group = optarg;
				break;
			case 'h':
				pbs_usage(0);
				break;
			case 'L':
				opt->log_filename = optarg;
				break;
			case 'm':
				opt->mode = pbs_mode_int(optarg);
				if(opt->mode < 0) {
					pbs_usage(-1);
				}
				break;
			case 'n':
				opt->no_daemon = 1;
				break;
			case 'p':
				opt->prefix = optarg;
				break;
			case 'q':
				opt->log_level = PBS_LOG_LEVEL_QUIET;
				break;
			case 'r':
				opt->regex = optarg;
				break;
			case 'u':
				opt->user = optarg;
				break;
		}
	}
	
	if (c < -1) {
		fprintf(stderr, 
			"options: %s: %s\n",
			poptBadOption(context, POPT_BADOPTION_NOALIAS),
			poptStrerror(c));
		pbs_usage(-1);
	}

	opt->leftover = poptGetArgs(context);
	if(opt->leftover != NULL && *(opt->leftover) == NULL) {
		opt->leftover = NULL;
	}

	/* Lame-o-rama
	 * If we call poptFreeContext() then the value returned
	 * by poptGetArgs() becomes NULL */
	if(opt->leftover == NULL) {
		poptFreeContext(context);
	}
	
	return(opt);
}


/**********************************************************************
 * usage
 * Display usage information and exit
 * Prints to stdout if exit_status=0, stderr otherwise
 **********************************************************************/

void pbs_usage(int exit_status){
	FILE *stream;

	if(exit_status!=0){
		stream=stderr;
	}
	else{
		stream=stdout;
	}

    
	fprintf(stream, 
	"perdition version " VERSION " Copyright Horms\n"
	"\n"
	"Usage: " LOG_IDENT " [-m|--mode daemon|purge] [options]\n"
	"Usage: " LOG_IDENT " -m|--mode list [options] [--] [key...]\n"
	"Usage: " LOG_IDENT " -m|--mode insert|remove [options] [--] key...\n"
	"Usage: " LOG_IDENT " -m|--mode setenv [options] [--] arg...\n"
	"\n"
	"  -m, --mode: MODE: Mode to run in.\n"
	"                    One of: \"daemon\", \"list\", \"insert\",\n"
	"                            \"remove\", \"setenv\" or \"purge\".\n"
	"                    (default \"%s\")\n"
	"\n"
	"  options:\n"
	"    -h, --help: print this message\n"
	"    -D, --db_file FILE:  Database file to access\n"
	"                         (default \"%s\")\n"
	"    -d, --debug:         Verbose error messages\n"
	"    -F, --log_facility FACILITY:\n"
	"                         Syslog facility to log to. If the faclilty\n"
	"                         has a leading '/' then it will be treated\n"
	"                         as a file to log to.\n"
	"                         (default \"%s\")\n"
        "    -g, --group: GROUP   Group to run as\n"
	"                         (default \"%s\")\n"
	"    -L, --log_file FILE: Log file to monitor\n"
	"                         (default \"%s\")\n"
	"    --no_daemon:         Do not detach from terminal when in\n"
	"                         daemon mode\n"
	"    -p, --prefix STRING: Prefix to prepend to keys to be\n"
	"                         (default \"%s\")\n"
	"                         stored in or retrieved from the database\n"
	"    -q, --quiet:         Supress all but critical log messages\n"
	"    -r, --regex: REGEX   Regular expression to use when parsing\n"
	"                         log file. Should match the ip address as\n"
	"                         the first result, and optionally the\n"
	"                         username as the second result\n"
	"                         (default \"%s\")\n"
        "    -u, --user: USERNAME User to run as\n"
	"                         (default \"%s\")\n"
	"\n"
	"Notes: Default for binary flags is off\n",
	STR_NULL_SAFE(pbs_mode_str(PBS_DEFAULT_MODE)),
	STR_NULL_SAFE(PBS_DEFAULT_DB_FILENAME),
	STR_NULL_SAFE(PBS_DEFAULT_GROUP),
	STR_NULL_SAFE(PBS_DEFAULT_LOG_FACILITY),
	STR_NULL_SAFE(PBS_DEFAULT_LOG_FILENAME),
	STR_NULL_SAFE(PBS_DEFAULT_PREFIX),
	STR_NULL_SAFE(PBS_DEFAULT_REGEX),
	STR_NULL_SAFE(PBS_DEFAULT_USERNAME)
	);

  	exit(exit_status);
}

void pbs_options_log(pbs_options_t *opt){
	PBS_INFO_UNSAFE(
		"db_file=\"%s\" "
		"debug=\"%s\" "
		"group=\"%s\" "
		"log_facility=\"%s\" "
		"log_file=\"%s\" "
		"mode=\"%s\" "
		"no_daemon=\"%s\" "
		"prefix=\"%s\" "
		"quiet=\"%s\" "
		"regex=\"%s\" "
		"user=\"%s\"",
		STR_NULL_SAFE(opt->db_filename),
		BIN_OPT_STR(opt->log_level == PBS_LOG_LEVEL_DEBUG),
		STR_NULL_SAFE(opt->group),
		STR_NULL_SAFE(opt->log_facility),
		STR_NULL_SAFE(opt->log_filename),
		STR_NULL_SAFE(pbs_mode_str(opt->mode)),
		BIN_OPT_STR(opt->no_daemon),
		STR_NULL_SAFE(opt->prefix),
		BIN_OPT_STR(opt->log_level == PBS_LOG_LEVEL_QUIET),
		STR_NULL_SAFE(opt->regex),
		STR_NULL_SAFE(opt->user)
	);
}


int pbs_mode_int(const char *mode) {
	if(strcasecmp(mode, PBS_MODE_DAEMON_STR) == 0) {
		return(PBS_MODE_DAEMON);
	}
	if(strcasecmp(mode, PBS_MODE_LIST_STR) == 0) {
		return(PBS_MODE_LIST);
	}
	if(strcasecmp(mode, PBS_MODE_INSERT_STR) == 0) {
		return(PBS_MODE_INSERT);
	}
	if(strcasecmp(mode, PBS_MODE_REMOVE_STR) == 0) {
		return(PBS_MODE_REMOVE);
	}
	if(strcasecmp(mode, PBS_MODE_PURGE_STR) == 0) {
		return(PBS_MODE_PURGE);
	}
	if(strcasecmp(mode, PBS_MODE_SETENV_STR) == 0) {
		return(PBS_MODE_SETENV);
	}

	PBS_DEBUG_UNSAFE("unknown mode: \"%s\"", mode);
	return(-1);
}


const char *pbs_mode_str(const int mode) {
	switch(mode) {
		case PBS_MODE_DAEMON:
		      return(PBS_MODE_DAEMON_STR);
		case PBS_MODE_LIST:
		      return(PBS_MODE_LIST_STR);
		case PBS_MODE_INSERT:
		      return(PBS_MODE_INSERT_STR);
		case PBS_MODE_REMOVE:
		      return(PBS_MODE_REMOVE_STR);
		case PBS_MODE_PURGE:
		      return(PBS_MODE_PURGE_STR);
		case PBS_MODE_SETENV:
		      return(PBS_MODE_SETENV_STR);
	}
	PBS_DEBUG_UNSAFE("unkown mode: %d", mode);
	return(NULL);
}
