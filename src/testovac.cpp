/**
 * Name:							Lukáš Drahník
 * Project: 					ISA: Měření ztrátovosti a RTT (Matěj Grégr)
 * Date:							30.9.2017
 * Email:						  <xdrahn00@stud.fit.vutbr.cz>, <ldrahnik@gmail.com>
 */

#include "testovac.h"

using namespace std;

/**
 * When is pressed ctrl+c.
 */
static int G_break = 0;

/**
 * Every hour is printed summary info to stdout.
 */
static int SUMMARY_INFO_INTERVAL = 216000;

/**
 * Help message.
 */
const char *HELP_MSG = {
  "Testovac\n\n"
  "monitors entered site nodes. Print informations on standard output when is\n"
  "packet loss or excedation Round trip time (RTT) over specified value. When is not\n"
  "specificated protocol and port with options -u and -p is used ICMP echo request/reply.\n"
  "When is selected protocol UDP packet send random data with size 64B and expect the same\n"
  "packet with the same content. When is specified port with option -l program listen on\n"
  "UDP port and on received packets reply the same packets with the same content. Program\n"
  "handle each of node parallely. When is not used option -r is tested only packets loss but\n"
  "no RTT. RTT is in this case included only to the summary statistics.\n\n"
  "Example of usage:\n\n"
  "./testovac [-h] [-u] [-t <interval>] [-i <interval>] [-p <port>] [-l <port>] [-s <size>] [-r <value>] <node1> <node2> <node3> ... \n\n"
  "Options:\n"
  "-h  -- show help message\n"
  "-u  - select UDP protocol\n"
  "-s - size of data for sending (default value is 56B)\n"
  "-t <interval> -- interval in seconds for which is packets loss evaluated (default value is 300s)\n"
  "-i <interval> -- interval in ms how often send testing messages (default value is 100ms)\n"
  "-w <timeout> -- how long is program waiting but only when is response not received (default value is 2s)\n"
  "-p <port> -- specification of UDP port\n"
  "-l <port> -- specification of listening UDP port\n"
  "-r <value> -- RTT treshold - reports only when is value exceeded\n"
  "-v - verbose mode - program prints on stdout send packets\n"
  "<node> or <node;RTT>-- IPv4/IPv6/hostname address of node, RTT which has priority for each node over global RTT -r <value>\n"
};

/**
 * Checksum.
 *
 * @return unsigned short
 */
unsigned short checksum(unsigned char *addr, int count) {
  register long sum = 0;

  // this is the inner loop
  while (count > 1) {
    sum += * (unsigned short *) addr;
    addr += 2;
    count -= 2;
  }

  // add left-over byte, if any
  if(count > 0)
    sum += *(unsigned char *) addr;

  // fold 32-bit sum to 16 bits
  while (sum>>16 != 0)
    sum = (sum & 0xffff) + (sum >> 16);

  return ~sum;
}

/**
 * Signal handler.
 */
void catchsignal(int sig) {
  if(sig == SIGINT) {
    G_break = 1;
  }
}

/**
 * Update and format time before is printf used.
 *
 * @return void
 */
void getCurrentTime(timeval* tv_current, time_t* curtime, char* time_buffer) {
  gettimeofday(tv_current, NULL);
  curtime = &tv_current->tv_sec;
  strftime(time_buffer, 30, "%F %T.", localtime(curtime));
}

/**
 * Report info.
 *
 * @return void
 */
void printReportInfo(int packets_sent_count, int recv_packets, int recv_packets_exceeded_rtt_percent, int recv_packets_exceeded_rtt_count, float rtt_current, TNode* node, TParams* params, timeval* evaluation_interval_tv) {

  // current time with ms
  char* time_buffer = (char *)calloc(30, 1);
  struct timeval tv_current;
  time_t curtime;

  getCurrentTime(&tv_current, &curtime, time_buffer);

  if((node->specific_rtt != -1 && rtt_current > node->specific_rtt) || (params->rtt_value != -1 && rtt_current > params->rtt_value)) {
    if(tv_current.tv_sec - evaluation_interval_tv->tv_sec >= params->evaluation_interval) {

      // reset tv when was last report info displayed
      gettimeofday(evaluation_interval_tv, NULL);

      // format: YYYY-MM-DD HH:MM:SS.MS <give-node-name-as-argument>: <00>% (<count-of-lost-packets>) packets exceeded RTT treshold <arg-r-value>ms
      fprintf(stdout, "%s%.2ld %s: %d %% (%d) packets exceeded RTT treshold %.3f ms\n", time_buffer, tv_current.tv_usec / 10000, node->node, recv_packets_exceeded_rtt_percent, recv_packets_exceeded_rtt_count, node->specific_rtt != -1 ? node->specific_rtt : params->rtt_value);
    }
  }

  free(time_buffer);
}

/**
 * Verbose info.
 *
 * @return void
 */
void printVerboseInfo(int recv_length, TNode* node, char* node_addr_string, double rtt_current) {

  // current time with ms
  char* time_buffer = (char *)calloc(30, 1);
  struct timeval tv_current;
  time_t curtime;

  getCurrentTime(&tv_current, &curtime, time_buffer);

  // format: YYYY-MM-DD HH:MM:SS.MS <count-of-bytes> bytes from <give-node-name-as-argument> (<translated-node-addr) time=<0.00> ms
  getCurrentTime(&tv_current, &curtime, time_buffer);
  fprintf(stdout, "%s%.2ld %d bytes from %s (%s) time=%.3f ms\n", time_buffer, tv_current.tv_usec / 10000, recv_length, node->node, node_addr_string, rtt_current);

  free(time_buffer);
}

