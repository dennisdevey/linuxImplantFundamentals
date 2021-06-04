/*
 *  File: sapc_parser.c
 *  Author: Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  Description: SAdoor packet config parser/typecheck routines.
 *  Version: 1.0
 *  Date: Fri Jan 10 22:23:47 CET 2003
 *
 *  Copyright (c) 2003 Claes M. Nyberg <md0claes@mdstud.chalmers.se>
 *  All rights reserved, all wrongs reversed.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 *  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 *  THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include "sapc.h"
#include "utils.h"
#include "net.h"

/* For mmap() */
#ifndef MAP_FILE
#define MAP_FILE    0
#endif

#define DATA_BUFSIZ     256        /* Size of initial data buffr size */

/* Free memory allocated for one sa_pkt structure */
#define FREE_SAPKT(__packet) \
    if ((__packet) != NULL) { \
        if ((__packet)->sa_pkt_ph != NULL) {\
            if (SAPKT_PROTO((__packet)) == SAPKT_PROTO_TCP) {\
                if ( ((struct sa_tcph *)((__packet)->sa_pkt_ph))->sa_tcph_data != NULL) \
                    free(((struct sa_tcph *)((__packet)->sa_pkt_ph))->sa_tcph_data); \
            }\
            else if (SAPKT_PROTO((__packet)) == SAPKT_PROTO_UDP) {\
                if ( ((struct sa_udph *)((__packet)->sa_pkt_ph))->sa_udph_data != NULL) \
                    free(((struct sa_udph *)((__packet)->sa_pkt_ph))->sa_udph_data); \
            }\
            else if (SAPKT_PROTO((__packet)) == SAPKT_PROTO_ICMP) {\
                if ( ((struct sa_icmph *)((__packet)->sa_pkt_ph))->sa_icmph_data != NULL) \
                    free(((struct sa_icmph *)((__packet)->sa_pkt_ph))->sa_icmph_data); \
            }\
            free((__packet)->sa_pkt_ph); \
        } \
        free(__packet); \
    }

/* Macro used by parse_arr() below, free's allocated memory and returns NULL
 * if strings does not match */
#define PARR_MATCH_OR_DIE(__s1, __expected) \
    if (!MATCH((__s1), (__expected))) { \
        parse_err("Expected '%s', received '%s'\n", \
            __expected, ((__s1) == NULL) ? (u_char *)"EOF" : (__s1)); \
        free_pkts(pkts); \
        pkts = NULL; \
        errors++; \
        return(NULL); \
    } \
    else { \
        strarr++; \
    }

/* Macro used by parse_packet below, free's allocated memory and returns NULL
 * if strings does not match */
#define PPKT_MATCH_OR_DIE(__s1, __expected) \
    if (!MATCH((__s1), (__expected))) { \
        parse_err("Expected '%s', received '%s'\n", \
            __expected, ((__s1) == NULL) ? (u_char *)"EOF" : (__s1)); \
        FREE_SAPKT(pkt); \
        errors++; \
        errors += pkterr; \
        return(NULL); \
    } \
    else { \
        strarr++; \
    }

/* Macro used by the header parser routines, prints error message
 * and returns __retpt if strings do not match */
#define PHDR_MATCH_OR_DIE(__s1, __expected, __ret) \
    if (!MATCH((__s1), (__expected))) { \
        parse_err("Expected '%s', received '%s'\n", \
            __expected, ((__s1) == NULL) ? (u_char *)"EOF" : (__s1)); \
            errors++; \
            return(__ret); \
    }\
    else {\
        strarr++; \
    }

/* Local functions */
static struct sa_pkts *parse_arr(void);
static void parse_err(const u_char *fmt, ...);
static struct sa_pkt *parse_packet(void);
static void *parse_tcph(void);
static void *parse_udph(void);
static void *parse_icmph(void);
static u_char *parse_str(u_char **, u_short *);
static int parse_tcpflags(void);

/* Local variables */
static size_t pkt_count;    /* Packet counter */
static size_t errors;    
static u_char **strarr;        /* Token pointers */

