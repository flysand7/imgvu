
#include"imgvu/imgvu.h"
#include"imgvu/main.c"
#include"imgvu/platform.h"

#define debug_variable_unused(lvalue_) do{{void* tmp=&(lvalue_);tmp = 0;}}while(0)

// NOTE(bumbread): Windows.h produces shit ton of 
// warnings starting /W4. I'm disabling them
#pragma warning(push, 1)
// NOTE(bumbread): MSVC's static assert macro
// messes with clang warnings. Apparently 
// the static asserts can be disabled by defining
// SORTPP_PASS
#define SORTPP_PASS
#include<Windows.h>
//#include<shellapi.h>
#include<Shlwapi.h> // PathFileExistsW
#pragma warning(pop)

#include"windows/filesystem.c"

#include<stdlib.h>
// TODO(bumbread): stop using stdio for printf stuff
#include<stdio.h>

struct {
  HWND handle;
  u32 clientWidth;
  u32 clientHeight;
  u32* pixels;
} typedef t_window;

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

// TODO(bumbread): presume a certain sorting pattern 
// and add inside a sorted array.
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

internal void win32_remove_file_entry(t_directory_state* directoryState, u32 index) {
  t_file_entry* entryToDelete = directoryState->files + index;
  free(entryToDelete->filename.ptr);
  
  for(u32 currentIndex = index; currentIndex < directoryState->fileCount - 1; currentIndex += 1) {
    directoryState->files[currentIndex] = directoryState->files[currentIndex+1];
  }
}

internal void request_next_image(t_directory_state* directoryState) {
  t_file_entry* lastEntry = directoryState->files + directoryState->fileIndex;
  assert(lastEntry->data != 0);
  free(lastEntry->data);
  lastEntry->data = 0;
  lastEntry->size = 0;
  
  directoryState->fileIndex += 1;
  if(directoryState->fileIndex == directoryState->fileCount) {
    directoryState->fileIndex = 0;
  }
  
  HANDLE lastHandle = 0;
  do {
    t_file_entry* newEntry = directoryState->files + directoryState->fileIndex;
    
    HANDLE fileHandle = CreateFileW((LPWSTR)newEntry->filename.ptr, GENERIC_READ, FILE_SHARE_READ,
                                    0, OPEN_EXISTING, 0, 0);
    if(fileHandle == INVALID_HANDLE_VALUE) {
      win32_remove_file_entry(directoryState, directoryState->fileIndex);
      if(directoryState->fileIndex == directoryState->fileCount) {
        directoryState->fileIndex = 0;
      }
    }
    else lastHandle = fileHandle;
  } while((lastHandle == 0) && (directoryState->fileCount != 0));
  
  if(lastHandle != 0) {
    t_file_entry* newEntry = directoryState->files + directoryState->fileIndex;
    
    LARGE_INTEGER fileSize;
    GetFileSizeEx(lastHandle, &fileSize);
    newEntry->size = (u32)fileSize.LowPart;
    newEntry->data = malloc(newEntry->size);
    
    DWORD bytesRead = 0;
    bool result = ReadFile(lastHandle, newEntry->data, newEntry->size, &bytesRead, 0);
    assert((u32)bytesRead == newEntry->size);
    assert(result);
    
    CloseHandle(lastHandle);
  }
}

internal void request_prev_image(t_directory_state* directoryState) {
  t_file_entry* lastEntry = directoryState->files + directoryState->fileIndex;
  assert(lastEntry->data != 0);
  free(lastEntry->data);
  lastEntry->data = 0;
  lastEntry->size = 0;
  
  if(directoryState->fileIndex == 0) {
    directoryState->fileIndex = directoryState->fileCount;
  }
  directoryState->fileIndex -= 1;
  
  HANDLE lastHandle = 0;
  do {
    t_file_entry* newEntry = directoryState->files + directoryState->fileIndex;
    
    HANDLE fileHandle = CreateFileW((LPWSTR)newEntry->filename.ptr, GENERIC_READ, FILE_SHARE_READ,
                                    0, OPEN_EXISTING, 0, 0);
    if(fileHandle == INVALID_HANDLE_VALUE) {
      win32_remove_file_entry(directoryState, directoryState->fileIndex);
      if(directoryState->fileIndex == 0) {
        directoryState->fileIndex = directoryState->fileCount;
      }
      directoryState->fileIndex -= 1;
    }
    else lastHandle = fileHandle;
    
  } while((lastHandle == 0) && (directoryState->fileCount != 0));
  
  if(lastHandle != 0) {
    t_file_entry* newEntry = directoryState->files + directoryState->fileIndex;
    
    LARGE_INTEGER fileSize;
    GetFileSizeEx(lastHandle, &fileSize);
    newEntry->size = (u32)fileSize.LowPart;
    newEntry->data = malloc(newEntry->size);
    
    DWORD bytesRead = 0;
    bool result = ReadFile(lastHandle, newEntry->data, newEntry->size, &bytesRead, 0);
    assert((u32)bytesRead == newEntry->size);
    assert(result);
    
    CloseHandle(lastHandle);
  }
}

internal t_string16 win32_argstring_get_full_path(LPWSTR commandLine) {
  t_string16 result = {0};
  int argCount = 0;
  LPWSTR* args = CommandLineToArgvW(commandLine, &argCount);
  if(args) {
    LPWSTR filePath = 0;
    if(argCount == 1) filePath = args[0];
    else if(argCount > 1) filePath = args[1];
    
    assert(filePath != 0);
    t_string16 filepathString = char16_to_string16((char16*)filePath);
    t_string16 fullPath = win32_get_full_path_to_file_mem(filepathString);
    return(fullPath);
  }
  return(result);
}

int main(void)
{
  LPWSTR commandLine = GetCommandLineW();
  HINSTANCE instance = GetModuleHandle(0);
  
  t_directory_state directoryState = {0};
  t_string16 baseFilename = win32_argstring_get_full_path(commandLine);
  // TODO(bumbread): test this more on invalid inputs
  
  win32_remove_trailing_backslash(&baseFilename);
  bool isPathValid = PathFileExistsW(baseFilename.ptr);
  if(!isPathValid) {
    // TODO(bumbread): remove if not needed
    debug_variable_unused(isPathValid);
  }
  t_string16 searchPath = win32_make_path_wildcard_mem(baseFilename);
  
  WIN32_FIND_DATAW findData;
  HANDLE searchHandle = FindFirstFileExW((LPWSTR)searchPath.ptr,
                                         FindExInfoBasic,
                                         &findData,
                                         FindExSearchNameMatch, 
                                         0, FIND_FIRST_EX_LARGE_FETCH|FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY);
  if(searchHandle == INVALID_HANDLE_VALUE) {
    FindClose(searchHandle);
  }
  else {
    loop {
      t_string16 filename = char16_to_string16((char16*)findData.cFileName);
      t_string16 fullPath = win32_get_full_path_to_file_mem(filename);
      bool isDirectory = findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
      if(!isDirectory) {
        t_file_entry fileEntry;
        fileEntry.filename = fullPath;
        fileEntry.extension = win32_get_file_extension(fullPath);
        win32_add_file_entry(&directoryState, fileEntry);
      }
      
      bool nextFileFound = FindNextFileW(searchHandle, &findData);
      if(!nextFileFound) break;
    }
  }
  free(searchPath.ptr);
  
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