/**
 * Summary info.
 *
 * @return void
 */
void printSummaryInfo(int packets_sent_count, int recv_packets, double rtt_max, double rtt_min, double rtt_avg, double rtt_mdev, pthread_args* pthread_args, TNode* node, timeval* summary_interval_tv) {

  // current time with ms
  char* time_buffer = (char *)calloc(30, 1);
  struct timeval tv_current;
  time_t curtime;

  getCurrentTime(&tv_current, &curtime, time_buffer);

  if(tv_current.tv_sec - summary_interval_tv->tv_sec >= SUMMARY_INFO_INTERVAL) {

    // reset tv when was last summary info displayed
    gettimeofday(summary_interval_tv, NULL);

    // packet loss
    int recv_packets_loss_percent = 100*(pthread_args->packets_sent_count-recv_packets)/(pthread_args->packets_sent_count);

    if(recv_packets_loss_percent == 100) {
      // format: YYYY-MM-DD HH:MM:SS.MS <give-node-name-as-argument>: <status down
      fprintf(stdout, "%s%.2ld %s: status down\n", time_buffer, tv_current.tv_usec / 10000, node->node);
    } else {
      // format: YYYY-MM-DD HH:MM:SS.MS <give-node-name-as-argument>: <status down
      fprintf(stdout, "%s%.2ld %s: %d %% packet loss, %.3lf/%.3lf/%.3lf/%.3lf ms\n", time_buffer, tv_current.tv_usec / 10000, node->node, recv_packets_loss_percent, rtt_max, rtt_min, rtt_avg, rtt_mdev);
    }
  }

  free(time_buffer);
}

/**
 * @return int
 */
