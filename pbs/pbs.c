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
#include <locale.h>
#include <vanessa_logger.h>
#include <vanessa_socket.h>

#include "pbs_log.h"
#include "pbs_db.h"
#include "pbs_option.h"
#include "pbs_record.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#define PBS_BUFFER_SIZE 4096

vanessa_logger_t *pbs_vl;

static int pbs_file_open(const char *filename, const int retry);
static int pbs_read_line(int fd, char **current);
static int pbs_consume_line(regex_t *preg, char *line, pbs_db_t *db,
		time_t timeout, const char *prefix_len);

static void pbs_exit_handler(int sig);
static void pbs_reread_handler(int sig);

static int do_mode_list(pbs_db_t *db, const char **key, const char *prefix);
static int do_mode_insert(pbs_db_t *db, const char **key, const char *prefix,
		time_t timeout, int quiet);
static int do_mode_remove(pbs_db_t *db, const char **key, const char *prefix,
		int quiet);
static int do_mode_purge(pbs_db_t *db, int quiet);
static int do_mode_setenv(pbs_db_t *db, const char **key);

static int pbs_exit_now = 0;
static int pbs_reread_now = 0;

int main(int argc, char **argv) {
	pbs_options_t *opt;
	int fd;
	int status;
	int exit_status = -1;
	struct stat stat_buf;
	char *line = NULL;
	off_t file_size;
	off_t old_file_size;
	regex_t pregex;
	pbs_db_t *db = NULL;
	time_t now;
	time_t expire_now = 0;

	setlocale(LC_ALL, "");

	pbs_vl=vanessa_logger_openlog_filehandle(stdout, LOG_IDENT, 
		LOG_DEBUG, LOG_CONS);
	if(pbs_vl == NULL) {
		fprintf(stderr, "main: vanessa_logger_openlog_syslog\n"
			"Fatal error opening logger.  Exiting.\n");
		goto leave;
	}

	opt = pbs_options_parse(argc, argv);
	if(opt == NULL) {
		PBS_DEBUG("pbs_options_parse");	
		PBS_ERR("Fatal error parsing options. Exiting");
		goto leave;
	}

	/* Update Logger */
	if(opt->mode == PBS_MODE_DAEMON){
  		vanessa_logger_closelog(pbs_vl);
		vanessa_socket_daemon_process();
		pbs_vl=vanessa_logger_openlog_syslog_byname(
				opt->log_facility, LOG_IDENT, 
				opt->log_level, LOG_CONS);
		if(pbs_vl == NULL) {
			fprintf(stderr, 
				"main: vanessa_logger_openlog_syslog\n"
				"Fatal error opening logger. Exiting.\n");
			goto leave;
		}
	}
	else {
		vanessa_socket_daemon_inetd_process();
		vanessa_logger_change_max_priority(pbs_vl, opt->log_level);
	}

	if(opt->mode == PBS_MODE_DAEMON || 
			opt->log_level == PBS_LOG_LEVEL_DEBUG) {
		pbs_options_log(opt);
	}

	/*Set signal handlers*/
	signal(SIGHUP,    pbs_reread_handler);
	signal(SIGINT,    pbs_exit_handler);
	signal(SIGQUIT,   pbs_exit_handler);
	signal(SIGILL,    pbs_exit_handler);
	signal(SIGTRAP,   pbs_exit_handler);
	signal(SIGIOT,    pbs_exit_handler);
	signal(SIGBUS,    pbs_exit_handler);
	signal(SIGFPE,    pbs_exit_handler);
	signal(SIGUSR1,   SIG_IGN);
	signal(SIGSEGV,   pbs_exit_handler);
	signal(SIGUSR2,   SIG_IGN);
	signal(SIGPIPE,   SIG_IGN);
	signal(SIGALRM,   pbs_exit_handler);
	signal(SIGTERM,   pbs_exit_handler);
	signal(SIGCHLD,   SIG_IGN);
	signal(SIGURG,    pbs_exit_handler);
	signal(SIGXCPU,   pbs_exit_handler);
	signal(SIGXFSZ,   pbs_exit_handler);
	signal(SIGVTALRM, pbs_exit_handler);
	signal(SIGPROF,   pbs_exit_handler);
	signal(SIGWINCH,  pbs_exit_handler);
	signal(SIGIO,     pbs_exit_handler);

	if(regcomp(&pregex, opt->regex, REG_EXTENDED|REG_NEWLINE) != 0) {
		PBS_DEBUG_ERRNO("regcomp");
		PBS_ERR_UNSAFE("Fatal error compiling regular expression "
				"\"%s\". Exiting");
		goto leave;
	}

	db = pbs_db_open(opt->db_filename, 0664);
	if(db == NULL) {
		PBS_DEBUG("pbs_db_open");
		PBS_ERR_UNSAFE("Fatal error opening database \"%s\". Exiting",
				opt->db_filename);
		goto leave;
	}

	if(opt->mode == PBS_MODE_SETENV) {
		status = do_mode_setenv(db, opt->leftover);
		if(status == -2) {
			PBS_ERR("There must be at least one argument "
					"to setenv");
		}
		else if(status < 0) {
			PBS_DEBUG("do_mode_setenv");
			PBS_ERR("Fatal error doing setenv. Exiting");
		}
		goto leave;
		goto leave;
	}

	if(opt->mode == PBS_MODE_LIST) {
		if(do_mode_list(db, opt->leftover, opt->prefix) < 0) {
			PBS_DEBUG("do_mode_list");
			PBS_ERR("Fatal error listing database. Exiting");
		}
		goto leave;
	}

	if(opt->mode == PBS_MODE_INSERT) {
		status = do_mode_insert(db, opt->leftover, opt->prefix, 
					opt->timeout, 
					opt->log_level==PBS_LOG_LEVEL_QUIET);
		if(status == -2) {
			PBS_ERR("There must be at least one key to insert");
		}
		else if(status < 0) {
			PBS_DEBUG("do_mode_insert");
			PBS_ERR("Fatal error inserting entries. Exiting");
		}
		goto leave;
	}

	if(opt->mode == PBS_MODE_REMOVE) {
		status = do_mode_remove(db, opt->leftover, opt->prefix, 
					opt->log_level==PBS_LOG_LEVEL_QUIET);
		if(status == -2) {
			PBS_ERR("There must be at least one key to remove");
		}
		else if(status < 0) {
			PBS_DEBUG("do_mode_insert");
			PBS_ERR("Fatal error inserting entries. Exiting");
		}
		goto leave;
	}

	if(opt->mode == PBS_MODE_PURGE) {
		if(do_mode_purge(db, 
				opt->log_level==PBS_LOG_LEVEL_QUIET) < 0) {
			PBS_DEBUG("do_mode_purge");
			PBS_ERR("Fatal error purging database. Exiting");
		}
		goto leave;
	}

	fd = pbs_file_open(opt->log_filename, 0);
	if(fd < 0) {
		PBS_DEBUG_UNSAFE("pbs_file_open \"%s\"", opt->log_filename);	
		PBS_ERR_UNSAFE("Fatal error opening log file \"%s\". Exiting",
				opt->log_filename);
		goto leave;
	}

	if(fstat(fd, &stat_buf) < 0) {
		PBS_DEBUG_ERRNO("fstat");
		PBS_ERR("Fatal error determining log size. Exiting");
		goto leave;
	}
	file_size=stat_buf.st_size;

	while(1) {
		if(pbs_exit_now) {
			PBS_ERR_UNSAFE("Exiting on Signal %d", pbs_exit_now);
			goto leave;
		}

		now = time(NULL);
		if(now > expire_now) {
			if(pbs_db_traverse(db, 
					pbs_db_traverse_func_expire_record, 
					&now) < 0) {
				PBS_DEBUG("pbs_db_traverse "
						"pbs_db_expire_record");

				PBS_ERR("Fatal error expiring keys from "
						"database. Exiting");
				goto leave;
			}
			expire_now = now + opt->timeout / 10;
		}

		if(fstat(fd, &stat_buf) < 0) {
			PBS_DEBUG_ERRNO("fstat");
			PBS_ERR("Fatal error determining log size. "
					"Exiting");
			goto leave;
		}
		old_file_size = file_size;
		file_size=stat_buf.st_size;

		if(pbs_reread_now || old_file_size > file_size) {
			PBS_INFO_UNSAFE("Reopening log file \"%s\"", 
					opt->log_filename);
			if(close(fd) < 0) {
				PBS_DEBUG_ERRNO("close");
				PBS_ERR("Fatal error closing log file. "
						"Exiting");
				goto leave;
			}
			fd =pbs_file_open(opt->log_filename, 1);
			if(fd < 0) {
				PBS_DEBUG("pbs_file_open");	
				PBS_ERR("Fatal error opening log file. "
						"Exiting");
				goto leave;
			}
			pbs_reread_now = 0;
			continue;
		}

		if(old_file_size == file_size) {
			sleep(1);
			continue;
		}
			
		while(1) {
			status = pbs_read_line(fd, &line);
			if(status < 0) {
				PBS_DEBUG("pbs_read_line");
				PBS_ERR("Fatal error reading log file. "
						"Exiting");
				goto leave;
			}
			if(status == 0) {
				break;
			}
			if(status == 1) {
				continue;
			}
			if(pbs_consume_line(&pregex, line, db, opt->timeout,
					opt->prefix) < 0) {
				PBS_DEBUG("pbs_consume_line");
				PBS_ERR("Fatal error processing line from "
						"log file. Exiting");
				goto leave;
			}
			free(line);
			line = NULL;
		}
	}

	exit_status = 0;
leave:
	if(db != NULL) {
		pbs_db_close(db);
	}
	return(exit_status);
}

