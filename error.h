#ifndef ERROR_H
#define ERROR_H
 
#include <cstdio>
#include <cstdlib>

inline void error(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

#endif
