/* date = October 6th 2020 3:36 pm */

#ifndef PLATFORM_H
#define PLATFORM_H

struct {
  t_string16 filename;
  t_string16 extension;
} typedef t_file_entry;

struct {
  u32 fileCount;
  t_file_entry* files;
  u32 fileIndex;
} typedef t_directory_state;

// TODO(bumbread): does the application even need to know about
// the existance of the three functions below?
// NOTE(bumbread): if the function below allocates additional 
// space, make sure to free that space when not needed
internal t_string16 get_full_filepath(t_string16 relativeName);
internal t_string16 get_file_extension(t_string16 anyName);
internal t_string16 get_short_filename(t_string16 relativeName);

// NOTE(bumbread): the api that *will* be used by the application
internal void request_next_image(void);
internal void request_prev_image(void);

#endif //PLATFORM_H
