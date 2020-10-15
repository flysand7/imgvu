/* date = October 6th 2020 3:36 pm */

#ifndef PLATFORM_H
#define PLATFORM_H

struct {
  u64 size;
  void* ptr;
} typedef t_data;

struct {
  u32 width;
  u32 height;
  u32* pixels;
} typedef t_image;

struct t_directory_state_s;
internal void platform_directory_set(struct t_directory_state_s* state, t_string16 path);
internal void platform_directory_next_file(struct t_directory_state_s* state);
internal void platform_directory_previous_file(struct t_directory_state_s* state);

#endif //PLATFORM_H
