
internal t_string16 win32_make_path_wildcard_mem(t_string16 fullPath) {
  t_string16 result;
  result.len = fullPath.len;
  result.ptr = (char16*)malloc((result.len+2) * sizeof(char16));
  i32 lastBackSlashPosition = -1;
  for(u32 charIndex = 0; charIndex < result.len; charIndex += 1) {
    char16 nextChar = fullPath.ptr[charIndex];
    result.ptr[charIndex] = nextChar;
    if(nextChar == L'\\') {
      lastBackSlashPosition = (i32)charIndex;
    }
  }
  lastBackSlashPosition += 1;
  result.ptr[lastBackSlashPosition+0] = L'*';
  result.ptr[lastBackSlashPosition+1] = 0;
  result.len = (u32)(lastBackSlashPosition + 1);
  return(result);
}

internal t_string16 win32_get_file_path_mem(t_string16 name) {
  persist t_string16 prepend = {0};
  if(!prepend.len) prepend = char16_copy(L"\\\\?\\");
  
  t_string16 result;
  // NOTE(bumbread): making sure there's no \\?\.
  if(string_begins_with(name, prepend)) {
    name.len -= 4;
    name.ptr += 4;
  }
  
  // NOTE(bumbread): running the command with empty buffer to get the receive length
  u32 receiveLength = (u32)GetFullPathNameW((LPCWSTR)name.ptr, 0, 0, 0);
  if(receiveLength == 0) {
    result.len = 0;
    result.ptr = 0;
    return(result);
  }
  // NOTE(bumbread): reveiving the actual full path
  char16* fullName = (char16*)malloc(receiveLength * sizeof(char16));
  GetFullPathNameW((LPCWSTR)name.ptr, receiveLength, fullName, 0);
  fullName[receiveLength] = 0;
  result.len = receiveLength-1;
  result.ptr = fullName;
  return(result);
}

internal void win32_remove_trailing_backslash(t_string16* path) {
  if(path->ptr[path->len-1] == L'\\') {
    path->len -= 1;
    path->ptr[path->len] = 0;
  }
}

internal t_string16 win32_get_file_extension(t_string16 name) {
  u32 charIndex = name.len;
  t_string16 result = {0};
  loop {
    if(name.ptr[charIndex] == L'.') {
      u32 symbolsBeforeExtension = (charIndex+1);
      result.ptr = name.ptr + symbolsBeforeExtension;
      result.len = name.len - symbolsBeforeExtension;
      return(result);
    }
    else if(name.ptr[charIndex] == L'\\') {
      result.ptr = name.ptr + name.len;
      result.len = 0;
    }
    if(charIndex == 0) break;
    charIndex -= 1;
  }
  return(result);
}

internal t_string16 win32_get_path_to_file_mem(t_string16 fullPath) {
  win32_remove_trailing_backslash(&fullPath);
  
  t_string16 result;
  result.ptr = (char16*)malloc(fullPath.len);
  for(u32 i = 0; i < fullPath.len; i += 1) result.ptr[i] = fullPath.ptr[i];
  u32 charIndex = fullPath.len - 1;
  
  loop {
    if(result.ptr[charIndex] == L'\\') {
      result.ptr[charIndex] = 0;
      result.len = charIndex;
      break;
    }
    if(charIndex == 0) break;
    charIndex -= 1;
  }
  
  return(result);
}

internal t_string16 win32_get_dir_level_mem(t_string16 filename, u32 level) {
  t_string16 result;
  result.len = 0;
  u32 currentLevel = 0;
  for(u32 charIndex = 0; charIndex < filename.len; charIndex += 1) {
    if(filename.ptr[charIndex] == '\\') { 
      if(currentLevel == level) {
        break;
      }
      currentLevel += 1;
    }
    result.len += 1;
  }
  result.ptr = (char16*)malloc((result.len + 1) * sizeof(char16));
  for(u32 charIndex = 0; charIndex < result.len; charIndex += 1) {
    result.ptr[charIndex] = filename.ptr[charIndex];
  }
  result.ptr[result.len] = 0;
  return(result);
}

internal bool directory_contains(t_string16 root, t_string16 dir) {
  // TODO(bumbread): assert full paths
  return(string_begins_with(dir, root));
}
