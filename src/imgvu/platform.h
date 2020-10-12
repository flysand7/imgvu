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

#endif //PLATFORM_H
