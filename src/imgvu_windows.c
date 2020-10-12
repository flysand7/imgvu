
#pragma warning(push, 1)
#include<stdlib.h>
#pragma warning(pop)

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
#include<Shlwapi.h> // PathFileExistsW
#pragma warning(pop)

#include"windows/filesystem.c"

struct {
  HWND handle;
  u32 clientWidth;
  u32 clientHeight;
  u32* pixels;
} typedef t_window;

struct {
  t_string16 name;
  t_data data;
  t_image image;
  bool found;
} typedef t_file;

struct {
  
  t_string16 dirPath;
  t_string16 dirSearchPath;
  
  u32 fileCount;
  u32 filesAllocated;
  t_file* files;
} typedef t_directory_state;

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
  
  BITMAPINFO bitmapInfo = {0};
  bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
  bitmapInfo.bmiHeader.biWidth = (LONG)window->clientWidth;
  bitmapInfo.bmiHeader.biHeight = (LONG)window->clientHeight;
  bitmapInfo.bmiHeader.biPlanes = 1;
  bitmapInfo.bmiHeader.biBitCount = 32;
  bitmapInfo.bmiHeader.biCompression = BI_RGB;
  
  StretchDIBits(deviceContext, 
                0, 0, (LONG)window->clientWidth, (LONG)window->clientHeight,
                0, 0, (LONG)window->clientWidth, (LONG)window->clientHeight,
                window->pixels, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

global bool global_running;
global t_button global_keyboard[0x100];
global t_window global_window;

internal void
win32_draw_app(t_window* window, HDC deviceContext) {
  for(u32 pixelIndex = 0; 
      pixelIndex < window->clientWidth*window->clientHeight; 
      pixelIndex += 1) {
    window->pixels[pixelIndex] = 0;
  }
  
  paint_window_gdi(window, deviceContext);
}

#define get_bit(num, bit) ( ((num) >> (bit)) & 1)
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
      win32_draw_app(&global_window, paintDC);
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

internal t_file* win32_directory_add(t_directory_state* state, t_string16 filename) {
  if(state->fileCount + 1 > state->filesAllocated) {
    state->filesAllocated *= 2;
    if(state->filesAllocated == 0) state->filesAllocated = 1;
    state->files = (t_file*)realloc(state->files, state->filesAllocated * sizeof(t_file));
  }
  // TODO(bumbread): chose index based 
  // on sorting constraints
  u32 newIndex = state->fileCount;
  t_file newFile = {0};
  newFile.name = filename;
  state->files[newIndex] = newFile;
  state->fileCount += 1;
  return(state->files + newIndex);
}

internal void win32_directory_remove(t_directory_state* state, u32 index) {
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

internal t_string16 win32_get_full_path_from_args(void) {
  int argCount = 0;
  LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &argCount);
  assert(args != 0);
  
  t_string16 filePath = {0};
  if(argCount == 1)     filePath = char16_copy_mem(args[0]);
  else if(argCount > 1) filePath = char16_copy_mem(args[1]);
  assert(filePath.ptr != 0);
  assert(filePath.len != 0);
  
  t_string16 fullPath = win32_get_file_path_mem(filePath);
  win32_remove_trailing_backslash(&fullPath);
  return(fullPath);
}

int main(void)
{
  t_string16 fileToOpen = win32_get_full_path_from_args();
  
  // TODO(bumbread): remove if not needed
  bool isPathValid = PathFileExistsW(fileToOpen.ptr);
  if(!isPathValid) {
    debug_variable_unused(isPathValid);
    return(1);
  }
  
  t_directory_state directoryState = {0};
  {
    t_string16 watchDir = win32_get_path_to_file_mem(fileToOpen);
    win32_directory_set(&directoryState, watchDir);
  }
  
  {
    HINSTANCE instance = GetModuleHandle(0);
    WNDCLASSEXW windowClass = {0};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = window_proc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = L"imgvu_window_class";
    
    RegisterClassExW(&windowClass);
    global_window.handle= CreateWindowExW(0, windowClass.lpszClassName, L"imgvu", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                          CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                          0, 0, instance, 0);
    assert(global_window.handle);
    ShowWindow(global_window.handle, SW_SHOWDEFAULT);
  }
  
  HDC deviceContext = GetDC(global_window.handle);
  global_running = true;
  
  r32 dt = 0;
  loop {
    {
      MSG message;
      while(PeekMessageW(&message, global_window.handle, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
      }
      if(!global_running) break;
    }
    
    if(app_update(global_keyboard, dt)) break;
    win32_draw_app(&global_window, deviceContext);
    
    for(u32 keyIndex = 0; keyIndex < KEYBOARD_SIZE; keyIndex += 1) {
      global_keyboard[keyIndex].pressed = false;
      global_keyboard[keyIndex].released = false;
    }
  }
  
  return(0);
}