/*
 * Memory map the packet config file and parse it
 * (typecheck is performed at the same time).
 * Arguments: 
 *  path - The path to the packet config file.
 * Returns NULL on error.
 */
struct sa_pkts *
sapc_getpkts(u_char *path)
{
    u_char **arr;
    struct sa_pkts *sapkts;
    int fd;          
    caddr_t mmad;   
    struct stat sb;

    /* Open file */
    if( (fd = open(path, O_RDONLY)) < 0) {
        fprintf(stderr, "sapc_parse_pktconf(): open(%s): %s\n", path,
            strerror(errno));
        return(NULL);
    }

    /* Get size of file */
    if (fstat(fd, &sb) < 0) {
        fprintf(stderr, "sapc_parse_pktconf(): fstat(): %s\n", 
            strerror(errno));
        return(NULL);
    }

    if (!sb.st_size) {
        fprintf(stderr, "sapc_parse_pktconf(): %s file is empty!\n", path);
        return(NULL);
    }

    /* Memory-map file */
    if ( (mmad = mmap(0, sb.st_size +2, PROT_READ, MAP_FILE | MAP_SHARED,
            fd, 0)) == MAP_FAILED) {
        fprintf(stderr, "sapc_parse_pktconf(): mmap(): %s\n",
            strerror(errno));
        close(fd);
        return(NULL);
    }

    /* Do a lexical analyze of the file contents according to syntax */
    arr = strarr = sapc_lexer((u_char *)mmad);
    munmap(mmad, sb.st_size);
    close(fd);

    /* Lexical analyze failed */
    if (strarr == NULL)
        return(NULL);

    /* Parse the file to retrieve the packets */
    sapkts = parse_arr();

    /* Free token-array */
    if (arr != NULL) {
        u_char **tmp = arr;

        while (*tmp != NULL) 
            free(*(tmp++));

        free(arr);
        strarr = NULL;
    }
    return(sapkts);
}

/*
 * Parse the strings according to syntax and build the packets.
 * Returns NULL on error and a pointer to a sa_pkts structure 
 * on success.
 */
