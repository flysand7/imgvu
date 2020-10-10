/* date = October 6th 2020 3:36 pm */

#ifndef PLATFORM_H
#define PLATFORM_H

// NOTE(bumbread): the pixel format is 0xAARRGGBB
struct {
  u32 width;
  u32 height;
  u32* pixels;
} typedef t_texture;

struct {
  t_string16 filename;
  t_string16 extension;
  void* data;
  u32 size;
  bool hasTexture;
  t_texture texture;
} typedef t_file_entry;

struct {
  u32 fileCount;
  u32 maxFiles;
  t_file_entry* files;
  u32 fileIndex;
} typedef t_directory_state;

// NOTE(bumbread): the api that *will* be used by the application
internal void request_next_image(t_directory_state*);
internal void request_prev_image(t_directory_state*);
internal void request_curr_image(t_directory_state*);

struct {
  t_texture texture;
  r32 posX;
  r32 posY;
  r32 scale;
  r32 angle;
} typedef t_image;

#endif //PLATFORM_H