int pbs_file_open(const char *filename, const int retry) {
	int fd = -1;
	int pause=2;
	int loop = 1;

	while(loop) {
		fd = open(filename, O_RDONLY);
		if(fd >=0 ) {
			break;
		}

		PBS_DEBUG_ERRNO("open");	
		if((errno != EACCES && errno != ENOENT && errno != ELOOP)
				|| !retry) {
			return(-1);
		}
		
		PBS_ERR_UNSAFE("Transient error opening file, will retry "
				"in %d seconds\n", pause);	
		sleep(pause);
		if(pbs_exit_now) {
			/* An exit signal has been recieved, just
			 * return a dummy file handle, as it won't get
			 * used anyway */
			return(0);
		}
		pause = (pause*2)%65;
	}

	if(lseek(fd, 0, SEEK_END) < 0) {
		PBS_DEBUG_ERRNO("fstat");
		return(-1);
	}

	return(fd);
}


static char __pbs_read_line_buffer[PBS_BUFFER_SIZE];
static ssize_t __pbs_read_line_offset = 0;
static ssize_t __pbs_read_line_bytes = 0;

static ssize_t fill_buffer(int fd, char **buffer) {
	if(__pbs_read_line_bytes > 0) {
		*buffer=__pbs_read_line_buffer+__pbs_read_line_offset;
		return(__pbs_read_line_bytes);
	}
	memset(__pbs_read_line_buffer, '\0', PBS_BUFFER_SIZE);
	__pbs_read_line_bytes = read(fd, __pbs_read_line_buffer, 
			PBS_BUFFER_SIZE-1);
	if(__pbs_read_line_bytes < 0) {
		PBS_DEBUG_ERRNO("read");
		return(-1);
	}
	__pbs_read_line_offset = 0;
	*buffer=__pbs_read_line_buffer;

	return(__pbs_read_line_bytes);
}

