#
#  File: Makefile.devel
#  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
#  Description: SAdoor developer makefile
#  Version: 1.0
#  Date: Mon Mar 17 12:10:52 CET 2003
#
#  Copyright (c) 2002 Claes M. Nyberg <md0claes@mdstud.chalmers.se>
#  All rights reserved, all wrongs reversed.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. The name of author may not be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
#  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
#  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
#  THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

SHELL       = /bin/sh
CC          = gcc
#CC_FLAGS    = -Wall -g3 -pipe -I/usr/local/include/ -L/usr/local/lib/
CC_FLAGS    = -Wall -O -s -I/usr/local/include/ -L/usr/local/lib
SADOOR_OBJS = sapc_lexer.o sapc_parser.o sadc.o utils.o net.o sadoor.o log.o replay.o \
              capture.o daemon.o command.o conn.o connloop.o sapty.o bfish_cfb.o \
			  bfish_decrypt.o bfish_encrypt.o bfish_keyinit.o bfish_ofb.o random.o
MKSADB_OBJS = mksadb.o sadc.o sapc_lexer.o sapc_parser.o utils.o net.o log.o \
              sadb_writeraw.o
LIBS        = -lpcap
SUNLIBS     = -lsocket -lnsl
SADOOR_PROG = sadoor
MKSADB_PROG = mksadb
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

# Root dir must end with '/' to avoid trouble ..
ROOT_DIR         = /
SBIN_DIR         = ${ROOT_DIR}usr/sbin
BIN_DIR          = ${ROOT_DIR}usr/bin
MAN_DIR          = ${ROOT_DIR}usr/share/man

# Default config dir
CONFIG_DIR       = ${ROOT_DIR}etc/sadoor
#CONFIG_DIR =  /mastercom.cmn/sadoor/skel/

# Default config file
CONFIG_FILE      = ${CONFIG_DIR}/sadoor.conf

# Default packet config file
PKTCONFIG_FILE   = ${CONFIG_DIR}/sadoor.pkts

# Default key file
KEY_FILE         = ${CONFIG_DIR}/sadoor.key

# Default private log file
PRIVLOG          = ${CONFIG_DIR}/sadoor.log

# Default file to write SADB entry to
SADBFILE         = ${CONFIG_DIR}/sadoor.db

# Deafult PID file
PID_FILE         = ${ROOT_DIR}var/run/sadoor.pid

# Default syslog facility
SYSLOG_FACILITY  = LOG_DAEMON

# Default syslog priority
SYSLOG_PRIORITY  = LOG_INFO

#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
SADOOR_DEFINES = -DSADOOR_CONFIG_DIR=\"${CONFIG_DIR}\" \
                 -DSADOOR_CONFIG_FILE=\"${CONFIG_FILE}\" \
				 -DSADOOR_PKTCONFIG_FILE=\"${PKTCONFIG_FILE}\" \
				 -DSADOOR_KEY_FILE=\"${KEY_FILE}\" \
				 -DSADOOR_PRIVLOG=\"${PRIVLOG}\" \
				 -DSADOOR_SADBFILE=\"${SADBFILE}\" \
				 -DSADOOR_PID_FILE=\"${PID_FILE}\" \
				 -DSADOOR_SYSLOG_FACILITY=${SYSLOG_FACILITY} \
				 -DSADOOR_SYSLOG_PRIORITY=${SYSLOG_PRIORITY}

CFLAGS = ${CC_FLAGS} ${SADOOR_DEFINES} #-DSADOOR_DISABLE_ENCRYPTION
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
SKEL_DIR       = ./
SKEL_CONFIG    = ${SKEL_DIR}/sadoor.conf
SKEL_PKTCONF   = ${SKEL_DIR}/sadoor.pkts

MANSRCDIR      = ./
MAN5_FILES     = sadoor.conf.5 sadoor.key.5 sadoor.pkts.5
MAN8_FILES     = mksadb.8 sadoor.8
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

none: 
	@echo
	@echo "Usage: make <systype>"
	@echo
	@echo "systypes: "
	@echo "     netbsd_x86"
	@echo "     obsd_x86"
	@echo "     fbsd_x86"
	@echo "     linux_x86"
	@echo "     solaris_sparc"
	@echo
	@echo "Type 'make install' for install .."
	@echo

all: ${SADOOR_PROG} ${MKSADB_PROG}

clean:
	rm -f *.o ${SADOOR_PROG} ${MKSADB_PROG}

netbsd_x86:
	@make SUNLIBS="" all

obsd_x86: 
	@make SUNLIBS="" all

linux_x86: 
	@make SUNLIBS="" all

fbsd_x86:
	@make SUNLIBS="" all

solaris_sparc: 
	@make CFLAGS='${CFLAGS} -DWORDS_BIGENDIAN -DSYSVREL4PTY' all

${SADOOR_PROG}: ${SADOOR_OBJS}
	${CC} ${CFLAGS} -o $@ ${SADOOR_OBJS} ${LIBS} ${SUNLIBS}

${MKSADB_PROG}: ${MKSADB_OBJS}
	${CC} ${CFLAGS} -o $@ ${MKSADB_OBJS} ${SUNLIBS}

# install(1) is not portable and this crap works
install:
	#strip ${SADOOR_PROG} ${MKSADB_PROG}
	@mkdir -p ${CONFIG_DIR}
	@chmod 0700 ${CONFIG_DIR}
	cp -i ${SKEL_CONFIG} ${CONFIG_FILE}
	@chown root:0 ${CONFIG_FILE}
	@chmod 600 ${CONFIG_FILE}
	cp -i ${SKEL_PKTCONF} ${PKTCONFIG_FILE}
	@chown root:0 ${PKTCONFIG_FILE}
	@chmod 600 ${PKTCONFIG_FILE}
	@mkdir -p ${SBIN_DIR}
	@mkdir -p ${BIN_DIR}
	cp -f ${SADOOR_PROG} ${SBIN_DIR}/${SADOOR_PROG}
	@chown root:0 ${SBIN_DIR}/${SADOOR_PROG}
	@chmod 550 ${SBIN_DIR}/${SADOOR_PROG}
	cp -f ${MKSADB_PROG} ${BIN_DIR}/${MKSADB_PROG}
	@chown root:0 ${BIN_DIR}/${MKSADB_PROG}
	@chmod 0555 ${BIN_DIR}/${MKSADB_PROG}
	@mkdir -p ${MAN_DIR}/man5
	@mkdir -p ${MAN_DIR}/man8
	PWD=`pwd`; cd ${MANSRCDIR}; \
	chown root:0 ${MAN5_FILES} ${MAN8_FILES}; \
	chmod 444 ${MAN5_FILES} ${MAN8_FILES}; \
	cp -pf ${MAN5_FILES} ${MAN_DIR}/man5/; \
	cp -pf ${MAN8_FILES} ${MAN_DIR}/man8/; \
	cd ${PWD}
	
uninstall:
	rm ${SBIN_DIR}/${SADOOR_PROG}
	rm ${BIN_DIR}/${MKSADB_PROG}
	rm ${PKTCONFIG_FILE}
	rm ${CONFIG_FILE}
	PWD=`pwd`; cd ${MAN_DIR}/man5/; rm -f ${MAN5_FILES}; cd ${PWD}
	PWD=`pwd`; cd ${MAN_DIR}/man8/; rm -f ${MAN8_FILES}; cd ${PWD}
	rmdir ${CONFIG_DIR}