static struct sa_pkts *
parse_arr(void)
{
    struct sa_pkts *pkts;
    struct sa_pkt *tail = NULL;
    struct sa_pkt *current;
    long repl = 0;
    
    errors = 0;
    pkt_count = 1;
    
    pkts = (struct sa_pkts *)calloc(1, sizeof(struct sa_pkts));
    if (pkts == NULL) {
        parse_err("%s", strerror(errno));
        return(NULL);
    }
    
    /* Get all key packets, at least one is required */
    while ((*strarr != NULL) && !MATCH(*strarr, CMDPKT_TOK)) {

        PARR_MATCH_OR_DIE(*strarr, KEYPKT_TOK);
    
        /* Get the number of times this packet 
         * should be replicated, if any. */
        if (MATCH(*strarr, REPL_BEGIN)) {
            strarr++;
            
            if (!strisnum(*strarr, &repl)) {
                parse_err("Bad replicate value\n"); 
                    return(NULL);
            }
            
            if (!ISu16(repl)) {
                parse_err("Unsigned 16 bit decimal expected for repliacte value,"
                    " received '%s'\n", *strarr);
                    return(NULL);
            }
            strarr++;
            
            PARR_MATCH_OR_DIE(*strarr, REPL_END);
        }

        PARR_MATCH_OR_DIE(*strarr, BLOCK_BEGIN);
        PARR_MATCH_OR_DIE(*strarr, IP_TOK);
        PARR_MATCH_OR_DIE(*strarr, BLOCK_BEGIN);

        if ( (current = parse_packet()) != NULL) {
    
            if (pkts->key_pkts == NULL) 
                pkts->key_pkts = current;
        
            if (tail == NULL) 
                tail = pkts->key_pkts;
        
            else {
                tail->sa_kpkt_next = current;
                tail = current;
            }
        }
        else {
            errors++;
            repl = 0;
        }

        /* End of 'keypkt' */
        PARR_MATCH_OR_DIE(*strarr, BLOCK_END);

        pkt_count++;

        /* Replicate this packet */
        while (repl > 0) {

            if ( (current = calloc(1, sizeof(struct sa_pkt))) == NULL) {
                fprintf(stderr, "Error: calloc(): %s\n", strerror(errno));
                free_pkts(pkts);
                return(NULL);
            }
            
            memcpy(current, tail, sizeof(struct sa_pkt));
            
            tail->sa_kpkt_next = current;
            tail = current;
            pkt_count++;
            repl--;
        }
    }
    
    if ((pkts->key_pkts == NULL)  && (errors == 0)) {
        fprintf(stderr, "Syntax error: At least one key packet is required.\n");
        free_pkts(pkts);
        return(NULL);
    }

    if (*strarr == NULL) {
        fprintf(stderr, "Syntax error: Command packet required last in file.\n");
        free_pkts(pkts);
        return(NULL);
    }

    PARR_MATCH_OR_DIE(*strarr, CMDPKT_TOK);
    PARR_MATCH_OR_DIE(*strarr, BLOCK_BEGIN);
    PARR_MATCH_OR_DIE(*strarr, IP_TOK);
    PARR_MATCH_OR_DIE(*strarr, BLOCK_BEGIN);

    if ( (pkts->cmd_pkt = parse_packet()) == NULL) 
        errors++;

    PARR_MATCH_OR_DIE(*strarr, BLOCK_END);

    if ( (pkts->cmd_pkt != NULL) &&
            (pkts->cmd_pkt->sa_pkt_proto == SAPKT_PROTO_ICMP)) {
        struct sa_icmph *icp = (struct sa_icmph *)pkts->cmd_pkt->sa_pkt_ph;
                                    
        if (icp->sa_icmph_type != 0 && icp->sa_icmph_type != 8) {
            fprintf(stderr, "Type Error: Only ICMP type 0 and 8"
                " supported in command packet\n");
            errors++;
        }
    }

    if ((errors == 0) && (*strarr != NULL)) {
        fprintf(stderr, "Syntax Error: Garbage at end of packet "
            "config file\n");
        errors++;
    }

    if (errors != 0) {
        free_pkts(pkts);
        pkts = NULL;
    }
    else 
        pkts->sa_pkts_count = (u_int)pkt_count;
    
    return(pkts);
}


/*
 * Print error message
 */
static void
parse_err(const u_char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "Syntax error in packet %u: ", pkt_count);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

/*
 * Parses IP header and calls the correct function 
 * for parsing the underlaying protocol.
 * Returns a pointer to a sa_pkt structure 
 * on success and NULL on error.
 */
