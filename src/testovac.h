/**
 * Name: Lukáš Drahník
 * Project: ISA: Měření ztrátovosti a RTT (Matěj Grégr)
 * Date: 29.6.2020
 * Email: <xdrahn00@stud.fit.vutbr.cz>, <ldrahnik@gmail.com>
 */

#ifndef _testovac_H_
#define _testovac_H_

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <math.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>

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

/**
 * Arguments for pthread created for each node.
 */
typedef struct pthread_args {
  struct params *params;
  int node_index;                     // start index is 0
  struct addrinfo *addrinfo;
  int packets_sent_count;
  int sock;                           // in UDP case is used in both sides (sending, listening)
} Tpthread_args;

/**
 * Error codes.
 */
enum ecodes {
  EOK = 0,               // ok
  EOPT = 1,              // invalid option (option argument is missing,
                          // unknown option, unknown option character)
  ENODE = 2,             // node is not valid (invalid IPv4 or IPv6 or hostname)
  ETHREAD_CREATE = 3,    // pthread_create funcion returned error
  ESOCKET_CREATE = 4     // socket function returned error
};

/**
 * Pseudoheader of UDP packet.
 */
struct outdata_udp {
  int id;
  struct timeval tv;
};

unsigned short checksum(unsigned char *addr, int count);
void catchsignal(int sig);
int isValidHost(char* host);
TNode getNode(char* host);
TParams getParams(int argc, char *argv[]);
void getCurrentTime(timeval* tv_current, time_t* curtime, char* time_buffer);
void printReportInfo(int packets_sent_count, int recv_packets, int recv_packets_exceeded_rtt_percent, int recv_packets_exceeded_rtt_count, float rtt_current, TNode* node, TParams* params, timeval* evaluation_interval_tv);
void printVerboseInfo(int recv_length, TNode* node, char* node_addr_string, double rtt_current);
void printSummaryInfo(int packets_sent_count, int recv_packets, double rtt_max, double rtt_min, double rtt_avg, double rtt_mdev, pthread_args* pthread_args, TNode* node, timeval* summary_interval_tv);
void* handleIcmpIpv6Receiving(void *threadarg);
void* handleIcmpIpv4Receiving(void *threadarg);
void* handleIcmpIpv6Sending(void *threadarg);
void* handleIcmpIpv4Sending(void *threadarg);
void* handleUdpIpv4Sending(void *threadarg);
void* handleUdpIpv6Sending(void *threadarg);
void* handleUdpServer(void *threadarg);
void* handleUdpIpv4Receiving(void *threadarg);
void* handleUdpIpv6Receiving(void *threadarg);
void clean(TParams *params, Tpthread_args* threads_args[]);
int main(int argc, char *argv[]);

#endif
