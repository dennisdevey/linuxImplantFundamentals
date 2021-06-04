 #
 #  File: Makefile
 #  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 #  Description: SAdoor kmod (FreeBSD) compile rules
 #  Version: 1.0
 #  Date: Thu Jul  3 15:48:22 CEST 2003
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


SHELL                       = /bin/sh
CONFIGDIR                   = config
TARGET_PKTCONFIG_FILE_NAME  = sadoor.pkts.c
TARGET_PKTCONFIG_FILE       = ${CONFIGDIR}/${TARGET_PKTCONFIG_FILE_NAME}
TARGET_SADB_FILE            = sadoor.db
GETCONF_DIR                 = getconf
GETCONF_PROG                = getconf
MODULE_DIR                  = module
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#


none:
	@echo
	@echo "make <target>"
	@echo
	@echo  "Targets:"
	@echo  "     cfg  - Generate config files"
	@echo  "    kmod  - Compile module"
	@echo  "     all  - Config + module"
	@echo  "    load  - Load module"
	@echo  "  unload  - Unload module"  
	@echo

cfg:
	cd ${GETCONF_DIR}; make all; \
	./${GETCONF_PROG} ../${TARGET_PKTCONFIG_FILE} ../${TARGET_SADB_FILE}
	ln -sf ../${TARGET_PKTCONFIG_FILE} ${MODULE_DIR}/${TARGET_PKTCONFIG_FILE_NAME}

kmod:
	@echo "Making all in ${MODULE_DIR}/"
	@cd ${MODULE_DIR}; make all; 

load: cfg kmod
	@cd ${MODULE_DIR}; make load

unload:
	@cd ${MODULE_DIR}; make unload

clean:
	@cd ${GETCONF_DIR}; make clean
	@cd ${MODULE_DIR}; make clean
	rm -f ${TARGET_PKTCONFIG_FILE} 
	rm -f ${TARGET_SADB_FILE}
	rm -f ${MODULE_DIR}/${TARGET_PKTCONFIG_FILE_NAME}

all: cfg kmod
