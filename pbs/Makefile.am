######################################################################
# Makefile.am                                                 May 2002
# Horms                                             horms@vergenet.net
#
# Perdition PBS
# Pop Before SMTP Tools
# Copyright (C) 2002 Horms
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307  USA
#
######################################################################/


sbin_PROGRAMS = perdition-pbs

perdition_pbs_SOURCES = \
pbs.c \
pbs_db.c pbs_db.h \
pbs_log.h \
pbs_option.c pbs_option.h \
pbs_record.c pbs_record.h 

#INCLUDES= 

# Removed because Debian doesn't like it
#LDFLAGS = -Wl,--rpath -Wl,@prefix@

LDADD = -ldb -lpopt \
-lvanessa_logger -lvanessa_socket @dmalloc_lib@
