
struct t_directory_state_s;
internal void platform_draw_image(t_location* loc, t_image* image);

internal void platform_directory_set(t_directory_state* dirState, t_string16 path) {
  if(!string16_compare(path, dirState->dirPath)) {
    win32_directory_clear(dirState);
    
    if(dirState->dirSearchPath.ptr) free(dirState->dirSearchPath.ptr);
    if(dirState->dirPath.ptr) free(dirState->dirPath.ptr);
    
    dirState->dirPath = path;
    dirState->dirSearchPath = win32_make_path_wildcard_mem(dirState->dirPath);
    win32_directory_scan(dirState);
  }
}

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

// TODO(bumbread): should make abstract color structure that describes color in a
// platform independent way.
internal void platform_clear_screen(u32 color) {
  
  for(u32 row = 0; row < g_window.clientHeight; row += 1) {
    for(u32 column = 0; column < g_window.clientWidth; column += 1) {
      g_window.pixels[row*g_window.clientWidth + column] = color;
    }
  }
  
}

internal void platform_draw_image(t_location* loc, t_image* image) {
  i32 maxWidth = (i32)g_window.clientWidth;
  i32 maxHeight = (i32)g_window.clientHeight;
  
  i32 xPosition = (i32)loc->posX;
  i32 yPosition = (i32)loc->posY;
  i32 width = (i32)image->width;
  i32 height = (i32)image->height;
  
  if(xPosition >= maxWidth) return;
  if(yPosition >= maxHeight) return;
  if(xPosition + maxWidth <= 0) return;
  if(yPosition + maxHeight <= 0) return;
  
  if(xPosition < 0) { 
    width += xPosition;
    xPosition = 0;
  }
  if(yPosition < 0) {
    height += yPosition;
    yPosition = 0;
  }
  if(xPosition + width > maxWidth) {
    i32 over = xPosition + width - maxWidth;
    assert(over > 0);
    width -= over;
  }
  if(yPosition + height > maxHeight) {
    i32 over = yPosition + height - maxHeight;
    assert(over > 0);
    height -= over;
  }
  
  assert(xPosition >= 0);
  assert(yPosition >= 0);
  assert(xPosition + width <= maxWidth);
  assert(yPosition + height <= maxHeight);
  
  u32* targetRow = g_window.pixels + (u32)yPosition*g_window.clientWidth + (u32)xPosition;
  u32* sourceRow = image->pixels;
  for(i32 row = 0; row < height; row += 1) {
    u32* targetPixel = targetRow;
    u32* sourcePixel = sourceRow;
    for(i32 column = 0; column < width; column += 1) {
      *targetPixel = *sourcePixel;
      targetPixel += 1;
      sourcePixel += 1;
    }
    sourceRow += image->width;
    targetRow += g_window.clientWidth;
  }
}

internal t_file_data platform_load_file(t_string16 fullFilename) {
  t_file_data fileData = {0};
  fileData.filename = fullFilename;
  
  HANDLE fileHandle = CreateFileW((LPCWSTR)fileData.filename.ptr, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if(fileHandle != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER fileSize;
    bool result = GetFileSizeEx(fileHandle, &fileSize);
    assert(result);
    if(fileSize.LowPart != 0) {
      fileData.ptr = malloc((u32)fileSize.LowPart);
      DWORD bytesRead = 0;
      
      result = ReadFile(fileHandle, fileData.ptr, fileSize.LowPart, &bytesRead, 0);
      assert(result);
      CloseHandle(fileHandle);
      assert((DWORD)fileSize.LowPart == bytesRead);
      
      fileData.size = (u32)fileSize.LowPart;
    }
  }
  
  return(fileData);
}

internal bool platform_write_file(t_file_data file) {
  HANDLE fileHandle = CreateFileW((LPCWSTR)file.filename.ptr, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if(fileHandle != INVALID_HANDLE_VALUE) {
    DWORD bytesWritten;
    bool result = WriteFile(fileHandle, file.ptr, (DWORD)file.size, &bytesWritten, 0);
    if(result == true) {
      if(bytesWritten == (DWORD)file.size) {
        return(true);
      }
    } 
  }
  
  return(false);
}

internal t_string16 platform_get_config_filename(void) {
  DWORD stringSize = 0;
  if(!GetAllUsersProfileDirectoryW(0, &stringSize)) {
    t_string16 directory;
    directory.len = stringSize-1;
    directory.ptr = malloc(stringSize * sizeof(char16));
    if(GetAllUsersProfileDirectoryW((LPWSTR)directory.ptr, &stringSize)) {
      win32_remove_trailing_backslash(&directory);
      
      static_make_string16(filename, L"\\imgvu_config.txt");
      t_string16 fullName = string16_concatenate_mem(directory, filename);
      
      return(fullName);
    }
    else {
      assert(0);
    }
  }
  else {
    assert(0);
  }
  return((t_string16) {0});
}
