
struct t_file {
  t_file_data data;
  t_image image;
  
  struct t_file *next;
  struct t_file *prev;
  
  bool freed;
  bool found;
  bool cached;
} typedef t_file;

struct t_directory_state_s {
  t_string16 dirPath;
  t_string16 dirSearchPath;
  
  t_file *file;
  t_file *currentFile;
  
  bool changed;
} typedef t_directory_state;

internal inline void win32_set_current_file(t_directory_state *dirState, t_file *file) {
  dirState->currentFile = file;
  dirState->changed = true;
}

internal void win32_file_uncache(t_file *file) {
  assert(file);
  assert(file->cached);
  file->cached = false;
  {
    assert(file->data.ptr);
    free(file->data.ptr);
    file->data.ptr = 0;
    file->data.size = 0;
    
    if(file->image.pixels) {
      free(file->image.pixels);
      file->image.pixels = 0;
      file->image.width = 0;
      file->image.height = 0;
    }
  }
}

internal inline t_file *win32_file_create(t_string16 filename) {
  t_file *result = (t_file*)malloc(1 * sizeof(t_file));
  memset(result, 0, sizeof(t_file));
  result->data.filename = filename;
  return(result);
}

internal inline void win32_file_free(t_file *file) {
  assert(file != 0);
  
  if(file->data.ptr != 0) {
    free(file->data.ptr);
    file->data.ptr = 0;
    file->data.size = 0;
  }
  
  assert(file->image.pixels == 0);
  free(file);
}

// NOTE(bumbread): clear all files from cache
internal inline void win32_cache_clear(t_directory_state *dirState) {
  t_file *file = dirState->file;
  if(file == 0) {return;}
  loop {
    win32_file_uncache(file);
    win32_file_free(file);
    
    file = file->next;
    if(file == dirState->file) {break;}
  }
  dirState->file = 0;
  win32_set_current_file(dirState, 0);
}

// NOTE(bumbread): removes and uncaches all 
// the files form the directory.
internal inline void win32_directory_free_files(t_directory_state* dirState) {
  t_file *file = dirState->file;
  if(file == 0) return;
  loop {
    t_file *next = file->next;
    if(file->cached) {win32_file_uncache(file);}
    win32_file_free(file);
    file = next;
    if(file == dirState->file) {break;}
  }
  win32_set_current_file(dirState, 0);
}

// NOTE(bumbread): remove the file from directory and 
// uncache, if data loaded
internal void win32_directory_file_remove(t_directory_state *dirState, t_file *file) {
  assert(file != 0);
  bool isLastFile = file->prev == file;
  
  if(isLastFile == false) {
    assert(file->next != file);
    
    if(file == dirState->currentFile) { //we're removing the file we're viewing
      win32_set_current_file(dirState, dirState->currentFile->next);
    }
    if(file == dirState->file) {
      dirState->file = dirState->file->next;
    }
    
    t_file *left = file->prev;
    t_file *right = file->next;
    left->next = right;
    right->prev = left;
    
    if(file->cached) {win32_file_uncache(file);}
    win32_file_free(file);
  }
  else { // this is the last file
    assert(file->next == file);
    if(file->cached) {win32_file_uncache(file);}
    win32_file_free(file);
    dirState->file = 0;
    win32_set_current_file(dirState, 0);
  }
  dirState->changed = true;
}

internal inline void win32_file_insert(t_file *left, t_file *file, t_file* right) {
  assert(left->next == right);
  assert(right->prev == left);
  left->next = file; file->prev = left;
  right->prev = file; file->next = right;
}

// NOTE(bumbread): load the file into the memory, 
// decode the file and mark as cached
internal bool win32_load_cache_file(t_file *file) {
  assert(file->cached == false);
  assert(file->data.ptr == 0);
  assert(file->image.pixels == 0);
  
  // NOTE(bumbread): loading into the cache
  t_file_data fileData = platform_load_file(file->data.filename);
  if(fileData.ptr != 0) {
    file->data = fileData;
    if(file->data.size != 0) {
      
      file->image = app_decode_file(file->data);
      if(file->image.success == true) {
        file->cached = true;
        return(true);
      }
      else {
        assert(file->image.pixels == 0);
        return(false);
      }
      
    }
  }
  
  return(false);
}

#define max_cache_distance_loaded 1

