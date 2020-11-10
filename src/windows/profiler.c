
#include<intrin.h>
internal u64 clock_cycles(void) {
  return((u64)(__rdtsc()));
}

LARGE_INTEGER typedef t_clock;
global u64 clockFrequency;

internal void clock_setup(void) {
  LARGE_INTEGER resolution;
  QueryPerformanceFrequency(&resolution);
  clockFrequency = (u64)resolution.QuadPart;
}

internal t_clock clock_seconds(void) {
  t_clock result;
  QueryPerformanceCounter(&result);
  return(result);
}

internal r64 clock_diff(t_clock start, t_clock end) {
  assert(end.QuadPart > start.QuadPart);
  u64 difference = (u64)(end.QuadPart - start.QuadPart);
  r64 seconds = (r64)difference / (r64)clockFrequency;
  return(seconds);
}

#define profile_block_start(name) \
u64 cycles_##name##_start = clock_cycles();\
t_clock clock_##name##_start = clock_seconds()

#include<stdio.h>
#define profile_block_end(name)\
u64 cycles_##name##_end = clock_cycles();\
t_clock clock_##name##_end = clock_seconds();\
u64 cycles_##name = cycles_##name##_end - cycles_##name##_start;\
r64 seconds_##name = clock_diff(clock_##name##_start, clock_##name##_end);\
printf( #name " cycles:\t%lld cyc.\n", (i64)(cycles_##name));\
printf( #name " time:  \t%f sec.\n", seconds_##name)