static struct sa_pkt *
parse_packet(void)
{
    struct sa_pkt *pkt;
    size_t pkterr = 0;
    u_char field;
    void *(*phf)() = NULL; /* Function pointer to protocol parser */


    if ( (pkt = calloc(1, sizeof(struct sa_pkt))) == NULL) {
        fprintf(stderr, "parse_packet(): calloc(): %s\n", strerror(errno));
        return(NULL);
    }

    
    while ((*strarr != NULL) && !MATCH(*strarr, BLOCK_END)) {
        field = 0;
    
        /* Source address */
        if (MATCH(*strarr, IP_SADDR_TOK)) 
            field = IPH_SADDR;

        /* Destination address */
        else if (MATCH(*strarr, IP_DADDR_TOK)) 
            field = IPH_DADDR;

        /* Type of service */
        else if (MATCH(*strarr, IP_TOS_TOK)) 
            field = IPH_TOS;

        /* ID */
        else if (MATCH(*strarr, IP_ID_TOK)) 
            field = IPH_ID;

        /* TTL */
        else if (MATCH(*strarr, IP_TTL_TOK)) 
            field = IPH_TTL;
    
        /* Parse TCP */
        else if (MATCH(*strarr, TCP_TOK)) {
            phf = (void *)parse_tcph;
            pkt->sa_pkt_proto = SAPKT_PROTO_TCP;
        }

        /* Parse UDP */
        else if (MATCH(*strarr, UDP_TOK)) {
            phf = (void *)parse_udph;
            pkt->sa_pkt_proto = SAPKT_PROTO_UDP;
        }

        /* Parse ICMP */
        else if (MATCH(*strarr, ICMP_TOK)) {
            phf = (void *)parse_icmph;
            pkt->sa_pkt_proto = SAPKT_PROTO_ICMP;
        }
        
        /* Bad token */
        else {
            fprintf(stderr, "Syntax Error: Unrecognized key word "
                "'%s' in IP header of packet %u\n", *strarr, pkt_count);
            FREE_SAPKT(pkt);
            return(NULL);
        }

        /* Set field value */
        if (field != 0) {
            u_char typerr = 0;
            u_char *fieldtok = *strarr;
            long val;
        
            if (FIELD_IS_SET(pkt->sa_iph_fields, field)) {
                fprintf(stderr, "Syntax Error: Multiple entries for "
                    "'%s' in IP header of packet %u\n", *strarr, pkt_count);
                pkterr++;
            }

            strarr++;
            PPKT_MATCH_OR_DIE(*strarr, ASSIGN);
            
            if (field == IPH_SADDR) {
                
                if (!isip(*strarr))
                    typerr++;
                else
                    pkt->sa_iph_saddr = net_inetaddr(*strarr);
            }
                    
            else if (field == IPH_DADDR) {
                
                if (!isip(*strarr))
                    typerr++;
                else
                    pkt->sa_iph_daddr = net_inetaddr(*strarr);
            }
                
            else if (!strisnum(*strarr, &val))
                    typerr++;

            else {
                switch (field) {
                        
                    case IPH_TOS:
                        if (!ISu8(val))
                            typerr++;
                        pkt->sa_iph_tos = (u_char)val;
                        break;

                    case IPH_TTL:
                        if (!ISu8(val))
                            typerr++;
                        pkt->sa_iph_ttl = (u_char)val;
                        break;
                            
                    case IPH_ID:
                        if (!ISu16(val))
                            typerr++;
                        pkt->sa_iph_id = htons((u_short)val);
                        break;
                }
            }
            
            if (typerr != 0) {
                fprintf(stderr, "Type Error: Bad value for '%s' in "
                    "IP header of packet %u\n", fieldtok, pkt_count);
                pkterr++;
            }
            else
                pkt->sa_iph_fields |= field;

            strarr++;
            PPKT_MATCH_OR_DIE(*strarr, CMD_END);
        }
        /* Parse protocol */
        else {
            size_t tmperr;
        
            if (pkt->sa_pkt_ph != NULL) {
                fprintf(stderr, "Syntax Error: Multiple sub protocols in packet %u\n",
                    pkt_count);
                pkterr++;
                break;    
            }
        
            strarr++;
            PPKT_MATCH_OR_DIE(*strarr, BLOCK_BEGIN);
            
            /* Since a empty header is valid syntax we need to check
             * if the number of errors has increased to fetch any error. */
            tmperr = errors;
            
            pkt->sa_pkt_ph = phf();

            if (tmperr != errors)
                pkterr++;
        }

    }

    /* A destinaton address is required for each packet */
    if (!FIELD_IS_SET(pkt->sa_iph_fields, IPH_DADDR)) {
        fprintf(stderr, "Syntax Error: Destination address missing in packet %u\n",
            pkt_count);
        pkterr++;
    }

    /* A protocol is required */
    if (pkt->sa_pkt_proto == 0) {
        fprintf(stderr, "Syntax Error: Protocol missing in packet %u\n",
            pkt_count);
        pkterr++;
    }

    if (pkterr != 0) {
        FREE_SAPKT(pkt);
        pkt = NULL;
        errors += pkterr;
    }

    PPKT_MATCH_OR_DIE(*strarr, BLOCK_END);
    return(pkt);
}


/*
 * Parse TCP header, returns a pointer to a sa_tcph 
 * structure on success and NULL on error.
 */
