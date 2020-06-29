/**
 * Name: Lukáš Drahník
 * Project: ISA: Měření ztrátovosti a RTT (Matěj Grégr)
 * Date: 29.6.2020
 * Email: <xdrahn00@stud.fit.vutbr.cz>, <ldrahnik@gmail.com>
 */

#include "testovac-params.h"

/**
 * Get TParams structure from terminal options, option arguments and nodes.
 *
 * @return TParams
 */
TParams getParams(int argc, char *argv[]) {

  // default params
  TParams params = { 0, 0, -1, 300, 100, 2, -1, -1, -1, 0, EOK, 0, NULL };

  // strtol endptr
  char *end_ptr;

  // don't want getopt() writing to stderr
  opterr = 0;

  // getopt
  int c;
  while ((c = getopt(argc, argv, "hus:t:i:w:p:l:r:v")) != -1) {
    switch (c) {
      case 'h':
        params.show_help_message = 1;
        return params;
      case 'u':
        params.udp_enable = 1;
        break;
      case 's':
        params.size_of_data = strtol(optarg, &end_ptr, 10);
        if(*end_ptr != '\0' || *end_ptr == '\n' || params.size_of_data < 0) {
          fprintf (stderr, "Option -%c requires an non negative int.\n", c);
          params.ecode = EOPT;
        }
        break;
      case 't':
        params.evaluation_interval = strtol(optarg, &end_ptr, 10);
        if(*end_ptr != '\0' || *end_ptr == '\n' || params.evaluation_interval < 0) {
          fprintf (stderr, "Option -%c requires an non negative int.\n", c);
          params.ecode = EOPT;
        }
        break;
      case 'i':
        params.message_interval = strtol(optarg, &end_ptr, 10);
        if(*end_ptr != '\0' || *end_ptr == '\n' || params.message_interval < 0) {
          fprintf (stderr, "Option -%c requires an non negative int.\n", c);
          params.ecode = EOPT;
        }
        break;
      case 'w':
        params.response_timeout = strtol(optarg, &end_ptr, 10);
        if(*end_ptr != '\0' || *end_ptr == '\n' || params.response_timeout < 0) {
          fprintf (stderr, "Option -%c requires an non negative int.\n", c);
          params.ecode = EOPT;
        }
        break;
      case 'p':
        params.udp_port = strtol(optarg, &end_ptr, 10);
        if(*end_ptr != '\0' || *end_ptr == '\n' || params.udp_port < 0) {
          fprintf (stderr, "Option -%c requires an non negative int.\n", c);
          params.ecode = EOPT;
        }
        break;
      case 'l':
        params.udp_listen_port = strtol(optarg, &end_ptr, 10);
        if(*end_ptr != '\0' || *end_ptr == '\n' || params.udp_listen_port < 0) {
          fprintf (stderr, "Option -%c requires an non negative int.\n", c);
          params.ecode = EOPT;
        }
        break;
      case 'r':
        params.rtt_value = strtof(optarg, &end_ptr);
        if(*end_ptr != '\0' || *end_ptr == '\n' || params.rtt_value < 0) {
          fprintf (stderr, "Option -%c requires an non negative float.\n", c);
          params.ecode = EOPT;
        }
        break;
      case 'v':
        params.verbose_mode = 1;
        break;
      case '?':
        if(optopt == 's' || optopt == 't' || optopt == 'i' || optopt == 'w' || optopt == 'p' || optopt == 'l' || optopt == 'r') {
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        } else if(isprint (optopt)) {
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        } else {
          fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        }
        params.ecode = EOPT;
        break;
      default:
        fprintf (stderr, "At least one node is required.\n");
        params.ecode = EOPT;
    }
  }

  // no args - show help message
  if(argc == 1) {
     params.show_help_message = 1;
     return params;
  }

  // validation of combination of udp and udp port options
  if(params.udp_port == -1 && params.udp_enable == 1) {
    fprintf (stderr, "Option udp [-u] option requires option udp port [-p].\n");
    params.ecode = EOPT;
  }

  // default size of data
  if(params.size_of_data == -1) {
    if(params.udp_enable) {
      params.size_of_data = 64;
    } else {
      params.size_of_data = 56;
    }
  }

  // default nodes [count of non option args = argc - optind]
  int nodes_count = argc - optind;

  TNode* tNodes = new TNode[nodes_count];
  params.nodes = tNodes;
  int index;
  for (index = optind; index < argc; index++) {

    // get node
    TNode node = getNode(argv[index]);

    if(node.ecode != EOK) {
      params.ecode = node.ecode;
      break;
    }

    // add node
    params.nodes[params.nodes_count] = node;
    params.nodes_count++;
  }

  return params;
}

/**
 * Validate host.
 *
 * @return int
 */
int isValidHost(char* host) {
  struct addrinfo hints;
  struct addrinfo* results;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_RAW;
  hints.ai_protocol = 0;
  hints.ai_flags = AI_ADDRCONFIG;

  if(getaddrinfo(host, NULL, &hints, &results) != 0) {
    fprintf(stderr, "Host `%s` is not valid IPv4 / IPv6 / hostname.\n", host);
    return 1;
  }

  return 0;
}

/**
 * Get TNode structure from entered node in terminal. Validate if is given host valid.
 *
 * @return TNode
 */
TNode getNode(char* host) {
  TNode node = { host, EOK, -1 };

  // node name
  char *ptr;
  ptr = strrchr(host, HOST_RTT_DELIMETER);

  // there is <HOST_RTT_DELIMETER>
  if (ptr != NULL) {
    *ptr = '\0';

    // is after <HOST_RTT_DELIMETER> valid float?
    char *end_ptr;
    float specific_rtt = strtof(ptr + 1, &end_ptr);
    if(*end_ptr != '\0' || *end_ptr == '\n' || specific_rtt < 0) {
      fprintf(stderr, "RTT value inside node: %s requires to be an non negative float.\n", ptr + 1);
      node.ecode = ENODE;
    } else {
      node.specific_rtt = specific_rtt;
    }
  }

  // validate host
  if(isValidHost(host) != 0) {
    node.ecode = ENODE;
  }

  return node;
}
