#include "cwf/arguments.h"

#include <string.h>

namespace cwf {

int argc = 0;
char **argv = 0;

void Copy(int ac, char **av) {
  argv = new char*[ac];
  for (int i=0; i<ac; ++i) {
    argv[i] = strdup(av[i]);
  }
  argc = ac;
}

}