static void *
parse_tcph(void)
{
    struct sa_tcph *tcph = NULL;
    long val;
    u_char field;
    u_char *fieldtok;
    size_t typerr;


    if ( (tcph = (struct sa_tcph *)calloc(1, sizeof(struct sa_tcph))) == NULL) {
        fprintf(stderr, "Error: parse_tcph(): calloc(): %s\n", strerror(errno));
        return(NULL);
    }

    while ((*strarr != NULL) && !MATCH(*strarr, BLOCK_END)) {

        field = 0;
        typerr = 0;

        if (MATCH(*strarr, TCP_DPORT_TOK))
            field = TCPH_DPORT;

        else if (MATCH(*strarr, TCP_SPORT_TOK))
            field = TCPH_SPORT;

        else if (MATCH(*strarr, TCP_SEQ_TOK))
            field = TCPH_SEQ;
            
        else if (MATCH(*strarr, TCP_ACK_TOK))
            field = TCPH_ACK;
        
        else if (MATCH(*strarr, TCP_FLAGS_TOK))
            field = TCPH_FLAGS;

        else if (MATCH(*strarr, DATA_TOK)) {
            
            strarr++;
            PHDR_MATCH_OR_DIE(*strarr, BLOCK_BEGIN, tcph);

            if ( (parse_str(&(tcph->sa_tcph_data), &(tcph->sa_tcph_dlen))) == NULL)
                errors++;
            
            continue;
        }


         else {
            fprintf(stderr, "Syntax Error: Unrecognized key word "
                "'%s' in TCP header of packet %u\n", *strarr, pkt_count);
            errors++;
            return((void *)tcph);
        }

        if (FIELD_IS_SET(tcph->sa_tcph_fields, field)) {
            fprintf(stderr, "Syntax Error: Multiple entries for "
                "'%s' in TCP header of packet %u\n", *strarr, pkt_count);
            errors++;
        }
        else 
            tcph->sa_tcph_fields |= field;    

        fieldtok = *strarr;
        strarr++;

        PHDR_MATCH_OR_DIE(*strarr, ASSIGN, tcph);

        if (field == TCPH_FLAGS) {

            if ( (val = parse_tcpflags()) == -1)
                typerr++;
            
            strarr--;
    
            if (!ISu6(val))
                typerr++;

            tcph->sa_tcph_flags = (u_char)val;
        }
        else if (!strisnum(*strarr, &val))
            typerr++;
    
        if (field == TCPH_DPORT) {
            if (!ISu16(val))
                typerr++;
            tcph->sa_tcph_dport = htons((u_short)val);
        }
        
        else if (field == TCPH_SPORT) {
            if (!ISu16(val))
                typerr++;
            tcph->sa_tcph_sport = htons((u_short)val);
        }
        
        else if (field == TCPH_SEQ)
            tcph->sa_tcph_seq = htonl(val);
        
        else if (field == TCPH_ACK)
            tcph->sa_tcph_ack = htonl(val);
        

        if (typerr != 0) {
            fprintf(stderr, "Type Error: Bad value for '%s' in TCP header of packet %u\n",
                fieldtok, pkt_count);
                            
            errors += typerr;
        }

        strarr++;
        PHDR_MATCH_OR_DIE(*strarr, CMD_END, tcph);
    }

    PHDR_MATCH_OR_DIE(*strarr, BLOCK_END, tcph);
    return((void *)tcph);
}


/*
 * Parse UDP header, returns a pointer to a sa_udph structure 
 * on success and NULL on error.
 */