// NOTE(bumbread): load the files that should be in the cache
// into the cache, unload the files that should not be in the cache
// out of the cache.
internal void win32_cache_update(t_directory_state *dirState) {
  
  t_file *file = dirState->currentFile;
  if(file == 0) return;
  
  t_file *last = file->prev;
  bool exit = false;
  
  loop {
    t_file *next = file->next;
    
    if(file->cached == false) {
      win32_load_cache_file(file);
      if(file->cached == false) {
        bool isLastFile = (file->next == file);
        win32_directory_file_remove(dirState, file);
        if(isLastFile) {
          break;
        }
      }
    }
    
    if(exit) {break;}
    file = next;
    if(file == last) {exit = true;}
  }
  
  file = dirState->currentFile;
  if(file == 0) return;
  
  t_file *left = file->prev;
  t_file *right = file->next;
  
  u32 distance = 1;
  loop {
    if(left->prev == right) {
      assert(right->next == left);
      break;
    }
    
    t_file *rightAdvanceTo = right->next;
    t_file *leftAdvanceTo = left->prev;
    
    if(distance <= max_cache_distance_loaded) {
      if(left->cached == false)  {
        win32_load_cache_file(left);
        if(left->cached == false) {
          bool isLastFile = (left->next == left);
          win32_directory_file_remove(dirState, left);
          if(isLastFile) { 
            break;
          }
        }
      }
      if(right->cached == false) {
        win32_load_cache_file(right);
        if(right->cached == false) {
          bool isLastFile = (left->next == left);
          win32_directory_file_remove(dirState, right);
          if(isLastFile) { 
            break;
          }
        }
      }
    }
    else {
      if(left->cached)  {win32_file_uncache(left);}
      if(right->cached) {win32_file_uncache(right);}
    }
    
    right = rightAdvanceTo;
    left = leftAdvanceTo;
  }
}

// NOTE(bumbread): returns the pointer to file with specified name or zero, if doesn't exists
internal t_file *win32_directory_find_file(t_directory_state *dirState, t_string16 filename) {
  t_file *file = dirState->file;
  if(file == 0) return(0);
  loop {
    if(string16_compare(file->data.filename, filename)) {
      return(file);
    }
    file = file->next;
    if(file == dirState->file) {break;}
  }
  return(0);
}

// TODO(bumbread): chose file location based on some sorting comparator
internal bool win32_order_is_alphabetical(t_string16 first, t_string16 second) {
  debug_variable_unused(first);
  debug_variable_unused(second);
  return(true);
}

internal t_file *win32_directory_add(t_directory_state *dirState, t_string16 filename) {
  assert(win32_directory_find_file(dirState, filename) == 0);
  t_file *file = win32_file_create(filename);
  
  // if none exist, make new one linked to itself
  if(dirState->file == 0) {
    dirState->file = file;
    file->next = file;
    file->prev = file;
  }
  else {
    t_file *it = dirState->file;
    loop {
      // if the current file is before the given one
      if(win32_order_is_alphabetical(file->data.filename, it->data.filename)) {
        win32_file_insert(it->prev, file, it);
        break;
      }
      // we looped around, insert at the end.
      else if(it->next == dirState->file) {
        win32_file_insert(dirState->file->prev, file, dirState->file);
        dirState->file = file;
        break;
      }
      it = it->next;
      //make sure we don't cycle more than once, without
      // finding the place for the file.
      assert(it != dirState->file);
    }
  }
  
  dirState->changed = true;
  return(file);
}

internal void win32_directory_scan(t_directory_state* dirState) {
  
  assert(dirState->dirPath.ptr);
  assert(dirState->dirSearchPath.ptr);
  
  WIN32_FIND_DATAW findData = {0};
  HANDLE searchHandle = FindFirstFileW((LPCWSTR)dirState->dirSearchPath.ptr, &findData);
  if(searchHandle != INVALID_HANDLE_VALUE) {
    do {
      bool isDirectory = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
      if(!isDirectory) {
        t_string16 shortName = char16_count((char16*)findData.cFileName);
        t_string16 fullName = win32_get_file_path_from_relative_mem(dirState->dirPath, shortName);
        
        t_file* file = win32_directory_find_file(dirState, fullName);
        if(file == 0) { //file not found
          file = win32_directory_add(dirState, fullName);
          assert(file != 0);
        }
        
        file->found = true;
      }
    } while(FindNextFileW(searchHandle, &findData));
    FindClose(searchHandle);
  }
  else {
    // TODO(bumbread): handle this case
  }
  
  t_file *it = dirState->file;
  if(it != 0) {
    loop {
      bool isLastFile = (it->next == it);
      t_file *itNext = it->next;
      
      if(!it->found) {
        win32_directory_file_remove(dirState, it);
        if(isLastFile) {break;}
      }
      else {
        it->found = false;
      }
      
      it = itNext;
      if(it == dirState->file) {break;}
    }
  }
  
  win32_set_current_file(dirState, dirState->file);
  win32_cache_update(dirState);
  
}