static int pbs_read_line(int fd, char **current) {
	ssize_t bytes;
	size_t cpy_length;
	size_t current_length;
	size_t alloc_length;
	char *this_line;
	char *next_line;
	char c;
	
	bytes = fill_buffer(fd, &this_line);
	if(bytes < 0) {
		PBS_DEBUG("fill_buffer");
		return(-1);
	}
	if(bytes == 0) {
		return(0);
	}

	next_line = strchr(this_line, '\n');
	cpy_length = (next_line == NULL)?bytes:next_line-this_line;
	current_length = (*current == NULL)?0:strlen(*current);
	alloc_length = cpy_length+current_length;
	
	*current = realloc(*current, alloc_length + 1);
	if(*current == NULL) {
		PBS_DEBUG_ERRNO("realloc");
		return(-1);
	}

	memset(*current+current_length, '\0', cpy_length + 1);
	memcpy(*current+current_length, this_line, cpy_length);

	__pbs_read_line_offset += cpy_length;
	__pbs_read_line_bytes -= cpy_length;
	while(1) {
		c = *(__pbs_read_line_buffer+__pbs_read_line_offset);
		if(c == '\n' || c == '\r') {
			__pbs_read_line_offset++;
			__pbs_read_line_bytes--;
		}
		else {
			break;
		}
	}

	return((next_line == NULL)?1:2);
}