static void *
parse_udph(void)
{
    struct sa_udph *udph = NULL;
    long val;
    u_char field;
    u_char *fieldtok;
    size_t typerr;


    if ( (udph = (struct sa_udph *)calloc(1, 
            sizeof(struct sa_udph))) == NULL) {
        fprintf(stderr, "Error: parse_udph(): "
        "calloc(): %s\n", strerror(errno));
        return(NULL);
    }

    while ((*strarr != NULL) && !MATCH(*strarr, BLOCK_END)) {

        field = 0;
        typerr = 0;

        if (MATCH(*strarr, UDP_SPORT_TOK))
            field = UDPH_SPORT;

        else if (MATCH(*strarr, UDP_DPORT_TOK))
            field = UDPH_DPORT;
        
        else if (MATCH(*strarr, DATA_TOK)) {

            strarr++;
            PHDR_MATCH_OR_DIE(*strarr, BLOCK_BEGIN, udph);

            if ( (parse_str(&(udph->sa_udph_data), 
                    &(udph->sa_udph_dlen))) == NULL)
                errors++;
            
            continue;
        }

         else {
            fprintf(stderr, "Syntax Error: Unrecognized key word "
                "'%s' in UDP header of packet %u\n", *strarr, pkt_count);
            errors++;
            
            return((void *)udph);
        }

        if (FIELD_IS_SET(udph->sa_udph_fields, field)) {
            fprintf(stderr, "Syntax Error: Multiple entries for "
                "'%s' in UDP header of packet %u\n", *strarr, pkt_count);
            errors++;
        }
        else 
            udph->sa_udph_fields |= field;    

        fieldtok = *strarr;
        strarr++;

        PHDR_MATCH_OR_DIE(*strarr, ASSIGN, udph)

        if (!strisnum(*strarr, &val))
            typerr++;    

        if (!ISu16(val))
            typerr++;

        if (field == UDPH_SPORT)
            udph->sa_udph_sport = htons((u_short)val);

        else if (field == UDPH_DPORT)
            udph->sa_udph_dport = htons((u_short)val);

        if (typerr != 0) {
            fprintf(stderr, "Type Error: Bad value for '%s' "
                "in UDP header of packet %u\n", fieldtok, pkt_count);
                            
            errors += typerr;
        }

        strarr++;
        PHDR_MATCH_OR_DIE(*strarr, CMD_END, udph);
    }

    PHDR_MATCH_OR_DIE(*strarr, BLOCK_END, udph);
    return((void *)udph);
}


/*
 * Parse ICMP "header", returns a pointer to a sa_icmph
 * structure on success and NULL on error.
 */
static void *
parse_icmph(void)
{
    struct sa_icmph *icmph = NULL;
    u_char field;
    u_char *fieldtok;
    long val;
    size_t typerr;

    if ( (icmph = (struct sa_icmph *)calloc(1, 
                sizeof(struct sa_icmph))) == NULL) {
        fprintf(stderr, "Error: parse_icmph(): "
        "calloc(): %s\n", strerror(errno));
        return(NULL);
    }

    while ((*strarr != NULL) && !MATCH(*strarr, BLOCK_END)) {

        field = 0;
        typerr = 0;

        if (MATCH(*strarr, ICMP_CODE_TOK))
            field = ICMPH_CODE;

        else if (MATCH(*strarr, ICMP_TYPE_TOK))
            field = ICMPH_TYPE;

        else if (MATCH(*strarr, ICMP_ID_TOK))
            field = ICMPH_ID;

        else if (MATCH(*strarr, ICMP_SEQ_TOK))
            field = ICMPH_SEQ;

        else if (MATCH(*strarr, DATA_TOK)) {
        
            strarr++;
            PHDR_MATCH_OR_DIE(*strarr, BLOCK_BEGIN, icmph);

            if ( (parse_str(&(icmph->sa_icmph_data), 
                    &(icmph->sa_icmph_dlen))) == NULL)
                errors++;
            
            continue;
        }
        else {
            fprintf(stderr, "Syntax Error: Unrecognized key word "
                "'%s' in ICMP header of packet %u\n", *strarr, pkt_count);
            errors++;
            
            return((void *)icmph);
        }

        if (FIELD_IS_SET(icmph->sa_icmph_fields, field)) {
            fprintf(stderr, "Syntax Error: Multiple entries for "
                "'%s' in ICMP header of packet %u\n", *strarr, pkt_count);
            errors++;
        }
        else 
            icmph->sa_icmph_fields |= field;    

        fieldtok = *strarr;
        strarr++;

        PHDR_MATCH_OR_DIE(*strarr, ASSIGN, icmph)

        if (!strisnum(*strarr, &val))
            typerr++;    

        else if (field == ICMPH_TYPE) {

            if (!ISu8(val))
                typerr++;            
            else {
                icmph->sa_icmph_type = (uint8_t)val;

                if (!IS_SUPPICMP_TYPE(icmph->sa_icmph_type)) {
                    fprintf(stderr, "Error in packet %u: Unsupported ICMP type\n", 
                        pkt_count);
                    errors++;
                }
            }
        }
        else if (field == ICMPH_CODE) {
            
            if (!ISu8(val))
                typerr++;
            else {    
                icmph->sa_icmph_code = (uint8_t)val;

                if (icmph->sa_icmph_code != 0) {
                    fprintf(stderr, "Error in packet %u: Unsupported "
                        "ICMP code\n", pkt_count);
                    errors++;
                }
            }
        }
        else if (field == ICMPH_ID) {
            if (!ISu16(val))
                typerr++;
            else
                icmph->sa_icmph_id = htons((uint16_t)val);
        }

        else if (field == ICMPH_SEQ) {
            if (!ISu16(val))
                typerr++;
            else
                icmph->sa_icmph_seq = htons((uint16_t)val);
        }

        if (typerr != 0) {
            fprintf(stderr, "Type Error: Bad value for '%s' in ICMP "
                "header of packet %u\n", fieldtok, pkt_count);
                            
            errors += typerr;
        }
        strarr++;
        PHDR_MATCH_OR_DIE(*strarr, CMD_END, icmph);
    }

    PHDR_MATCH_OR_DIE(*strarr, BLOCK_END, icmph);


    /* Type required if seq or id is set */
    if ( (FIELD_IS_SET(icmph->sa_icmph_fields, ICMPH_SEQ) || 
            FIELD_IS_SET(icmph->sa_icmph_fields, ICMPH_ID)) &&
            !FIELD_IS_SET(icmph->sa_icmph_fields, ICMPH_TYPE)) {
        fprintf(stderr, "Error in packet %u: ICMP type missing.\n", pkt_count);
        errors++;
    }
    
    return((void *)icmph);
}


