#
#  File: Makefile
#  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
#  Description: SAdoor client compiling rules.
#  Version: 1.0
#  Date: Mon Mar 17 20:11:03 CET 2003
#
#  Copyright (c) 2003 Claes M. Nyberg <md0claes@mdstud.chalmers.se>
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

#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
SHELL       = /bin/sh
CC          = gcc
#CC_FLAGS    = -Wall -g3  #-DSADOOR_DISABLE_ENCRYPTION
CC_FLAGS    = -Wall -O -s -I/usr/local/include -L/usr/local/include
SASH_OBJS   = sash.o net.o sadb.o utils.o iraw.o iraw_print.o random.o command.o \
              connloop.o escape.o tty.o conn.o bfish_decrypt.o bfish_encrypt.o \
			  bfish_keyinit.o bfish_ofb.o bfish_cfb.o bfish_cbc_decrypt.o sashcfg.o
SADUMP_OBJS = sadump.o utils.o net.o sadb.o bfish_keyinit.o bfish_decrypt.o \
              bfish_encrypt.o bfish_cbc_encrypt.o bfish_cbc_decrypt.o 
SACAT_OBJS  = sadbcat.o bfish_keyinit.o bfish_decrypt.o bfish_encrypt.o sadb.o \
              bfish_cbc_encrypt.o bfish_cbc_decrypt.o utils.o net.o random.o
MAN1_FILES  = man/man1/sash.1 man/man1/sacat.1 man/man1/sadump.1
MAN5_FILES  = man/man5/sash.conf.5
SASH_PROG   = sash
SADUMP_PROG = sadump
SACAT_PROG  = sadbcat
LIBS        =
SUNLIBS     = -lsocket -lnsl
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

# Root dir must end with '/' to avoid trouble ..
ROOT_DIR         = /
BIN_DIR          = ${ROOT_DIR}usr/bin
MAN_DIR          = ${ROOT_DIR}usr/share/man

# Default config directory in users home directory
CONFIG_DIR  = .sash

# Default SADB file in config directory
SADB_FILE   = sash.db

# Default config file in config directory
CONFIG_FILE = sash.conf

#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
SASH_DEFINES = -DSASH_CONFIG_DIR=\"${CONFIG_DIR}\" \
               -DSASH_SADB_FILE=\"${SADB_FILE}\" \
			   -DSASH_CONFIG_FILE=\"${CONFIG_FILE}\"

CFLAGS = ${CC_FLAGS} ${SASH_DEFINES}

MAN1_FILES   = sadbcat.1 sadump.1 sash.1
MAN5_FILES   = sash.conf.5
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

none:
	@echo
	@echo "make <systype>"
	@echo
	@echo "systypes: "
	@echo "   netbsd_x86"
	@echo "   obsd_x86 "
	@echo "   fbsd_x86 "
	@echo "   linux_x86"
	@echo "   solaris_sparc "
	@echo
	@echo "Type 'make install' for install .."
	@echo 

clean:
	rm -f *.o ${SASH_PROG} ${SADUMP_PROG} ${SACAT_PROG}

new: clean all
all: ${SASH_PROG} ${SADUMP_PROG} ${SACAT_PROG}

netbsd_x86:
	@make all

linux_x86:
	@make all

obsd_x86:
	@make CFLAGS='${CFLAGS} -DFIXIPLEN' all

fbsd_x86:
	@make all

solaris_sparc: 
	@make CFLAGS='${CFLAGS} -DWORDS_BIGENDIAN' LIBS='${SUNLIBS}' all

${SASH_PROG}: ${SASH_OBJS}
	${CC} ${CFLAGS} -o $@ ${SASH_OBJS} ${LIBS}

${SADUMP_PROG}: ${SADUMP_OBJS}
	${CC} ${CFLAGS} -o $@ ${SADUMP_OBJS} ${LIBS}

${SACAT_PROG}: ${SACAT_OBJS}
	${CC} ${CFLAGS} -o $@ ${SACAT_OBJS} ${LIBS}

install:
	@mkdir -p ${BIN_DIR}
	@chmod 0755 ${BIN_DIR}
	@chown root:0 ${SASH_PROG} ${SADUMP_PROG} ${SACAT_PROG}
	@chmod 0555 ${SASH_PROG} ${SADUMP_PROG} ${SACAT_PROG}
	cp -pf ${SASH_PROG} ${BIN_DIR}/${SASH_PROG}
	cp -pf ${SADUMP_PROG} ${BIN_DIR}/${SADUMP_PROG}
	cp -pf ${SACAT_PROG} ${BIN_DIR}/${SACAT_PROG}
	@mkdir -p ${MAN_DIR}/man5
	@mkdir -p ${MAN_DIR}/man1
	@chown root:0 ${MAN1_FILES} ${MAN5_FILES}
	@chmod 0444 ${MAN1_FILES} ${MAN5_FILES}
	cp -pf ${MAN1_FILES} ${MAN_DIR}/man1/
	cp -pf ${MAN5_FILES} ${MAN_DIR}/man5/

uninstall:
	rm -f ${BIN_DIR}/${SASH_PROG}
	rm -f ${BIN_DIR}/${SADUMP_PROG}
	rm -f ${BIN_DIR}/${SACAT_PROG}
	PWD=`pwd`; cd ${MAN_DIR}/man1/; rm -f ${MAN1_FILES}; cd ${PWD}
	PWD=`pwd`; cd ${MAN_DIR}/man5/; rm -f ${MAN5_FILES}; cd ${PWD}
