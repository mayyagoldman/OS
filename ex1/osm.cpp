#include "osm.h"
#include <cmath>
#include <sys/time.h>
#include <iostream>

/* Time measurement function for a simple arithmetic operation.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_operation_time(unsigned int iterations)
{
  struct timeval start_time, finish_time;
  if (iterations == 0) {return -1;}
  int dummy0 = 0 , dummy1 = 0, unrollingFactor = 100;
  iterations = ceil (iterations / unrollingFactor);
  int normalize = iterations * unrollingFactor; // total num of iterations
  double totalDuration = 0;
  gettimeofday(&start_time, nullptr);
  for (int i = 0 ; i < iterations ; i++)
    {
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;
      dummy0 = dummy1 + 1;

    }
  gettimeofday(&finish_time,nullptr);
  totalDuration += \
      (double)(finish_time.tv_sec - start_time.tv_sec) * 1000000000L + \
      (double)(finish_time.tv_usec - start_time.tv_usec) * 1000L;

  return totalDuration / normalize;
}

void empty_func(){}

/* Time measurement function for an empty function call.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_function_time(unsigned int iterations)
{
  struct timeval start_time, finish_time;
  if (iterations == 0) {return -1;}
  int unrollingFactor = 100;
  iterations = ceil (iterations / unrollingFactor);
  int normalize = iterations * unrollingFactor;
  double totalDuration = 0;

  gettimeofday(&start_time, nullptr);
  for (int i = 0 ; i < iterations ; i++)
    {
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();

    }
  gettimeofday(&finish_time,nullptr);
  totalDuration += \
      (double)(finish_time.tv_sec - start_time.tv_sec) * 1000000000L \
      + (double)(finish_time.tv_usec - start_time.tv_usec) * 1000L;
  return totalDuration / normalize;
}

/* Time measurement function for an empty trap into the operating system.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_syscall_time(unsigned int iterations)
{
  struct timeval start_time, finish_time;
  if (iterations == 0) {return -1;}
  int unrollingFactor = 100;
  iterations = ceil (iterations / unrollingFactor);
  int normalize = iterations * unrollingFactor;
  double totalDuration = 0;
  gettimeofday(&start_time, nullptr);
  for (int i = 0 ; i < iterations ; i++)
    {
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
    }
  gettimeofday(&finish_time,nullptr);
  totalDuration += \
      (double)(finish_time.tv_sec - start_time.tv_sec)* 1000000000L \
      + (double)(finish_time.tv_usec - start_time.tv_usec) * 1000L;

  return totalDuration / normalize;
}