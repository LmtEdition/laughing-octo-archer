#ifndef PARALLEL_H
#define PARALLEL_H

#include <stdbool.h>

typedef struct {

  bool is_output;
  char* file_name;

} file;

typedef file* file_t;

#endif