#define PBS_NMATCH 3
static int pbs_consume_line(regex_t *preg, char *line, pbs_db_t *db,
		time_t timeout, const char *prefix){
	regmatch_t pmatch[PBS_NMATCH];
	char *user = NULL;
	char *ip = NULL;
	size_t len;
	size_t prefix_len;
	time_t expire;
	int status = 0;

	if(regexec(preg, line, PBS_NMATCH, pmatch, 0) != 0) {
		printf("consume: no match\n");
		return(-1);
	}
	
	if(pmatch[1].rm_so == -1 || pmatch[1].rm_eo == -1 ||
			pmatch[1].rm_so >= pmatch[1].rm_eo) {
		PBS_DEBUG("ip not found");
		return(-1);
	}
	len = pmatch[1].rm_eo - pmatch[1].rm_so;
	prefix_len=(prefix != NULL)?strlen(prefix):0;
	ip = malloc(len + +prefix_len + 1);
	if(ip == NULL) {
		PBS_DEBUG_ERRNO("malloc ip");
		return(-1);
	}
	memset(ip, '\0', len + prefix_len + 1);
	if(prefix != NULL) {
		memcpy(ip, prefix, prefix_len);
	}
	memcpy(ip + prefix_len, line+pmatch[1].rm_so, len);

	if(pmatch[2].rm_so != -1 && pmatch[2].rm_eo != -1 &&
			pmatch[2].rm_so < pmatch[2].rm_eo) {
		len = pmatch[2].rm_eo - pmatch[2].rm_so;
		user = malloc(len + 1);
		if(user == NULL) {
			free(ip);
			PBS_DEBUG_ERRNO("malloc user");
			return(-1);
		}
		memset(user, '\0', len+1);
		memcpy(user, line+pmatch[2].rm_so, len);
	}

	expire = time(NULL) + timeout;
	if(pbs_db_put(db, ip, strlen(ip)+1, &expire, sizeof(expire)) < 0) {
		PBS_DEBUG("pbs_db_put");
		status = -1;
	}

	PBS_INFO_UNSAFE("Added: \"%s\" for \"%s\"\n", ip, user);

	free(ip);
	free(user);
	return(status);
}

static void pbs_exit_handler(int sig) {
	pbs_exit_now = sig;
}

static void pbs_reread_handler(int sig) {
	pbs_reread_now = sig;
}

