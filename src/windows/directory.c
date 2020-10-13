
struct {
  t_string16 name;
  t_data data;
  t_image image;
  bool found;
} typedef t_file;

struct {
  t_string16 dirPath;
  t_string16 dirSearchPath;
  
  u32 cacheOffset;
  u32 currentFile;
  
  u32 fileCount;
  u32 filesAllocated;
  t_file* files;
} typedef t_directory_state;

internal i32 win32_directory_ring_distance(t_directory_state* state, u32 start, u32 end) {
  i32 distance = (i32)end - (i32)start;
  i32 sign = distance >= 0 ? 1 : -1;
  u32 absDistance = (distance >= 0) ? (distance) : (-distance);
  u32 revDistance = state->fileCount - absDistance;
  
  if(absDistance >= revDistance) {
    return(-(i32)revDistance*sign);
  }
  else {
    return((i32)absDistance*sign);
  }
}

internal void win32_free_file(t_file* file) {
  if(file->data.ptr) {
    free(file->data.ptr);
    file->data.ptr = 0;
    file->data.size = 0;
  }
  if(file->image.pixels) {
    free(file->image.pixels);
    file->image.pixels = 0;
    file->image.width = 0;
    file->image.height = 0;
  }
}

internal void win32_directory_cache_file(t_file* file) {
  // TODO(bumbread): write the cache in logic here
}

internal t_file* win32_directory_add(t_directory_state* state, t_string16 filename) {
  if(state->fileCount + 1 > state->filesAllocated) {
    state->filesAllocated *= 2;
    if(state->filesAllocated == 0) state->filesAllocated = 1;
    state->files = (t_file*)realloc(state->files, state->filesAllocated * sizeof(t_file));
  }
  // TODO(bumbread): chose index based 
  // on sorting constraints
  u32 newIndex = state->fileCount/2;
  if(newIndex <= state->currentFile) {
    state->currentFile += 1;
  }
  
  for(u32 index = state->fileCount-1; index >= newIndex+1; index -= 1) {
    state->files[index] = state->files[index - 1];
  }
  
  t_file* newFile = state->files + newIndex;
  t_file emptyFile = {0};
  emptyFile.name = filename;
  *newFile = emptyFile;
  state->fileCount += 1;
  
  {
    i32 dist = win32_directory_ring_distance(state, newIndex, state->currentFile);
    u32 absDist = (dist >= 0) ? (u32)(dist) : (u32)(-dist);
    if(absDist <= state->cacheOffset) {
      u32 excludedFile = 0;
      assert(dist != 0);
      if(dist > 0) {
        if(state->currentFile < state->cacheOffset+1) excludedFile += state->fileCount;
        excludedFile -= state->cacheOffset;
        excludedFile -= 1;
      }
      else {
        excludedFile += state->cacheOffset;
        excludedFile += 1;
        if(excludedFile >= state->fileCount) excludedFile -= state->fileCount;
      }
      t_file* file = state->files + excludedFile;
      win32_free_file(file);
      
      win32_directory_cache_file(newFile);
    }
  }
  
  return(newFile);
}

internal void win32_directory_remove(t_directory_state* state, u32 index) {
  t_file* file = state->files + index;
  win32_free_file(file);
  assert(file->name.ptr);
  free(file->name.ptr);
  file->name.ptr = 0;
  file->name.len = 0;
  
  if(index < state->currentFile) {
    if(state->currentFile == 0) state->currentFile += state->fileCount;
    state->currentFile -= 1;
  }
  
  {
    i32 dist = win32_directory_ring_distance(state, index, state->currentFile);
    u32 absDist = (dist >= 0) ? (u32)(dist) : (u32)(-dist);
    if(absDist <= state->cacheOffset) {
      assert(dist != 0);
      u32 addedIndex = state->currentFile;
      if(dist > 0) {
        if(addedIndex < (state->cacheOffset+1)) {
          addedIndex += state->fileCount;
        }
        addedIndex -= state->cacheOffset;
        addedIndex -= 1;
      }
      else {
        addedIndex += state->cacheOffset;
        addedIndex += 1;
        if(addedIndex >= state->fileCount) {
          addedIndex -= state->fileCount;
        }
      }
      
      t_file* cacheInFile = state->files + addedIndex;
      win32_directory_cache_file(cacheInFile);
    }
  }
  
  for(u32 fileIndex = index; fileIndex < state->fileCount - 1; fileIndex += 1) {
    state->files[fileIndex] = state->files[fileIndex + 1];
  }
  state->fileCount -= 1;
  t_file emptyFile = {0};
  state->files[state->fileCount] = emptyFile;
}

internal void win32_directory_clear(t_directory_state* state) {
  for(u32 index = 0; index < state->fileCount; index += 1) {
    t_file* file = state->files + index;
    assert(file->name.ptr);
    free(file->name.ptr);
    file->name.ptr = 0;
    file->name.len = 0;
    
    win32_free_file(file);
  }
  state->fileCount = 0;
}

internal t_file* win32_directory_find_file(t_directory_state* state, t_string16 filename) {
  for(u32 fileIndex = 0; fileIndex < state->fileCount; fileIndex += 1) {
    if(string_compare(state->files[fileIndex].name, filename)) {
      return(state->files + fileIndex);
    }
  }
  return(0);
}

internal void win32_directory_scan(t_directory_state* state) {
  assert(state->dirPath.ptr);
  assert(state->dirSearchPath.ptr);
  
  WIN32_FIND_DATAW findData = {0};
  HANDLE searchHandle = FindFirstFileW((LPCWSTR)state->dirSearchPath.ptr, &findData);
  if(searchHandle != INVALID_HANDLE_VALUE) {
    do {
      t_string16 shortName = char16_count((char16*)findData.cFileName);
      t_string16 fullName = win32_get_file_path_mem(shortName);
      
      t_file* file = win32_directory_find_file(state, fullName);
      if(file == 0) {
        file = win32_directory_add(state, fullName);
      }
      assert(file != 0);
      file->found = true;
    } while(FindNextFileW(searchHandle, &findData));
    FindClose(searchHandle);
  }
  else {
    // TODO(bumbread): handle this case
  }
  
  for(u32 fileIndex = 0; fileIndex < state->fileCount;) {
    t_file* file = state->files + fileIndex;
    if(!file->found) {
      win32_directory_remove(state, fileIndex);
    }
    else {
      fileIndex += 1;
    }
  }
}

internal void win32_directory_set(t_directory_state* state, t_string16 path) {
  if(!string_compare(path, state->dirPath)) {
    win32_directory_clear(state);
    
    assert(state->dirSearchPath.ptr);
    assert(state->dirPath.ptr);
    
    free(state->dirSearchPath.ptr);
    free(state->dirPath.ptr);
    state->dirPath = path;
    state->dirSearchPath = win32_make_path_wildcard_mem(state->dirPath);
    win32_directory_scan(state);
  }
}
