
#include"imgvu/imgvu.h"
#include"imgvu/main.c"
#include"imgvu/platform.h"

// NOTE(bumbread): Windows.h produces shit ton of 
// warnings starting /W4. I'm disabling them
#pragma warning(push, 1)
// NOTE(bumbread): MSVC's static assert macro
// messes with clang warnings. Apparently 
// the static asserts can be disabled by defining
// SORTPP_PASS
#define SORTPP_PASS
#include<Windows.h>
#include<shellapi.h>
#pragma warning(pop)

#include<stdlib.h>

struct {
  HWND handle;
  u32 clientWidth;
  u32 clientHeight;
  u32* pixels;
} typedef t_window;

#define get_bit(num, bit) ( ((num) >> (bit)) & 1)

internal void
resize_window(t_window* window, u32 newClientWidth, u32 newClientHeight) {
  if((newClientWidth != window->clientWidth) 
     || newClientHeight != window->clientHeight) {
    window->clientWidth = newClientWidth;
    window->clientHeight = newClientHeight;
    if(window->pixels) {
      free(window->pixels);
    }
    window->pixels = (u32*)malloc(window->clientWidth * window->clientHeight * sizeof(u32));
  }
}

internal void
paint_window_gdi(t_window* window, HDC deviceContext) {
  assert(window->pixels);
  
  BITMAPINFO bitmapInfo = {
    .bmiHeader = {
      .biSize = sizeof(BITMAPINFO),
      .biWidth = (LONG)window->clientWidth,
      .biHeight = (LONG)window->clientHeight,
      .biPlanes = 1,
      .biBitCount = 32,
      .biCompression = BI_RGB
    }
  };
  
  StretchDIBits(deviceContext, 
                0, 0, (LONG)window->clientWidth, (LONG)window->clientHeight,
                0, 0, (LONG)window->clientWidth, (LONG)window->clientHeight,
                window->pixels, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

//
//
//

global bool global_running;
global t_button global_keyboard[0x100];
global t_window global_window;
global t_program_state global_programState;

internal void
win32_draw_app(t_program_state* programState, t_window* window, HDC deviceContext) {
  for(u32 pixelIndex = 0; 
      pixelIndex < window->clientWidth*window->clientHeight; 
      pixelIndex += 1) {
    window->pixels[pixelIndex] = 0;
  }
  draw_app(&global_programState);
  paint_window_gdi(window, deviceContext);
}

internal LRESULT CALLBACK 
window_proc(HWND window, UINT msg, WPARAM wp, LPARAM lp) {
  switch(msg) {
    case(WM_NULL): {} break;
    
    case(WM_SYSKEYUP):
    case(WM_SYSKEYDOWN):
    case(WM_KEYUP):
    case(WM_KEYDOWN): {
      bool wasPressed = get_bit(lp, 30);
      bool isPressed = !get_bit(lp, 31);
      u64 keyCode = (u64)wp;
      assert(keyCode < KEYBOARD_SIZE);
      if(wasPressed != isPressed) {
        global_keyboard[keyCode].down = isPressed;
        global_keyboard[keyCode].pressed = isPressed;
        global_keyboard[keyCode].released = wasPressed;
      }
      return(0);
    }
    
    case(WM_ACTIVATEAPP): {
      return(0);
    }
    
    case(WM_SIZE): {
      u32 newClientWidth = LOWORD(lp);
      u32 newClientHeight = HIWORD(lp);
      resize_window(&global_window, newClientWidth, newClientHeight);
      return(0);
    }
    
    case(WM_PAINT): {
      PAINTSTRUCT paintStruct;
      HDC paintDC = BeginPaint(global_window.handle, &paintStruct);
      win32_draw_app(&global_programState, &global_window, paintDC);
      EndPaint(global_window.handle, &paintStruct);
      return(0);
    }
    
    case(WM_CLOSE): {
      global_running = false;
      return(0);
    }
    
    default: {
      return(DefWindowProcW(window, msg, wp, lp));
    }
  }
  return(0);
}

internal t_string16  win32_widestring_to_string16(LPWSTR wide) {
  char16* stringPointer = (char16*)wide;
  u32 length = 0;
  loop {
    if(*stringPointer == 0x0000) break;
    stringPointer += 1;
    length = 0;
  }
  t_string16 result;
  result.len = length;
  result.ptr = stringPointer;
  return(result);
}

internal t_string16 win32_char16_to_string16(char16* chars) {
  u32 length = 0;
  char16* charPointer = chars;
  loop {
    if(*charPointer == 0) {
      break;
    }
    length += 1;
    charPointer += 1;
  }
  t_string16 result;
  result.len = length;
  result.ptr = chars;
  return(result);
}

internal t_string16 win32_make_path_wildcard(t_string16 fullPath) {
  t_string16 result;
  result.len = fullPath.len;
  result.ptr = (char16*)malloc((result.len+1) * sizeof(char16));
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
  result.ptr[lastBackSlashPosition+1] = L'.';
  result.ptr[lastBackSlashPosition+2] = L'*';
  result.ptr[lastBackSlashPosition+3] = 0;
  result.len = (u32)(lastBackSlashPosition + 3);
  return(result);
}

internal t_string16 win32_full_filepath_from_args(LPWSTR commandLine) {
  t_string16 result = {0};
  int argCount = 0;
  LPWSTR* args = CommandLineToArgvW(commandLine, &argCount);
  if(args) {
    if(argCount >= 1) {
      LPWSTR filePath = args[0];
      // NOTE(bumbread): running the command with empty buffer to get the receive length
      u32 receiveLength = (u32)GetFullPathNameW(filePath, 0, 0, 0);
      if(receiveLength == 0) {
        return(result);
      }
      // NOTE(bumbread): reveiving the actual full path
      char16* fullName = (char16*)malloc(receiveLength * sizeof(char16));
      GetFullPathNameW(filePath, receiveLength, fullName, 0);
      fullName[receiveLength] = 0;
      // NOTE(bumbread): returning
      result.len = receiveLength - 1;
      result.ptr = fullName;
    }
  }
  return(result);
}

// NOTE(bumbread): Platform layer function realisations
internal t_string16 get_file_extension(t_string16 name) {
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

internal t_string16 get_short_filename(t_string16 relativeName);

internal void win32_add_file_entry(t_directory_state* directory, t_file_entry fileEntry) {
  if(directory->fileCount + 1 > directory->maxFiles) {
    directory->maxFiles *= 2;
    if(!directory->maxFiles) directory->maxFiles = 1;
    directory->files = (t_file_entry*)realloc(directory->files, directory->maxFiles * sizeof(t_file_entry));
    assert(directory->files);
  }
  directory->files[directory->fileCount] = fileEntry;
  directory->fileCount += 1;
}

int wWinMain(HINSTANCE instance, HINSTANCE m_unused0, LPWSTR commandLine, int cmdShow) {
  
  t_directory_state directoryState = {0};
  t_string16 fullFilepath = win32_full_filepath_from_args(commandLine);
  // TODO(bumbread): test this more on invalid inputs
  // so far this seems to be more-less robust function
  assert(fullFilepath.ptr);
  
  // TODO(bumbread): check that search mask never has a trailing backslash
  // this function fails on trailing backslashes
  t_string16 searchMask = win32_make_path_wildcard(fullFilepath);
  
  WIN32_FIND_DATAW findData;
  HANDLE searchHandle = FindFirstFileExW((LPWSTR)searchMask.ptr,
                                         FindExInfoBasic,
                                         &findData,
                                         FindExSearchNameMatch, 
                                         0, FIND_FIRST_EX_LARGE_FETCH|FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY);
  if(searchHandle == INVALID_HANDLE_VALUE) {
    FindClose(searchHandle);
  }
  else {
    loop {
      
      char16* filename = (char16*)findData.cFileName;
      bool isDirectory = findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
      if(!isDirectory) {
        t_file_entry fileEntry;
        fileEntry.filename = win32_char16_to_string16(filename);
        fileEntry.extension = get_file_extension(fileEntry.filename);
        // TODO(bumbread): presume a certain sorting pattern 
        // and add inside a sorted array.
        win32_add_file_entry(&directoryState, fileEntry);
      }
      
      bool nextFileFound = FindNextFileW(searchHandle, &findData);
      if(!nextFileFound) break;
    }
  }
  
  // TODO(bumbread): look if the file directory gets changed on a separate thread.
  
  WNDCLASSEXW windowClass = {
    .cbSize = sizeof(WNDCLASSEXW),
    .style = CS_HREDRAW | CS_VREDRAW,
    .lpfnWndProc = window_proc,
    .hInstance = instance,
    .lpszClassName = L"imgvu_window_class"
  };
  RegisterClassExW(&windowClass);
  global_window.handle= CreateWindowExW(0, windowClass.lpszClassName, L"imgvu", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                        0, 0, instance, 0);
  assert(global_window.handle);
  ShowWindow(global_window.handle, SW_SHOWDEFAULT);
  HDC deviceContext = GetDC(global_window.handle);
  
  global_running = true;
  loop {
    MSG message;
    while(PeekMessageW(&message, global_window.handle, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessageW(&message);
    }
    if(!global_running) break;
    
    r32 dt = 0;
    bool exit = update_app(global_keyboard, &global_programState, dt);
    if(exit) break;
    
    win32_draw_app(&global_programState, &global_window, deviceContext);
    
    for(u32 keyIndex = 0; keyIndex < KEYBOARD_SIZE; keyIndex += 1) {
      global_keyboard[keyIndex].pressed = false;
      global_keyboard[keyIndex].released = false;
    }
  }
  
  return(0);
}
