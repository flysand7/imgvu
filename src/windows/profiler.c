
struct t_clock {
  u64 count;
};

struct {
  char const *name;
  struct t_clock clockStart;
  u64 cyclesStart;
} typedef t_profile_state;

// globals
global u64 clockFrequency;

struct {
  u32 count;
  t_profile_state states[20];
} global profileStack;

internal u64 clock_cycles(void) {
  return((u64)(__rdtsc()));
}

internal void win_clock_setup(void) {
  LARGE_INTEGER resolution;
  QueryPerformanceFrequency(&resolution);
  clockFrequency = (u64)resolution.QuadPart;
  profileStack.count = 0;
}

internal struct t_clock platform_clock(void) {
  struct t_clock result;
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  result.count = (u64)counter.QuadPart;
  return(result);
}

internal r64 platform_clock_diff(struct t_clock end, struct t_clock start) {
  assert(end.count >= start.count);
  u64 difference = (u64)(end.count - start.count);
  r64 seconds = (r64)difference / (r64)clockFrequency;
  return(seconds);
}

internal void platform_profile_state_push(char const *name) {
  assert(profileStack.count+1 <= 20);
  t_profile_state state;
  state.name = name;
  state.clockStart = platform_clock();
  state.cyclesStart = clock_cycles();
  profileStack.states[profileStack.count] = state;
  profileStack.count += 1;
}

internal void platform_profile_state_pop(void) {
  assert(profileStack.count > 0);
  profileStack.count -= 1;
  struct t_clock currentTime = platform_clock();
  u64 currentCycleCounter = clock_cycles();
  t_profile_state state = profileStack.states[profileStack.count];
  r64 time = platform_clock_diff(currentTime, state.clockStart);
  u64 cycles = currentCycleCounter - state.cyclesStart;
  for(u32 _=0;_<profileStack.count;_+=1) {printf(" ");}
  printf("name: %s, seconds: %f, cycles: %lld\n", state.name, time, (i64)cycles);
}

internal void platform_profile_pop_all(void) {
  while(profileStack.count > 0) {
    platform_profile_state_pop();
  }
}
