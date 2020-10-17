
struct {
  t_image_data data;
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
internal inline u32 win32_directory_ring_distance(t_directory_state* dirState, u32 a, u32 b) {
  i32 distance = (i32)a - (i32)b;
  distance = (distance >= 0) ? (distance) : (-distance);
  u32 remainingSpace = dirState->fileCount - (u32)distance;
  return((remainingSpace < (u32)distance) ? (remainingSpace) : ((u32)distance));
}

internal void win32_free_file(t_file* file) {
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

internal void win32_cache_clear(t_directory_state* dirState) {
  for(u32 fileIndex = 0; fileIndex < dirState->fileCount; fileIndex += 1) {
    t_file* fileToFree = dirState->files + fileIndex;
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

internal void win32_cache_remove(t_directory_state* dirState, u32 fileIndex) {
  t_file* currentFile = dirState->files + fileIndex;
  assert(!currentFile->cached);
  currentFile->cached = false;
  currentFile->cacheLoads = 0;
  win32_free_file(currentFile);
}

internal void win32_directory_remove(t_directory_state* dirState, u32 index) {
  assert(dirState->fileCount != 0);
  
  t_file* file = dirState->files + index;
  win32_cache_remove(dirState, index);
  assert(file->data.filename.ptr);
  free(file->data.filename.ptr);
  file->data.filename.ptr = 0;
  file->data.filename.len = 0;
  file->cacheLoads = 0;
  file->found = false;
  
  if(index < dirState->currentFile) {
    if(dirState->currentFile == 0) dirState->currentFile += dirState->fileCount;
    dirState->currentFile -= 1;
  }
  
  for(u32 fileIndex = index; fileIndex < dirState->fileCount - 1; fileIndex += 1) {
    dirState->files[fileIndex] = dirState->files[fileIndex + 1];
  }
  dirState->fileCount -= 1;
  t_file emptyFile = {0};
  dirState->files[dirState->fileCount] = emptyFile;
}

internal bool win32_cache_add(t_directory_state* dirState, u32 fileIndex) {
  assert(fileIndex < dirState->fileCount);
  
  t_file* file = dirState->files + fileIndex;
  assert(!file->cached);
  assert(file->data.ptr == 0);
  assert(file->image.pixels == 0);
  
  // NOTE(bumbread): loading into the cache
  HANDLE fileHandle = CreateFileW((LPCWSTR)file->data.filename.ptr, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if(fileHandle != INVALID_HANDLE_VALUE) {
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
    
    // NOTE(bumbread): updaing the cache
    file->data.ptr = fileData;
    file->data.size = (u32)fileSize.LowPart;
    file->image = app_decode_file(file->data);
    if(!file->image.skip) {
      file->cached = true;
      file->cacheLoads += 1;
    }
    else {
      win32_directory_remove(dirState, fileIndex);
      return(true);
    }
  }
  else {
    win32_directory_remove(dirState, fileIndex);
    return(true);
  }
  return(false);
}

internal void win32_cache_update(t_directory_state* dirState) {
  // NOTE(bumbread): updating the cache by load frequency
  u32 maxCacheLoads = 0;
  for(u32 fileIndex = 0; fileIndex < dirState->fileCount;) {
    u32 currentCacheLoads = dirState->files[fileIndex].cacheLoads;
    if(currentCacheLoads > maxCacheLoads) maxCacheLoads = currentCacheLoads;
  }
  
  u32 fileIndex = 0;
  loop {
    t_file* file = dirState->files + fileIndex;
    
    u32 currentCacheLoads = file->cacheLoads;
    bool shouldBeCached = win32_cache_is_frequent_enough(currentCacheLoads, maxCacheLoads);
    bool isCached = file->cached;
    
    if(!isCached && shouldBeCached) {
      win32_cache_add(dirState, fileIndex);
    }
    else if(isCached && !shouldBeCached){
      win32_cache_remove(dirState, fileIndex);
    }
    
    fileIndex += 1;
    if(fileIndex == dirState->fileCount) break;
  }
  
  // NOTE(bumbread): updating the cache by neighbouring
  u32 lastLeftNeighbour;
  u32 lastRightNeighbour;
  
  //   NOTE(bumbread): left
  lastRightNeighbour = (dirState->currentFile + 1) % dirState->fileCount;
  loop {
    lastLeftNeighbour = (dirState->fileCount + dirState->currentFile - 1) % dirState->fileCount;
    if(lastLeftNeighbour == lastRightNeighbour) break;
    
    t_file* left = dirState->files + lastLeftNeighbour;
    if(!left->cached) {
      bool repeat = win32_cache_add(dirState, lastLeftNeighbour);
      if(!repeat) break;
    }
    else break;
  }
  
  //   NOTE(bumbread): right
  lastLeftNeighbour = (dirState->fileCount + dirState->currentFile - 1) % dirState->fileCount;
  loop {
    lastRightNeighbour = (dirState->currentFile + 1) % dirState->fileCount;
    if(lastRightNeighbour == lastLeftNeighbour) break;
    
    t_file* right = dirState->files + lastRightNeighbour;
    if(!right->cached) {
      bool repeat = win32_cache_add(dirState, lastRightNeighbour);
      if(!repeat) break;
    }
    else break;
  }
}

internal bool win32_cache_request_add(t_directory_state* dirState, u32 fileIndex) {
  if(win32_directory_ring_distance(dirState, fileIndex, dirState->currentFile) <= 1) {
    win32_cache_add(dirState, fileIndex);
  }
  return(false);
}

internal t_file* win32_directory_add(t_directory_state* dirState, t_string16 filename) {
  if(dirState->fileCount + 1 > dirState->filesAllocated) {
    dirState->filesAllocated *= 2;
    if(dirState->filesAllocated == 0) dirState->filesAllocated = 1;
    dirState->files = (t_file*)realloc(dirState->files, dirState->filesAllocated * sizeof(t_file));
  }
  // TODO(bumbread): chose index based 
  // on sorting constraints
  u32 newIndex = dirState->fileCount/2;
  
  if(dirState->fileCount != 0) {
    if(newIndex <= dirState->currentFile) dirState->currentFile += 1;
    for(u32 index = dirState->fileCount; index > newIndex; index -= 1) {
      dirState->files[index] = dirState->files[index - 1];
    }
  }
  
  t_file* newFile = dirState->files + newIndex;
  t_file emptyFile = {0};
  emptyFile.data.filename = filename;
  *newFile = emptyFile;
  dirState->fileCount += 1;
  
  win32_cache_request_add(dirState, newIndex);
  return(newFile);
}

internal void win32_directory_clear(t_directory_state* dirState) {
  win32_cache_clear(dirState);
  for(u32 index = 0; index < dirState->fileCount; index += 1) {
    t_file* file = dirState->files + index;
    assert(file->data.filename.ptr);
    free(file->data.filename.ptr);
    file->data.filename.ptr = 0;
    file->data.filename.len = 0;
    file->cacheLoads = 0;
    file->found = false;
  }
  dirState->fileCount = 0;
}

internal t_file* win32_directory_find_file(t_directory_state* dirState, t_string16 filename) {
  for(u32 fileIndex = 0; fileIndex < dirState->fileCount; fileIndex += 1) {
    if(string_compare(dirState->files[fileIndex].data.filename, filename)) {
      return(dirState->files + fileIndex);
    }
  }
  return(0);
}

internal void win32_directory_scan(t_directory_state* dirState) {
  assert(dirState->dirPath.ptr);
  assert(dirState->dirSearchPath.ptr);
  
  for(u32 fileIndex = 0; fileIndex < dirState->fileCount; fileIndex += 1) {
    dirState->files[fileIndex].found = false;
  }
  
  WIN32_FIND_DATAW findData = {0};
  HANDLE searchHandle = FindFirstFileW((LPCWSTR)dirState->dirSearchPath.ptr, &findData);
  if(searchHandle != INVALID_HANDLE_VALUE) {
    do {
      bool isDirectory = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
      if(!isDirectory) {
        t_string16 shortName = char16_count((char16*)findData.cFileName);
        t_string16 fullName = win32_get_file_path_mem(shortName);
        t_file* file = win32_directory_find_file(dirState, fullName);
        if(file == 0) {
          file = win32_directory_add(dirState, fullName);
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
  
  for(u32 fileIndex = 0; fileIndex < dirState->fileCount;) {
    t_file* file = dirState->files + fileIndex;
    if(!file->found) {
      win32_directory_remove(dirState, fileIndex);
    }
    else {
      //win32_cache_request_add(dirState, fileIndex);
      fileIndex += 1;
    }
  }
}

// NOTE(bumbread): The following are the platform layer API function
// realisations

internal void platform_directory_set(t_directory_state* dirState, t_string16 path) {
  if(!string_compare(path, dirState->dirPath)) {
    win32_directory_clear(dirState);
    
    if(dirState->dirSearchPath.ptr) free(dirState->dirSearchPath.ptr);
    if(dirState->dirPath.ptr) free(dirState->dirPath.ptr);
    
    dirState->dirPath = path;
    dirState->dirSearchPath = win32_make_path_wildcard_mem(dirState->dirPath);
    win32_directory_scan(dirState);
  }
}

// TODO(bumbread): handle the case when the file is not found
internal void platform_directory_next_file(t_directory_state* dirState) {
  if(dirState->fileCount != 0) {
    u32 newFileIndex = (dirState->currentFile+1) % dirState->fileCount;
    if(newFileIndex != dirState->currentFile) {
      win32_cache_update(dirState);
      dirState->currentFile = newFileIndex;
      dirState->changed = true;
    }
  }
}

internal void platform_directory_previous_file(t_directory_state* dirState) {
  if(dirState->fileCount != 0) {
    u32 newFileIndex = dirState->currentFile;
    if(newFileIndex == 0) newFileIndex += dirState->fileCount;
    newFileIndex -= 1;
    if(newFileIndex != dirState->currentFile) {
      win32_cache_update(dirState);
      dirState->currentFile = newFileIndex;
      dirState->changed = true;
    }
  }
}

internal t_image* platform_get_current_image(t_directory_state* dirState) {
  t_file* file = (dirState->files + dirState->currentFile);
  if(file->image.pixels != 0) {
    return(&file->image);
  }
  return(0);
}
