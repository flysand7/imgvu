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
} typedef t_directory_state;

// NOTE(bumbread): if the function below allocates additional 
// space, make sure to free that space when not needed
internal t_string16 get_full_filepath(t_string16 relativeName);
internal t_string16 get_file_extension(t_string16 anyName);
internal t_string16 get_short_filename(t_string16 relativeName);

#if 0
// TODO(bumbread): Think about caching and how
// it is affected by deleting the file while watching it.
internal void directory_start_listing(void);
internal void directory_list_next(void);
internal void directory_list_prev(void);
#endif


#endif //PLATFORM_H