/*
 * Parse TCP flags.
 * Flags is either in base 2,8,10 or 16,
 * or symbolic names as URG/U, PSH/P ..
 * On error -1 is returned.
 */
static int
parse_tcpflags(void)
{
    long val;
    size_t err = 0;

    if (!strisnum(*strarr, &val)) {
        
        val = 0;

        while ((*strarr != NULL) && !MATCH(*strarr, CMD_END)) {
            if (CMATCH(*strarr, "URG") || CMATCH(*strarr, "U"))
                val |= 0x20;
            else if (CMATCH(*strarr, "ACK") || CMATCH(*strarr, "A"))
                val |= 0x10;
            else if (CMATCH(*strarr, "PSH") || CMATCH(*strarr, "P"))
                val |= 0x08;
            else if (CMATCH(*strarr, "RST") || CMATCH(*strarr, "R"))
                val |= 0x04;
            else if (CMATCH(*strarr, "SYN") || CMATCH(*strarr, "S"))
                val |= 0x02;
            else if (CMATCH(*strarr, "FIN") || CMATCH(*strarr, "F"))
                val |= 0x01;
            else {
                fprintf(stderr, "Syntax Error: Bad TCP flag '%s' in packet %u\n",
                    *strarr, pkt_count);
                err++;
            }
            strarr++;
        }
    }
    else
        strarr++;

    return( (err != 0) ? -1 : (int)val);
}

/*
 * Parse data string.
 * Data string might contain the following character
 * escape sequences (not completely ANSI ..):
 * \e - Escape character
 * \a - Bell character
 * \b - Backspace character
 * \f - Form-feed character
 * \n - New-line character
 * \r - Carriage return character
 * \t - Tab character
 * \v - Vertical tab character
 * \s - Space character
 * \\ - Backslash character
 * \x - Interpret the next two characters as hex
 *
 * On success a pointer to a char array containing the translated
 * string is returned, NULL otherwise.
 *
 * Arguments:
 *  dbuf - Returned pointer is written here if dbuf is not NULL.
 *  dbuflen - The length of the returned data in bytes is written to
 *            this address if it is not NULL.
 */