static int do_mode_list(pbs_db_t *db, const char **key, const char *prefix) {
	size_t width;
	time_t *time;
	size_t len;
	size_t buf_len = 0;
	const char **k;
	const char *k_fixed;
	char *buf = NULL;

	width = pbs_key_width(prefix);

	if(pbs_record_top(width) < 0) {
		PBS_DEBUG("pbs_record_hrule");
		return(-1);
	}

	if(key == NULL) {
		if(pbs_db_traverse(db, pbs_db_traverse_func_show_record, 
					&width) < 0) {
			PBS_DEBUG("pbs_db_traverse pbs_db_show_record");
			return(-1);
		}
		return(0);
	}


	for(k = key ; *k != NULL; k++) {
		k_fixed = pbs_record_fix_key(*k, prefix, &buf, &buf_len);
		if(k_fixed == NULL) {
			PBS_DEBUG("pbs_record_fix_key");
			return(-1);
		}
		pbs_db_get(db, (char *)k_fixed, strlen(k_fixed)+1, 
				(void **)&time, &len);
		if(time == NULL || len == 0) {
			pbs_record_show_str((char *)*k, "Not found", width);
		}
		else {
			pbs_record_show((char *)*k, *time, width);
		}
	}

	if(buf != NULL) {
		free(buf);
	}

	return(0);
}



static int do_mode_remove(pbs_db_t *db, const char **key, const char *prefix,
		int quiet) {
	size_t width;
	size_t buf_len = 0;
	const char **k;
	const char *k_fixed;
	char *buf = NULL;
	char *response;

	if(key == NULL) {
		PBS_DEBUG("no key");
		return(-2);
	}

	width = pbs_key_width(prefix);

	if(!quiet && pbs_record_top(width) < 0) {
		PBS_DEBUG("pbs_record_hrule");
		return(-1);
	}

	for(k = key ; *k != NULL; k++) {

		k_fixed = pbs_record_fix_key(*k, prefix, &buf, &buf_len);
		if(k_fixed == NULL) {
			PBS_DEBUG("pbs_record_fix_key");
			return(-1);
		}
		if(pbs_db_del(db, (char *)k_fixed, strlen(k_fixed)+1) < 0) {
			response = "Not found";
		}
		else {
			response = "Deleted";
		}
		if(!quiet) {
			pbs_record_show_str((char *)*k, response, width);
		}
	}

	if(buf != NULL) {
		free(buf);
	}

	return(0);
}


static int do_mode_insert(pbs_db_t *db, const char **key, const char *prefix,
		time_t timeout, int quiet) {
	size_t width;
	time_t expire;
	size_t buf_len = 0;
	const char **k;
	const char *k_fixed;
	char *buf = NULL;

	if(key == NULL) {
		PBS_DEBUG("no key");
		return(-2);
	}

	width = pbs_key_width(prefix);

	if(!quiet && pbs_record_top(width) < 0) {
		PBS_DEBUG("pbs_record_hrule");
		return(-1);
	}

	for(k = key ; *k != NULL; k++) {

		k_fixed = pbs_record_fix_key(*k, prefix, &buf, &buf_len);
		if(k_fixed == NULL) {
			PBS_DEBUG("pbs_record_fix_key");
			return(-1);
		}
		expire = time(NULL) + timeout;
		if(pbs_db_put(db, (char *)k_fixed, strlen(k_fixed)+1,
					&expire, sizeof(expire)) < 0) {
			PBS_DEBUG("pbs_db_put");
			return(-1);
		}
		if(!quiet) {
			pbs_record_show((char *)*k, expire, width);
		}
	}

	if(buf != NULL) {
		free(buf);
	}

	return(0);
}


static int do_mode_purge(pbs_db_t *db, int quiet) {
	time_t expire;

	expire = ~0;

	if(pbs_db_traverse(db, pbs_db_traverse_func_expire_record, 
				&expire) < 0) {
		PBS_DEBUG("pbs_db_traverse "
				"pbs_db_expire_record");
		return(-1);
	}

	if(!quiet) {
		printf("Purged database\n");
	}

	return(0);
}


static int do_mode_setenv(pbs_db_t *db, const char **key) {
	char value = '\0';

	PBS_ERR("You need to check the source IP address!!");

	if(key == NULL) {
		PBS_DEBUG("no key");
		return(-2);
	}

	setenv("RELAYCLIENT", &value, 0);
	execv(key[0], (char **const)key);
	PBS_DEBUG_ERRNO("execl");	
	return(-1);
}
