
#include <sys/time.h>

void timer(double *et) {
  
    struct timeval t;

  gettimeofday(&t, (struct timezone *)0);
  *et = t.tv_sec + t.tv_usec * 1.0e-6;
}