static u_char *
parse_str(u_char **dbuf, u_short *dbuflen)
{
    u_char *data;
    size_t dlen = 0; /* Size of data */
    size_t bufsize = DATA_BUFSIZ;
    size_t err = 0;
    u_char hdata[3] = {0,0,'\0'};

    if ( (data = (u_char *)malloc(bufsize)) == NULL) {
        fprintf(stderr, "Error: parse_str(): malloc(): %s\n",
            strerror(errno));
        return(NULL);
    }

    while ((*strarr != NULL) && !MATCH(*strarr, BLOCK_END)) {
        u_char *str = *strarr;

        while (*str != '\0') {

            /* Out of size, realloc */
            if (dlen == bufsize) {
                u_char *tmp;

                bufsize *= 2;

                if ( (tmp = (u_char *)realloc(data, bufsize)) == NULL) {
                    fprintf(stderr, "Error: parse_str(): realloc(): %s\n",
                        strerror(errno));

                    if (data != NULL)
                        free(data);

                    data = NULL;
                    return(NULL);
                }

                data = tmp;
            }


            if (*str == '\\') {
                str++;

                switch(*str) {

                    case 'e': data[dlen++] = '\e'; break; /* Escape character */
                    case 'a': data[dlen++] = '\a'; break; /* Bell character */
                    case 'b': data[dlen++] = '\b'; break; /* Backspace character */
                    case 'f': data[dlen++] = '\f'; break; /* Form-feed character */
                    case 'n': data[dlen++] = '\n'; break; /* New-line character */
                    case 'r': data[dlen++] = '\r'; break; /* Carriage return character */
                    case 't': data[dlen++] = '\t'; break; /* Tab character */
                    case 'v': data[dlen++] = '\v'; break; /* Vertical tab character */
                    case 's': data[dlen++] = ' '; break;  /* Space character */
                    case '\\': data[dlen++] = '\\'; break; /* Backslash character */

                    /* Interpret the next two characters as hex */
                    case 'x':

                        str++;
                        if (!ISHEX((int)(hdata[0] = *str))) {
                            parse_err("Bad hexadecimal sequence in data '\\x%c'\n", 
                                *str);
                            continue;
                        }

                        str++;
                        if (!ISHEX((int)(hdata[1] = *str))) {
                            parse_err("Bad hexadecimal sequence in data '\\x%c%c'\n", 
                                *(str-1), *str);
                            err++;
                            continue;
                        }

                        data[dlen++] = (u_char)strtol(hdata, NULL, 16);
                        break;

                    default:
                        parse_err("Bad escape sequence in data '\\%c'\n", *str);
                        err++;
                        continue;
                        break;
                }
                str++;
            }
            else
                data[dlen++] = *str++;
        }
        strarr++;
    }

    if (!MATCH(*strarr, BLOCK_END)) {
        parse_err("Expected '%s', received '%s'\n",
                BLOCK_END, (*strarr == NULL) ? (u_char *)"EOF" : *strarr);
        err++;
    }
    else
        strarr++;

    /* Empty data is not permitted */
    if (dlen == 0) {
        parse_err("An empty data block is not permitted!\n");
        err++;
    }

    if (err != 0) {
        if (data != NULL)
            free(data);

        data = NULL;
        errors += err;
    }
    else {
        if (dbuf != NULL)
            *dbuf = data;

        if (dbuflen != NULL)
            *dbuflen = dlen;
    }

    return(data);
}


/*
 * Free memory allocated for packets
 * TODO! Fixme!
 */
void
free_pkts(struct sa_pkts *pkts)
{

    if (pkts != NULL) {

        if (pkts->key_pkts != NULL) {        
            struct sa_pkt *pkt = pkts->key_pkts;
            
            while (pkt != NULL) {
                struct sa_pkt *tmp = pkt->sa_kpkt_next;

                /* Replicated packets have the same protocol pointer, 
                 * so we only need to free one of them */
                if (tmp && (tmp->sa_pkt_ph != NULL) && (tmp->sa_pkt_ph == pkt->sa_pkt_ph))
                    free(pkt);
                else {
                    FREE_SAPKT(pkt);
                }
                pkt = tmp;
            }
        }
        FREE_SAPKT(pkts->cmd_pkt);
        free(pkts);
    }
}

