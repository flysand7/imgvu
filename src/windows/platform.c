
struct t_directory_state_s;

internal void platform_directory_set(t_directory_state* dirState, t_string16 path) {
  
  if(!string16_compare(path, dirState->dirPath)) {
    win32_directory_free_files(dirState);
    
    if(dirState->dirSearchPath.ptr) free(dirState->dirSearchPath.ptr);
    if(dirState->dirPath.ptr) free(dirState->dirPath.ptr);
    
    dirState->dirPath = path;
    
    dirState->dirSearchPath = win32_make_path_wildcard_mem(dirState->dirPath);
    win32_directory_scan(dirState);
  }
}

internal void platform_directory_next_file(t_directory_state* dirState) {
  profile_block_start(next_file);
  if(dirState->currentFile != 0) {
    win32_set_current_file(dirState, dirState->currentFile->next);
    win32_cache_update(dirState);
  }
  profile_block_end(next_file);
}

internal void platform_directory_previous_file(t_directory_state* dirState) {
  if(dirState->currentFile != 0) {
    win32_set_current_file(dirState, dirState->currentFile->prev);
    win32_cache_update(dirState);
  }
}

internal t_image* platform_get_current_image(t_directory_state* dirState) {
  if(dirState->currentFile != 0) {
    t_file* file = dirState->currentFile;
    if(file->image.pixels != 0) {
      return(&file->image);
    }
  }
  return(0);
}

internal void platform_chose_graphics_provider(t_string provider) {
  if(string_compare(provider, char_count("gl"))) {g_graphics_provider = GRAPHICS_GL;}
  else if(string_compare(provider, char_count("gdi"))) {g_graphics_provider = GRAPHICS_GDI; }
  else {g_graphics_provider = GRAPHICS_GL;}
}

internal void platform_initialize_graphics_provider(void) {
  profile_block_start(gl_init);
  if(g_graphics_provider == GRAPHICS_GL) {
    HGLRC glContext;
    
    PIXELFORMATDESCRIPTOR pixelFormat = {0};
    pixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pixelFormat.nVersion = 1;
    pixelFormat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixelFormat.iPixelType = PFD_TYPE_RGBA;
    pixelFormat.cColorBits = 32;
    pixelFormat.cDepthBits = 24;
    pixelFormat.cStencilBits = 8;
    pixelFormat.cAuxBuffers = 0;
    pixelFormat.iLayerType = PFD_MAIN_PLANE;
    
    int pixelFormatIndex = ChoosePixelFormat(g_window.deviceContext, &pixelFormat);
    assert(pixelFormatIndex != 0); // TODO(bumbread): correct handling of this case, where pixel format wasn't found
    SetPixelFormat(g_window.deviceContext, pixelFormatIndex, &pixelFormat);
    
    glContext = wglCreateContext(g_window.deviceContext);
    assert(glContext != 0); // TODO(bumbread): correct handling
    
    wglMakeCurrent(g_window.deviceContext, glContext);
    
  }
  profile_block_end(gl_init);
  printf("\n");
}

internal void platform_clear_screen(u32 color) {
  switch(g_graphics_provider) {
    case(GRAPHICS_GDI): gdi_clear_screen(color); break;
    case(GRAPHICS_GL): gl_clear_screen(color); break;
  }
}

internal void platform_draw_image(t_location* loc, t_image* image) {
  switch(g_graphics_provider) {
    case(GRAPHICS_GDI): gdi_draw_image(loc, image); break;
    case(GRAPHICS_GL): gl_draw_image(loc, image); break;
  }
}

internal void platform_show(void) {
  switch(g_graphics_provider) {
    case(GRAPHICS_GDI): gdi_show(); break;
    case(GRAPHICS_GL): gl_show(); break;
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
      byte *bytes = malloc((u32)fileSize.LowPart);
      DWORD bytesRead = 0;
      
      result = ReadFile(fileHandle, bytes, fileSize.LowPart, &bytesRead, 0);
      assert(result);
      CloseHandle(fileHandle);
      assert((DWORD)fileSize.LowPart == bytesRead);
      
      fileData.size = (u32)fileSize.LowPart;
      fileData.ptr = bytes;
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
