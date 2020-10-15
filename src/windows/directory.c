
struct {
  t_string16 name;
  t_data data;
  t_image image;
  
  bool found;
  bool cached;
  u32 cacheLoads;
} typedef t_file;

struct t_directory_state_s {
  t_string16 dirPath;
  t_string16 dirSearchPath;
  
  u32 currentFile;
  u32 fileCount;
  u32 filesAllocated;
  t_file* files;
  
  bool changed;
} typedef t_directory_state;

// TODO(bumbread): some of the methods operating on directories should be up in the platform layer
#if 0
internal inline i32 win32_directory_ring_distance(t_directory_state* state, u32 start, u32 end) {
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
#endif

internal inline u32 win32_directory_ring_distance(t_directory_state* state, u32 a, u32 b) {
  i32 distance = (i32)a - (i32)b;
  distance = (distance >= 0) ? (distance) : (-distance);
  u32 remainingSpace = state->fileCount - distance;
  return((remainingSpace < (u32)distance) ? (remainingSpace) : ((u32)distance));
}

internal void win32_free_file(t_file* file) {
  assert(file->data.ptr);
  assert(file->image.pixels);
  
  free(file->data.ptr);
  file->data.ptr = 0;
  file->data.size = 0;
  free(file->image.pixels);
  file->image.pixels = 0;
  file->image.width = 0;
  file->image.height = 0;
}

internal void win32_cache_clear(t_directory_state* state) {
  for(u32 fileIndex = 0; fileIndex < state->fileCount; fileIndex += 1) {
    t_file* fileToFree = state->files + fileIndex;
    fileToFree->cached = false;
    fileToFree->cacheLoads = false;
    win32_free_file(fileToFree);
  }
}

// NOTE(bumbread): caching policy:
//
// A file is cached if and only if:
//  it is next to the current file in the directory
//  it is frequently used (4*freq > max)

internal bool win32_cache_is_frequent_enough(u32 cacheLoads, u32 maxCacheLoads) {
  return(4*cacheLoads > maxCacheLoads);
}

internal void win32_cache_add(t_directory_state* state, u32 fileIndex) {
  assert(fileIndex < state->fileCount);
  
  t_file* file = state->files + fileIndex;
  if(!file->cached) {
    assert(file->data.ptr == 0);
    assert(file->image.pixels == 0);
    
    // NOTE(bumbread): loading into the cache
    HANDLE fileHandle = CreateFileW((LPCWSTR)file->name.ptr, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    // TODO(bumbread): if file not found, remove the file desc from directory
    assert(fileHandle != INVALID_HANDLE_VALUE);
    
    LARGE_INTEGER fileSize;
    bool result = GetFileSizeEx(fileHandle, &fileSize);
    assert(result);
    assert(fileSize.LowPart != 0);
    void* fileData = malloc((u32)fileSize.LowPart);
    DWORD bytesRead = 0;
    
    result = ReadFile(fileHandle, fileData, fileSize.LowPart, &bytesRead, 0);
    assert(result);
    CloseHandle(fileHandle);
    assert((DWORD)fileSize.LowPart == bytesRead);
    file->data.ptr = fileData;
    file->data.size = (u32)fileSize.LowPart;
    file->image = app_decode_file(file->data);
    
    // NOTE(bumbread): updaing the cache
    file->cached = true;
    file->cacheLoads += 1;
  }
}

internal void win32_cache_remove(t_directory_state* state, u32 fileIndex) {
  t_file* currentFile = state->files + fileIndex;
  assert(!currentFile->cached);
  currentFile->cached = false;
  currentFile->cacheLoads = 0;
  win32_free_file(currentFile);
}

internal void win32_cache_update(t_directory_state* state) {
  u32 maxCacheLoads = 0;
  for(u32 fileIndex = 0; fileIndex < state->fileCount; fileIndex += 1) {
    u32 currentCacheLoads = state->files[fileIndex].cacheLoads;
    if(currentCacheLoads > maxCacheLoads) maxCacheLoads = currentCacheLoads;
  }
  
  for(u32 fileIndex = 0; fileIndex < state->fileCount; fileIndex += 1) {
    t_file* file = state->files + fileIndex;
    u32 currentCacheLoads = file->cacheLoads;
    if(win32_cache_is_frequent_enough(currentCacheLoads, maxCacheLoads)) {
      win32_cache_add(state, fileIndex);
    }
    else {
      if(win32_directory_ring_distance(state, fileIndex, state->currentFile) > 1) {
        if(file->cached) {
          win32_cache_remove(state, fileIndex);
        }
      }
    }
  }
}

internal bool win32_cache_request_add(t_directory_state* state, u32 fileIndex) {
  if(win32_directory_ring_distance(state, fileIndex, state->currentFile) <= 1) {
    win32_cache_add(state, fileIndex);
  }
  return(false);
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
  
  if(state->fileCount != 0) {
    if(newIndex <= state->currentFile) state->currentFile += 1;
    for(u32 index = state->fileCount; index > newIndex; index -= 1) {
      state->files[index] = state->files[index - 1];
    }
  }
  
  t_file* newFile = state->files + newIndex;
  t_file emptyFile = {0};
  emptyFile.name = filename;
  *newFile = emptyFile;
  state->fileCount += 1;
  
  win32_cache_request_add(state, newIndex);
  return(newFile);
}

internal void win32_directory_remove(t_directory_state* state, u32 index) {
  assert(state->fileCount != 0);
  
  t_file* file = state->files + index;
  win32_free_file(file);
  assert(file->name.ptr);
  free(file->name.ptr);
  file->name.ptr = 0;
  file->name.len = 0;
  file->cacheLoads = 0;
  file->found = false;
  
  if(index < state->currentFile) {
    if(state->currentFile == 0) state->currentFile += state->fileCount;
    state->currentFile -= 1;
  }
  
  win32_cache_remove(state, index);
  
  for(u32 fileIndex = index; fileIndex < state->fileCount - 1; fileIndex += 1) {
    state->files[fileIndex] = state->files[fileIndex + 1];
  }
  state->fileCount -= 1;
  t_file emptyFile = {0};
  state->files[state->fileCount] = emptyFile;
}

internal void win32_directory_clear(t_directory_state* state) {
  win32_cache_clear(state);
  for(u32 index = 0; index < state->fileCount; index += 1) {
    t_file* file = state->files + index;
    assert(file->name.ptr);
    free(file->name.ptr);
    file->name.ptr = 0;
    file->name.len = 0;
    file->cacheLoads = 0;
    file->found = false;
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
  
  for(u32 fileIndex = 0; fileIndex < state->fileCount; fileIndex += 1) {
    state->files[fileIndex].found = false;
  }
  
  WIN32_FIND_DATAW findData = {0};
  HANDLE searchHandle = FindFirstFileW((LPCWSTR)state->dirSearchPath.ptr, &findData);
  if(searchHandle != INVALID_HANDLE_VALUE) {
    do {
      bool isDirectory = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
      if(!isDirectory) {
        t_string16 shortName = char16_count((char16*)findData.cFileName);
        t_string16 fullName = win32_get_file_path_mem(shortName);
        t_file* file = win32_directory_find_file(state, fullName);
        if(file == 0) {
          file = win32_directory_add(state, fullName);
        }
        assert(file != 0);
        file->found = true;
      }
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
      win32_cache_request_add(state, fileIndex);
      fileIndex += 1;
    }
  }
}

// NOTE(bumbread): The following are the platform layer API function
// realisations

internal void platform_directory_set(t_directory_state* state, t_string16 path) {
  if(!string_compare(path, state->dirPath)) {
    win32_directory_clear(state);
    
    if(state->dirSearchPath.ptr) free(state->dirSearchPath.ptr);
    if(state->dirPath.ptr) free(state->dirPath.ptr);
    
    state->dirPath = path;
    state->dirSearchPath = win32_make_path_wildcard_mem(state->dirPath);
    win32_directory_scan(state);
  }
}

// TODO(bumbread): handle the case when the file is not found
internal void platform_directory_next_file(t_directory_state* state) {
  u32 newFileIndex = (state->currentFile+1) % state->fileCount;
  if(newFileIndex != state->currentFile) {
    win32_cache_update(state);
    state->currentFile = newFileIndex;
    state->changed = true;
  }
}

internal void platform_directory_previous_file(t_directory_state* state) {
  u32 newFileIndex = state->currentFile;
  if(newFileIndex == 0) newFileIndex += state->fileCount;
  newFileIndex -= 1;
  if(newFileIndex != state->currentFile) {
    win32_cache_update(state);
    state->currentFile = newFileIndex;
    state->changed = true;
  }
}
