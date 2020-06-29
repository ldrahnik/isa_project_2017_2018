/**
 * Name: Lukáš Drahník
 * Project: ISA: Měření ztrátovosti a RTT (Matěj Grégr)
 * Date: 29.6.2020
 * Email: <xdrahn00@stud.fit.vutbr.cz>, <ldrahnik@gmail.com>
 */

#ifndef _testovac_params_H_
#define _testovac_params_H_

#include "testovac-error.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/**
 * Syntaxe: <uzel;RTT> - např. 2001:db8::1<HOST_RTT_DELIMETER>4.5.
 */
static const char HOST_RTT_DELIMETER = ';';

/**
 * Node structure.
 */
typedef struct node {
  char* node;         // IPv4/IPv6/hostname adresa uzlu
  int ecode;          // error code
  float specific_rtt; // <uzel;RTT> default value is -1
} TNode;

/**
 * Terminal parameters:
 *    boolean values: int [0 = FALSE, 1 = TRUE]
 *    ecode: int [0 = DEFAULT IS OK, X = ERROR CODE]
 *    others: int, float [-1 = DEFAULT IS UNSET, X = SET VALUE]
 */
typedef struct params {
  int show_help_message;              // option h
  int udp_enable;                     // option u
  int size_of_data;                   // option s
  int evaluation_interval;            // option t
  int message_interval;               // option i
  int response_timeout;               // option w
  int udp_port;                       // option p
  int udp_listen_port;                // option l
  float rtt_value;                    // option r
  int verbose_mode;                   // option v
  int ecode;                          // error code
  int nodes_count;
  struct node *nodes;
} TParams;

TNode getNode(char* host);
int isValidHost(char* host);
TParams getParams(int argc, char *argv[]);

#endif