void* handleIcmpIpv6Receiving(void *threadarg) {
  struct pthread_args *pthread_args = (struct pthread_args *) threadarg;
  struct node node = (struct node) pthread_args->params->nodes[pthread_args->node_index];
  struct addrinfo *addrinfo = pthread_args->addrinfo;
  TParams* params = (TParams*) pthread_args->params;

  socklen_t size;
  struct icmp6_hdr *recv;
  struct ip6_hdr *ip;
  int recv_length;
  struct sockaddr_in6 receive_sock_addr;
  char* recv_buffer = (char *)calloc(IP_MAXPACKET, 1);
  fd_set my_set;
  char node_addr_string[INET6_ADDRSTRLEN];

  struct timeval tv;
  struct timeval *out, in;
  struct timeval evaluation_interval_tv;
  struct timeval summary_interval_tv;
  gettimeofday(&evaluation_interval_tv, NULL);
  gettimeofday(&summary_interval_tv, NULL);

  // received packets
  int recv_packets = 0;
  int recv_packets_loss_percent = 0;

  // received over rtt
  int recv_packets_exceeded_rtt_count = 0;
  int recv_packets_exceeded_rtt_percent = 0;

  // RTT Statistics
  double rtt_current = 0;
  double rtt_max = 0;
  double rtt_min = 1000000;
  double rtt_sum = 0;
  double rtt_sum2 = 0;
  double rtt_avg = 0;
  double rtt_avg2 = 0;
  double rtt_mdev = 0;

  // create socket
  int sock;
  if((sock = socket(PF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) == -1) {
    fprintf(stderr, "Socket can not be created. Run program as sudo.\n");

    pthread_exit(NULL);
  }

  // install signal handler for CTRL+C
  signal(SIGINT, catchsignal);

  while(true) {

    // CTRL+C handler
    if(G_break == 1) {
      break;
    }

    do {

      // summary info
      printSummaryInfo(pthread_args->packets_sent_count, recv_packets, rtt_max, rtt_min, rtt_avg, rtt_mdev, pthread_args, &node, &summary_interval_tv);

      // report info
      printReportInfo(pthread_args->packets_sent_count, recv_packets, recv_packets_exceeded_rtt_percent, recv_packets_exceeded_rtt_count, rtt_current, &node, params, &evaluation_interval_tv);

      FD_ZERO(&my_set);
      FD_SET(sock, &my_set);
      if(select(sock + 1, &my_set, NULL, NULL, &tv) < 0) {
        fprintf(stderr, "Select failed\n");
        break;
      }
      if(FD_ISSET(sock, &my_set)) {
        size = sizeof(sockaddr_in);
        if((recv_length = recvfrom(sock, recv_buffer, IP_MAXPACKET, 0, (sockaddr *)&receive_sock_addr,&size)) == -1) {
          fprintf(stderr, "Receiving data error\n");
        }

        // get timestamp (when was received)
        gettimeofday(&in, NULL);

        recv = (struct icmp6_hdr *) (recv_buffer);

        if((ntohs(recv->icmp6_id) == pthread_args->node_index) && (recv->icmp6_type == ICMP6_ECHO_REPLY)) {
          out = (struct timeval *) (recv + 1);

          // set up waiting time for packet (for first packet arg -w (2s default), for next 2xRTT)
          float response_timeout_sec = 2 * rtt_current;
          if(rtt_current == 0) {
            response_timeout_sec = params->response_timeout;
          }

          // is too late
          // lost packet
          if((in.tv_sec - out->tv_sec + 1e-6*(in.tv_usec - out->tv_usec)) > response_timeout_sec) {
            break;
          }

          recv_packets++;

          // get node (destination) addr
          strdup(inet_ntop(AF_INET6, &(receive_sock_addr.sin6_addr.s6_addr), node_addr_string, INET6_ADDRSTRLEN));

          // re-calculate
          rtt_current = 1e3*(in.tv_sec - out->tv_sec + 1e-6*(in.tv_usec - out->tv_usec));
          rtt_max = (rtt_current > rtt_max)?rtt_current:rtt_max;
          rtt_min = (rtt_current < rtt_min)?rtt_current:rtt_min;
          rtt_sum += rtt_current;
          rtt_sum2 += pow(rtt_current, 2);
          rtt_avg = rtt_sum / recv_packets;
          rtt_avg2 = rtt_sum2 / recv_packets;
          rtt_mdev = sqrt(rtt_avg2 - pow(rtt_avg, 2));

          // report when is rtt_current higher than arg -r
          if((node.specific_rtt != -1 && rtt_current > node.specific_rtt) || (params->rtt_value != -1 && rtt_current > params->rtt_value)) {
            recv_packets_exceeded_rtt_count += 1;
            recv_packets_exceeded_rtt_percent = 100*(recv_packets_exceeded_rtt_count)/(recv_packets);
          }

          // verbose mode -> print incoming info of packets (like ping does)
          if(params->verbose_mode) {
            printVerboseInfo(recv_length, &node, node_addr_string, rtt_current);
          }
        }
      } else {
        // lost packet
        break;
      }
    }
    while(!((ntohs(recv->icmp6_id) == pthread_args->node_index) && (recv->icmp6_type == ICMP6_ECHO_REPLY)));
  }

  // close sock
  close(sock);

  // clean
  free(recv_buffer);

  pthread_exit(NULL);
}

/**
 * @return int
 */
void* handleIcmpIpv4Receiving(void *threadarg) {
  struct pthread_args *pthread_args = (struct pthread_args *) threadarg;
  struct node node = (struct node) pthread_args->params->nodes[pthread_args->node_index];
  struct addrinfo *addrinfo = pthread_args->addrinfo;
  TParams* params = (TParams*) pthread_args->params;
  int sock = pthread_args->sock;

  socklen_t size;
  struct icmp *recv;
  struct ip *ip;
  int recv_length, recv_length_without_ip;
  struct sockaddr_in receive_sock_addr;
  char* recv_buffer = (char *)calloc(IP_MAXPACKET, 1);
  fd_set my_set;
  char node_addr_string[INET_ADDRSTRLEN];
  int hlen1;

  struct timeval tv;
  struct timeval *out, in;
  struct timeval evaluation_interval_tv;
  struct timeval summary_interval_tv;
  gettimeofday(&evaluation_interval_tv, NULL);
  gettimeofday(&summary_interval_tv, NULL);

  // received packets
  int recv_packets = 0;
  int recv_packets_loss_percent = 0;

  // received over rtt
  int recv_packets_exceeded_rtt_count = 0;
  int recv_packets_exceeded_rtt_percent = 0;

  // RTT Statistics
  double rtt_current = 0;
  double rtt_max = 0;
  double rtt_min = 1000000;
  double rtt_sum = 0;
  double rtt_sum2 = 0;
  double rtt_avg = 0;
  double rtt_avg2 = 0;
  double rtt_mdev = 0;

  // install signal handler for CTRL+C
  signal(SIGINT, catchsignal);

  while(true) {

    // CTRL+C handler
    if(G_break == 1) {
      break;
    }

    do {

      // summary info
      printSummaryInfo(pthread_args->packets_sent_count, recv_packets, rtt_max, rtt_min, rtt_avg, rtt_mdev, pthread_args, &node, &summary_interval_tv);

      // report info
      printReportInfo(pthread_args->packets_sent_count, recv_packets, recv_packets_exceeded_rtt_percent, recv_packets_exceeded_rtt_count, rtt_current, &node, params, &evaluation_interval_tv);

      FD_ZERO(&my_set);
      FD_SET(sock, &my_set);
      if(select(sock + 1, &my_set, NULL, NULL, &tv) < 0) {
        fprintf(stderr, "Select failed\n");
        break;
      }
      if(FD_ISSET(sock, &my_set)) {
        size = sizeof(sockaddr_in);
        if((recv_length = recvfrom(sock, recv_buffer, IP_MAXPACKET, 0, (sockaddr *)&receive_sock_addr, &size)) == -1) {
          fprintf(stderr, "Receiving data error\n");
        }

        // get timestamp (when was received)
        gettimeofday(&in, NULL);

        // start of IP header
        ip = (struct ip *) recv_buffer;
        // length of IP header
        hlen1 = ip->ip_hl << 2;
        // start of ICMP header
        recv = (struct icmp *) (recv_buffer + hlen1);

        if((ntohs(recv->icmp_id) == pthread_args->node_index) && (recv->icmp_type == ICMP_ECHOREPLY)) {
          out = (struct timeval *) recv->icmp_data;

          // set up waiting time for packet (for first packet arg -w (2s default), for next 2xRTT)
          float response_timeout_sec = 2 * rtt_current;
          if(rtt_current == 0) {
            response_timeout_sec = params->response_timeout;
          }

          // is too late
          // lost packet
          if((in.tv_sec - out->tv_sec + 1e-6*(in.tv_usec - out->tv_usec)) > response_timeout_sec) {
            break;
          }

          recv_packets++;

          // get node (destination) addr
          strdup(inet_ntop(AF_INET, &(receive_sock_addr.sin_addr.s_addr), node_addr_string, INET_ADDRSTRLEN));

          // re-calculate
          rtt_current = 1e3*(in.tv_sec - out->tv_sec + 1e-6*(in.tv_usec - out->tv_usec));
          rtt_max = (rtt_current > rtt_max)?rtt_current:rtt_max;
          rtt_min = (rtt_current < rtt_min)?rtt_current:rtt_min;
          rtt_sum += rtt_current;
          rtt_sum2 += pow(rtt_current, 2);
          rtt_avg = rtt_sum / recv_packets;
          rtt_avg2 = rtt_sum2 / recv_packets;
          rtt_mdev = sqrt(rtt_avg2 - pow(rtt_avg, 2));

          // report when is rtt_current higher than arg -r
          if((node.specific_rtt != -1 && rtt_current > node.specific_rtt) || (params->rtt_value != -1 && rtt_current > params->rtt_value)) {
            recv_packets_exceeded_rtt_count += 1;
            recv_packets_exceeded_rtt_percent = 100*(recv_packets_exceeded_rtt_count)/(recv_packets);
          }

          // verbose mode -> print incoming info of packets (like ping does)
          if(params->verbose_mode) {
            printVerboseInfo(recv_length, &node, node_addr_string, rtt_current);
          }
        }
      } else {
        break;
      }
    }
    while (!(ntohs(recv->icmp_id) == pthread_args->node_index) && (recv->icmp_type == ICMP_ECHOREPLY));
  }

  // close sock
  close(sock);

  // clean
  free(recv_buffer);

  pthread_exit(NULL);
}

/**
 * @return int
 */
void* handleIcmpIpv6Sending(void *threadarg) {
  struct pthread_args *pthread_args = (struct pthread_args *) threadarg;
  struct node node = (struct node) pthread_args->params->nodes[pthread_args->node_index];
  struct addrinfo *addrinfo = pthread_args->addrinfo;
  TParams* params = (TParams*) pthread_args->params;
  int sock = pthread_args->sock;

  struct icmp6_hdr *send;
  struct sockaddr_in6 send_sock_addr;

  int icmp_len = sizeof(struct icmp6_hdr) + sizeof(struct timeval) + params->size_of_data;
  char* send_buffer = (char *)calloc(icmp_len, 1);

  // icmp6_hdr
  send = (struct icmp6_hdr *)(send_buffer);
  send->icmp6_type = ICMP6_ECHO_REQUEST;
  send->icmp6_code = htons(0);
  send->icmp6_seq = htons(0);

  // sockaddr_in6
  memset(&send_sock_addr, 0, sizeof(struct sockaddr_in6));
  memcpy(&send_sock_addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
  send_sock_addr.sin6_family = AF_INET6;
  send_sock_addr.sin6_port = 0;

  // install signal handler for CTRL+C
  signal(SIGINT, catchsignal);

  while(true) {

    // CTRL+C handler
    if(G_break == 1)
      break;

    // identifier
    send->icmp6_id = 0;
    send->icmp6_id = htons(pthread_args->node_index);

    // timeval
    gettimeofday((struct timeval*) (send + 1), NULL);

    if(sendto(sock, send_buffer, icmp_len, 0, (sockaddr *)&send_sock_addr, sizeof(send_sock_addr)) == -1) {
      fprintf(stderr, "Error during sendto: %s\n", strerror(errno));

      // clean
      free(send_buffer);

      // close sock
      close(sock);

      pthread_exit(NULL);
    }

    pthread_args->packets_sent_count++;

    usleep(params->message_interval * pow(10,3));
  }

  // close sockf
  close(sock);

  // clean
  free(send_buffer);

  pthread_exit(NULL);
}

/**
 * @return int
 */
void* handleIcmpIpv4Sending(void *threadarg) {
  struct pthread_args *pthread_args = (struct pthread_args *) threadarg;
  struct node node = (struct node) pthread_args->params->nodes[pthread_args->node_index];
  struct addrinfo *addrinfo = pthread_args->addrinfo;
  TParams* params = (TParams*) pthread_args->params;
  int sock = pthread_args->sock;

  struct icmp *send;
  struct sockaddr_in send_sock_addr;

  int icmp_len = sizeof(struct icmp) + params->size_of_data;
  char* send_buffer = (char *)calloc(icmp_len, 1);

  // icmp
  send = (struct icmp *)(send_buffer);
  send->icmp_type = ICMP_ECHO;
  send->icmp_code = htons(0);
  send->icmp_seq = htons(0);

  // sockaddr_in
  memset(&send_sock_addr, 0, sizeof(struct sockaddr_in));
  memcpy(&send_sock_addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
  send_sock_addr.sin_family = AF_INET;
  send_sock_addr.sin_port = 0;

  // install signal handler for CTRL+C
  signal(SIGINT, catchsignal);

  while(true) {

    // CTRL+C handler
    if(G_break == 1)
      break;

    // identifier
    send->icmp_id = 0;
    send->icmp_id = htons(pthread_args->node_index);

    // timeval
    gettimeofday((struct timeval*) send->icmp_data, NULL);

    // checksum
    send->icmp_cksum = 0;
    send->icmp_cksum = checksum((unsigned char*)send_buffer, icmp_len);

    if(sendto(sock, send_buffer, icmp_len, 0, (sockaddr *)&send_sock_addr, sizeof(send_sock_addr)) == -1) {
      fprintf(stderr, "Error during sendto: %s\n", strerror(errno));

      // clean
      free(send_buffer);

      // close sock
      close(sock);

      pthread_exit(NULL);
    }

    pthread_args->packets_sent_count++;

    usleep(params->message_interval * pow(10,3));
  }

  // close sock
  close(sock);

  // clean
  free(send_buffer);

  pthread_exit(NULL);
}

/**
 * @return int
 */
void* handleUdpIpv4Sending(void *threadarg) {
  struct pthread_args *pthread_args = (struct pthread_args *) threadarg;
  struct node node = (struct node) pthread_args->params->nodes[pthread_args->node_index];
  struct addrinfo *addrinfo = pthread_args->addrinfo;
  TParams* params = (TParams*) pthread_args->params;
  int sock = pthread_args->sock;

  struct outdata_udp *outdata_udp;
  struct sockaddr_in send_sock_addr, recv_sock_addr;

  int udp_len = sizeof(struct outdata_udp) + params->size_of_data;
  char* send_buffer = (char *)calloc(udp_len, 1);

  outdata_udp = (struct outdata_udp *) (send_buffer);

  // sendto addr and port
  memset(&send_sock_addr, 0, sizeof(struct sockaddr_in6));
  memcpy(&send_sock_addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
  send_sock_addr.sin_family = AF_INET;
  send_sock_addr.sin_port = htons(params->udp_port);
  send_sock_addr.sin_addr = ((struct sockaddr_in*)addrinfo->ai_addr)->sin_addr;

  // install signal handler for CTRL+C
  signal(SIGINT, catchsignal);

  while(true) {

    // CTRL+C handler
    if(G_break == 1) {
      break;
    }

    // identifier
    outdata_udp->id = htons(pthread_args->node_index);

    // timeval
    gettimeofday(&outdata_udp->tv, NULL);

    if(sendto(sock, send_buffer, udp_len, 0, (struct sockaddr *)&send_sock_addr, sizeof(send_sock_addr)) == -1) {
      fprintf(stderr, "Error during sendto: %s\n", strerror(errno));

      // clean
      free(send_buffer);

      pthread_exit(NULL);
    }

    pthread_args->packets_sent_count++;

    usleep(params->message_interval * pow(10,3));
  }

 // clean
 free(send_buffer);

 pthread_exit(NULL);
}

/**
 * @return int
 */
void* handleUdpIpv6Sending(void *threadarg) {
  struct pthread_args *pthread_args = (struct pthread_args *) threadarg;
  struct node node = (struct node) pthread_args->params->nodes[pthread_args->node_index];
  struct addrinfo *addrinfo = pthread_args->addrinfo;
  TParams* params = (TParams*) pthread_args->params;
  int sock = pthread_args->sock;

  struct outdata_udp *outdata_udp;
  struct sockaddr_in6 send_sock_addr, recv_sock_addr;

  int udp_len = sizeof(struct outdata_udp) + params->size_of_data;
  char* send_buffer = (char *)calloc(udp_len, 1);

  outdata_udp = (struct outdata_udp *) (send_buffer);

  // sendto addr and port
  memset(&send_sock_addr, 0, sizeof(struct sockaddr_in6));
  memcpy(&send_sock_addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
  send_sock_addr.sin6_family = AF_INET6;
  send_sock_addr.sin6_port = htons(params->udp_port);
  send_sock_addr.sin6_addr = ((struct sockaddr_in6 *)addrinfo->ai_addr)->sin6_addr;

  // install signal handler for CTRL+C
  signal(SIGINT, catchsignal);

  while(true) {

    // CTRL+C handler
    if(G_break == 1) {
      break;
    }

    // identifier
    outdata_udp->id = htons(pthread_args->node_index);

    // timeval
    gettimeofday(&outdata_udp->tv, NULL);

    if(sendto(sock, send_buffer, udp_len, 0, (struct sockaddr *)&send_sock_addr, sizeof(send_sock_addr)) == -1) {
      fprintf(stderr, "Error during sendto: %s\n", strerror(errno));

      // clean
      free(send_buffer);

      // close sock
      close(sock);

      pthread_exit(NULL);
    }

    pthread_args->packets_sent_count++;

    usleep(params->message_interval * pow(10,3));
  }

 // clean
 free(send_buffer);

 // close sock
 close(sock);

 pthread_exit(NULL);
}

/**
 * @return void*
 */
void* handleUdpServer(void *threadarg) {
  TParams *params = (TParams *) threadarg;

  struct sockaddr_storage serverStorage;
  socklen_t addr_size;
  addr_size = sizeof serverStorage;

  struct sockaddr_in6 server_addr;
  char buffer[IP_MAXPACKET];
  int sock;
  long recv_len;
  struct timeval tv;

  fd_set my_set;

  // create socket
  if((sock = socket(PF_INET6, SOCK_DGRAM, 0)) < 0) {
    fprintf(stderr, "Socket can not be created. Run program as sudo.\n");

    pthread_exit(NULL);
  }

  memset((char *)&server_addr, 0, sizeof(server_addr));
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_addr = in6addr_any;
  server_addr.sin6_port = htons(params->udp_listen_port);

  int no = 0;
  if(setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&no, sizeof(no))) {
    fprintf(stderr, "setsockopt() for IPV6_V6ONLY error.\n");

    // close sock
    close(sock);

    pthread_exit(NULL);
  }

  // install signal handler for CTRL+C
  signal(SIGINT, catchsignal);

  if(bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "Socket can not be binded. Port is probably already in use.\n");

    // close sock
    close(sock);

    pthread_exit(NULL);
  }

  while(true) {

    // CTRL+C handler
    if(G_break == 1) {
      break;
    }

    FD_ZERO(&my_set);
    FD_SET(sock, &my_set);
    if(select(sock + 1, &my_set, NULL, NULL, &tv) < 0) {
      fprintf(stderr, "Select failed.\n");
      break;
    }
    if(FD_ISSET(sock, &my_set)) {

      if((recv_len = recvfrom(sock, buffer, IP_MAXPACKET, 0, (struct sockaddr *)&serverStorage, &addr_size)) == -1) {
        fprintf(stderr, "Error during recvfrom: %s\n", strerror(errno));

        // close sock
        close(sock);

        pthread_exit(NULL);
      }

      if(sendto(sock, buffer, recv_len, 0, (struct sockaddr *)&serverStorage, addr_size) == -1) {
        fprintf(stderr, "Error during sendto: %s\n", strerror(errno));

        // close sock
        close(sock);

        pthread_exit(NULL);
      }
    }
  }

  // close
  close(sock);

  pthread_exit(NULL);
}

/**
 * @return int
 */
void* handleUdpIpv4Receiving(void *threadarg) {
  struct pthread_args *pthread_args = (struct pthread_args *) threadarg;
  struct node node = (struct node) pthread_args->params->nodes[pthread_args->node_index];
  struct addrinfo *addrinfo = pthread_args->addrinfo;
  TParams* params = (TParams*) pthread_args->params;
  int sock = pthread_args->sock;

  socklen_t size;
  struct outdata_udp *recv;
  int recv_length;
  struct sockaddr_in receive_sock_addr;
  char* recv_buffer = (char *)calloc(IP_MAXPACKET, 1);
  fd_set my_set;
  char node_addr_string[INET_ADDRSTRLEN];

  struct timeval tv;
  struct timeval *out, in;
  struct timeval evaluation_interval_tv;
  struct timeval summary_interval_tv;
  gettimeofday(&evaluation_interval_tv, NULL);
  gettimeofday(&summary_interval_tv, NULL);

  // received packets
  int recv_packets = 0;
  int recv_packets_loss_percent = 0;

  // received over rtt
  int recv_packets_exceeded_rtt_count = 0;
  int recv_packets_exceeded_rtt_percent = 0;

  // RTT Statistics
  double rtt_current = 0;
  double rtt_max = 0;
  double rtt_min = 1000000;
  double rtt_sum = 0;
  double rtt_sum2 = 0;
  double rtt_avg = 0;
  double rtt_avg2 = 0;
  double rtt_mdev = 0;

  receive_sock_addr.sin_family = AF_INET;
  receive_sock_addr.sin_addr = ((struct sockaddr_in*)addrinfo)->sin_addr;
  receive_sock_addr.sin_port = htons(params->udp_port);

  // install signal handler for CTRL+C
  signal(SIGINT, catchsignal);

  while(true) {

    // CTRL+C handler
    if(G_break == 1) {
      break;
    }

    do {

      // summary info
      printSummaryInfo(pthread_args->packets_sent_count, recv_packets, rtt_max, rtt_min, rtt_avg, rtt_mdev, pthread_args, &node, &summary_interval_tv);

      // report info
      printReportInfo(pthread_args->packets_sent_count, recv_packets, recv_packets_exceeded_rtt_percent, recv_packets_exceeded_rtt_count, rtt_current, &node, params, &evaluation_interval_tv);

      FD_ZERO(&my_set);
      FD_SET(sock, &my_set);
      if(select(sock + 1, &my_set, NULL, NULL, &tv) < 0) {
        fprintf(stderr, "Select failed\n");
        break;
      }
      if(FD_ISSET(sock, &my_set)) {
        size = sizeof(sockaddr_in);
        if((recv_length = recvfrom(sock, recv_buffer, IP_MAXPACKET, 0, (sockaddr *)&receive_sock_addr, &size)) == -1) {
          fprintf(stderr, "Receiving data error\n");
        }

        // get timestamp (when was received)
        gettimeofday(&in, NULL);

        recv = (struct outdata_udp *) (recv_buffer);

        if(ntohs(recv->id) == pthread_args->node_index) {
          out = (struct timeval *) (&recv->tv);

          // set up waiting time for packet (for first packet arg -w (2s default), for next 2xRTT)
          float response_timeout_sec = 2 * rtt_current;
          if(rtt_current == 0) {
            response_timeout_sec = params->response_timeout;
          }

          // is too late
          // lost packet
          if((in.tv_sec - out->tv_sec + 1e-6*(in.tv_usec - out->tv_usec)) > response_timeout_sec) {
            break;
          }

          recv_packets++;

          // get node (destination) addr
          strdup(inet_ntop(AF_INET, &(receive_sock_addr.sin_addr.s_addr), node_addr_string, INET_ADDRSTRLEN));

          // re-calculate
          rtt_current = 1e3*(in.tv_sec - out->tv_sec + 1e-6*(in.tv_usec - out->tv_usec));
          rtt_max = (rtt_current > rtt_max)?rtt_current:rtt_max;
          rtt_min = (rtt_current < rtt_min)?rtt_current:rtt_min;
          rtt_sum += rtt_current;
          rtt_sum2 += pow(rtt_current, 2);
          rtt_avg = rtt_sum / recv_packets;
          rtt_avg2 = rtt_sum2 / recv_packets;
          rtt_mdev = sqrt(rtt_avg2 - pow(rtt_avg, 2));

          // report when is rtt_current higher than arg -r
          if((node.specific_rtt != -1 && rtt_current > node.specific_rtt) || (params->rtt_value != -1 && rtt_current > params->rtt_value)) {
            recv_packets_exceeded_rtt_count += 1;
            recv_packets_exceeded_rtt_percent = 100*(recv_packets_exceeded_rtt_count)/(recv_packets);
          }

          // verbose mode -> print incoming info of packets (like ping does)
          if(params->verbose_mode) {
            printVerboseInfo(recv_length, &node, node_addr_string, rtt_current);
          }
        }
      } else {
        break;
      }
    }
    while (!(ntohs(recv->id) == pthread_args->node_index));
  }

  // clean
  free(recv_buffer);

  pthread_exit(NULL);
}

/**
 * @return int
 */
void* handleUdpIpv6Receiving(void *threadarg) {
  struct pthread_args *pthread_args = (struct pthread_args *) threadarg;
  struct node node = (struct node) pthread_args->params->nodes[pthread_args->node_index];
  struct addrinfo *addrinfo = pthread_args->addrinfo;
  TParams* params = (TParams*) pthread_args->params;
  int sock = pthread_args->sock;

  socklen_t size;
  struct outdata_udp *recv;
  int recv_length;
  struct sockaddr_in6 receive_sock_addr;
  char* recv_buffer = (char *)calloc(IP_MAXPACKET, 1);
  fd_set my_set;
  char node_addr_string[INET6_ADDRSTRLEN];

  struct timeval tv;
  struct timeval *out, in;
  struct timeval evaluation_interval_tv;
  struct timeval summary_interval_tv;
  gettimeofday(&evaluation_interval_tv, NULL);
  gettimeofday(&summary_interval_tv, NULL);

  // received packets
  int recv_packets = 0;
  int recv_packets_loss_percent = 0;

  // received over rtt
  int recv_packets_exceeded_rtt_count = 0;
  int recv_packets_exceeded_rtt_percent = 0;

  // RTT Statistics
  double rtt_current = 0;
  double rtt_max = 0;
  double rtt_min = 1000000;
  double rtt_sum = 0;
  double rtt_sum2 = 0;
  double rtt_avg = 0;
  double rtt_avg2 = 0;
  double rtt_mdev = 0;

  // sockaddr_in6
  receive_sock_addr.sin6_family = AF_INET6;
  receive_sock_addr.sin6_addr = ((struct sockaddr_in6*)addrinfo)->sin6_addr;
  receive_sock_addr.sin6_port = htons(params->udp_port);

  // install signal handler for CTRL+C
  signal(SIGINT, catchsignal);

  while(true) {

    // CTRL+C handler
    if(G_break == 1) {
      break;
    }

    do {

      // summary info
      printSummaryInfo(pthread_args->packets_sent_count, recv_packets, rtt_max, rtt_min, rtt_avg, rtt_mdev, pthread_args, &node, &summary_interval_tv);

      // report info
      printReportInfo(pthread_args->packets_sent_count, recv_packets, recv_packets_exceeded_rtt_percent, recv_packets_exceeded_rtt_count, rtt_current, &node, params, &evaluation_interval_tv);

      FD_ZERO(&my_set);
      FD_SET(sock, &my_set);
      if(select(sock + 1, &my_set, NULL, NULL, &tv) < 0) {
        fprintf(stderr, "Select failed\n");
        break;
      }
      if(FD_ISSET(sock, &my_set)) {
        size = sizeof(struct sockaddr_in6);
        if((recv_length = recvfrom(sock, recv_buffer, IP_MAXPACKET, 0, (sockaddr *)&receive_sock_addr, &size)) == -1) {
          fprintf(stderr, "Receiving data error\n");
        }

        // get timestamp (when was received)
        gettimeofday(&in, NULL);

        recv = (struct outdata_udp *) (recv_buffer);

        if(recv->id == pthread_args->node_index) {
          out = (struct timeval *) (&recv->tv);

          // set up waiting time for packet (for first packet arg -w (2s default), for next 2xRTT)
          float response_timeout_sec = 2 * rtt_current;
          if(rtt_current == 0) {
            response_timeout_sec = params->response_timeout;
          }

          // is too late
          // lost packet
          if(1e3*(in.tv_sec - out->tv_sec + 1e-6*(in.tv_usec - out->tv_usec)) > response_timeout_sec) {
            break;
          }

          recv_packets++;

          // get node (destination) addr
          strdup(inet_ntop(AF_INET6, &(receive_sock_addr.sin6_addr.s6_addr), node_addr_string, INET6_ADDRSTRLEN));

          // re-calculate
          rtt_current = 1e3*(in.tv_sec - out->tv_sec + 1e-6*(in.tv_usec - out->tv_usec));
          rtt_max = (rtt_current > rtt_max)?rtt_current:rtt_max;
          rtt_min = (rtt_current < rtt_min)?rtt_current:rtt_min;
          rtt_sum += rtt_current;
          rtt_sum2 += pow(rtt_current, 2);
          rtt_avg = rtt_sum / recv_packets;
          rtt_avg2 = rtt_sum2 / recv_packets;
          rtt_mdev = sqrt(rtt_avg2 - pow(rtt_avg, 2));

          // report when is rtt_current higher than arg -r
          if((node.specific_rtt != -1 && rtt_current > node.specific_rtt) || (params->rtt_value != -1 && rtt_current > params->rtt_value)) {
            recv_packets_exceeded_rtt_count += 1;
            recv_packets_exceeded_rtt_percent = 100*(recv_packets_exceeded_rtt_count)/(recv_packets);
          }

          // verbose mode -> print incoming info of packets (like ping does)
          if(params->verbose_mode) {
            printVerboseInfo(recv_length, &node, node_addr_string, rtt_current);
          }
        }
      } else {
        break;
      }
    }
    while (!(recv->id == pthread_args->node_index));
  }

  // clean
  free(recv_buffer);

  pthread_exit(NULL);
}

/**
 * Clean mess when is program closing successfuly or with error.
 *
 * @return void
 */
void clean(TParams *params, Tpthread_args* threads_args[]) {
  for (int index = 0; index < params->nodes_count; index++) {

    // close sock
    close(threads_args[index]->sock);

    // free node addrinfo struct
    free(threads_args[index]->addrinfo);

    // delete ThreadArgs[] (created with new)
    delete threads_args[index];
  }
  // delete TNode[] (created with new)
  delete params->nodes;
}

/**
 * Entry point of application.
 *
 * @param int argc
 * @param char *argv[]
 *
 * @return int
 */
int main(int argc, char *argv[]) {
  int ecode = EOK;

  // parsing parameters
  TParams params = getParams(argc, argv);
  if(params.ecode != EOK) {
    return params.ecode;
  }

  // show help message
  if(params.show_help_message) {
    cout<<HELP_MSG<<endl;
    return ecode;
  }

  // udp server
  pthread_t thread;
  if(params.udp_listen_port != -1) {

    if(pthread_create(&thread, NULL, handleUdpServer, (void *)&params) != 0) {
      fprintf(stderr, "Error: unable to create thread with udp server enabled with argument -l.\n");
      ecode = ETHREAD_CREATE;
      return ecode;
    }
  }

  struct addrinfo hints;
  struct addrinfo *results;

  int index;
  pthread_t threads[params.nodes_count];
  Tpthread_args* threads_args[params.nodes_count];

  // multithread handling of nodes
  for (index = 0; index < params.nodes_count; index++) {

    // set structure for getaddrinfo
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_ADDRCONFIG;

    getaddrinfo(params.nodes[index].node, NULL, &hints, &results);

    // create Tpthread_args
    Tpthread_args* threadarg = new Tpthread_args();
    threadarg->params = &params;
    threadarg->packets_sent_count = 0;
    threadarg->node_index = index;
    threadarg->addrinfo = results;
    threads_args[index] = threadarg;

    // create sending add receiving thread per node

    // IPv6
    if(results->ai_family == AF_INET6) {

      // UDP
      if(params.udp_enable) {

        int sock;
        if((sock = socket(PF_INET6, SOCK_DGRAM, 0)) == -1) {
          fprintf(stderr, "Socket can not be created. Run program as sudo.\n");
          ecode = ESOCKET_CREATE;
          break;
        }
        threadarg->sock = sock;

        if(pthread_create(&threads[index], NULL, handleUdpIpv6Receiving, (void *) threadarg) != 0) {
          fprintf(stderr, "Error: unable to create thread %i.\n", index);
          ecode = ETHREAD_CREATE;
          break;
        }

        if(pthread_create(&threads[index], NULL, handleUdpIpv6Sending, (void *) threadarg) != 0) {
          fprintf(stderr, "Error: unable to create thread %i.\n", index);
          ecode = ETHREAD_CREATE;
          break;
        }

      // ICMP
      } else {

        // create socket
        int sock;
        if((sock = socket(PF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) == -1) {
          fprintf(stderr, "Socket can not be created. Run program as sudo.\n");
          ecode = ESOCKET_CREATE;
          break;
        }
        threadarg->sock = sock;

        if(pthread_create(&threads[index], NULL, handleIcmpIpv6Receiving, (void *) threadarg) != 0) {
          fprintf(stderr, "Error: unable to create thread %i.\n", index);
          ecode = ETHREAD_CREATE;
          break;
        }
        if(pthread_create(&threads[index], NULL, handleIcmpIpv6Sending, (void *) threadarg) != 0) {
          fprintf(stderr, "Error: unable to create thread %i.\n", index);
          ecode = ETHREAD_CREATE;
          break;
        }
      }
    }
    // IPv4
    else if(results->ai_family == AF_INET) {

      // UDP
      if(params.udp_enable) {

        // create socket
        int sock;
        if((sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
          fprintf(stderr, "Socket can not be created. Run program as sudo.\n");
          ecode = ESOCKET_CREATE;
          break;
        }
        threadarg->sock = sock;

        if(pthread_create(&threads[index], NULL, handleUdpIpv4Receiving, (void *) threadarg) != 0) {
          fprintf(stderr, "Error: unable to create thread %i.\n", index);
          ecode = ETHREAD_CREATE;
          break;
        }
        if(pthread_create(&threads[index], NULL, handleUdpIpv4Sending, (void *) threadarg) != 0) {
          fprintf(stderr, "Error: unable to create thread %i.\n", index);
          ecode = ETHREAD_CREATE;
          break;
        }

      // ICMP
      } else {

        // create socket
        int sock;
        if((sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
          fprintf(stderr, "Socket can not be created. Run program as sudo.\n");
          ecode = ESOCKET_CREATE;
          break;
        }
        threadarg->sock = sock;

        if(pthread_create(&threads[index], NULL, handleIcmpIpv4Receiving, (void *) threadarg) != 0) {
          fprintf(stderr, "Error: unable to create thread %i.\n", index);
          ecode = ETHREAD_CREATE;
          break;
        }
        if(pthread_create(&threads[index], NULL, handleIcmpIpv4Sending, (void *) threadarg) != 0) {
          fprintf(stderr, "Error: unable to create thread %i.\n", index);
          ecode = ETHREAD_CREATE;
          break;
        }
      }
    }
  }

  // wait for all child node threads
  for (index = 0; index < params.nodes_count; index++) {
    pthread_join(threads[index], NULL);
  }

  // wait for udp server thread
  if(params.udp_listen_port != -1) {
    pthread_join(thread, NULL);
  }

  // clean
  clean(&params, threads_args);

  return ecode;
}
