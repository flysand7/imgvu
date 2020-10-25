
struct {
  t_file_data data;
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

internal bool win32_check_usage_frequency(u32 cacheLoads, u32 maxCacheLoads, u32 avgCacheLoads) {
  bool isUsageSpread = 2*avgCacheLoads > maxCacheLoads;
  bool isFrequentEnough = 4*cacheLoads > maxCacheLoads;
  return(isUsageSpread && isFrequentEnough);
}

internal void win32_cache_remove(t_directory_state* dirState, u32 fileIndex) {
  t_file* currentFile = dirState->files + fileIndex;
  assert(currentFile->cached);
  currentFile->cached = false;
  currentFile->cacheLoads = 0;
  win32_free_file(currentFile);
}

internal void win32_directory_remove(t_directory_state* dirState, u32 index) {
  assert(dirState->fileCount != 0);
  
  t_file* file = dirState->files + index;
  if(file->cached) win32_cache_remove(dirState, index);
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
  assert(file->cached == false);
  assert(file->data.ptr == 0);
  assert(file->image.pixels == 0);
  
  // NOTE(bumbread): loading into the cache
  file->data = platform_load_file(file->data.filename);
  if(file->data.size != 0) {
    file->image = app_decode_file(file->data);
    if(file->image.success == true) {
      file->cached = true;
      file->cacheLoads += 1;
    }
  }
  
  if(file->cached == false) {
    win32_directory_remove(dirState, fileIndex);
    return(true);
  }
  
  return(false);
}

internal void win32_cache_update(t_directory_state* dirState) {
  // NOTE(bumbread): updating the cache by load frequency
  u32 maxCacheLoads = 0;
  u32 totalCacheLoads = 0;
  for(u32 fileIndex = 0; fileIndex < dirState->fileCount; fileIndex += 1) {
    u32 currentCacheLoads = dirState->files[fileIndex].cacheLoads;
    if(currentCacheLoads > maxCacheLoads) maxCacheLoads = currentCacheLoads;
    totalCacheLoads += currentCacheLoads;
  }
  // TODO(bumbread): should this be counted using 
  // floating point arithmetic
  u32 avgCacheLoads = totalCacheLoads / dirState->fileCount;
  
  u32 fileIndex = 0;
  loop {
    t_file* file = dirState->files + fileIndex;
    
    u32 currentCacheLoads = file->cacheLoads;
    bool shouldBeCached = win32_check_usage_frequency(currentCacheLoads, maxCacheLoads, avgCacheLoads);
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
        t_string16 fullName = win32_get_file_path_from_relative_mem(dirState->dirPath, shortName);
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
