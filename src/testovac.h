/**
 * Name: Lukáš Drahník
 * Project: ISA: Měření ztrátovosti a RTT (Matěj Grégr)
 * Date: 29.6.2020
 * Email: <xdrahn00@stud.fit.vutbr.cz>, <ldrahnik@gmail.com>
 */

#ifndef _testovac_H_
#define _testovac_H_

#include "testovac-params.h"
#include "testovac-error.h"

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
 * Pseudoheader of UDP packet.
 */
struct outdata_udp {
  int id;
  struct timeval tv;
};

unsigned short checksum(unsigned char *addr, int count);
void catchsignal(int sig);
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
