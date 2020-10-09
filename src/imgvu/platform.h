/* date = October 6th 2020 3:36 pm */

#ifndef PLATFORM_H
#define PLATFORM_H

struct {
  t_string16 filename;
  t_string16 extension;
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

#endif //PLATFORM_H
