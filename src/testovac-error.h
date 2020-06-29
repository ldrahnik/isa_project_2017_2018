/**
 * Name: Lukáš Drahník
 * Project: ISA: Měření ztrátovosti a RTT (Matěj Grégr)
 * Date: 29.6.2020
 * Email: <xdrahn00@stud.fit.vutbr.cz>, <ldrahnik@gmail.com>
 */

#ifndef _testovac_error_H_
#define _testovac_error_H_

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

#endif
