
struct t_directory_state_s;
internal void platform_draw_image(t_location* loc, t_image* image);


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
