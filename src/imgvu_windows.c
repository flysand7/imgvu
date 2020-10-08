
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
#include<shellapi.h>
#pragma warning(pop)

#include<stdlib.h>
#include<stdio.h>

struct {
  HWND handle;
  u32 clientWidth;
  u32 clientHeight;
  u32* pixels;
} typedef t_window;

#define get_bit(num, bit) ( ((num) >> (bit)) & 1)

// NOTE(bumbread): directory update polling thread related functions

struct {
  u32 _stub;
} typedef t_update;

struct {
  // TODO(bumbread): actually write updates
  u32 maxUpdatesCount;
  u32 updatesCount;
  t_update* updates;
  t_string16 filePath;
} typedef t_directory_updates;

// TODO(bumbread): rewrite this so that it handles all updates from the root of the
// disk up to the target directory, and only select the updates that directly touch 
// any directory above the target folder, target folder itself or the subdirectory
// of the target folder
internal DWORD WINAPI directories_listen_proc(t_directory_updates* updates) {
  HANDLE dirHandle = CreateFileW((LPCWSTR)updates->filePath.ptr, GENERIC_READ, 
                                 FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
  assert(dirHandle != INVALID_HANDLE_VALUE);
  byte infos[1024] = {0};
  loop {
    byte* infoPointer = infos;
    DWORD bytesReturned = 0;
    
    // TODO(bumbread): separate check to whether the name
    // of the watched directory had changed.
    wprintf(L"waiting for the next dir change...\n");
    // TODO(bumbread): check if the function below works in other
    // commonly used filesystems.
    bool result = ReadDirectoryChangesW(dirHandle, infos, 1024,
                                        false, FILE_NOTIFY_CHANGE_FILE_NAME
                                        | FILE_NOTIFY_CHANGE_SIZE
                                        | FILE_NOTIFY_CHANGE_LAST_WRITE
                                        | FILE_NOTIFY_CHANGE_LAST_ACCESS
                                        |FILE_NOTIFY_CHANGE_CREATION,
                                        &bytesReturned, 0, 0);
    // TODO(bumbread): make sure this code doesn't crash 
    // because the buffer was too short
    assert(result);
    struct _FILE_NOTIFY_INFORMATION* fileInfo = (struct _FILE_NOTIFY_INFORMATION*)infoPointer;
    
    t_string16 filename;
    u32 stringLength = fileInfo->FileNameLength/sizeof(char16);
    filename.len = stringLength;
    filename.ptr = (char16*)fileInfo->FileName;
    filename.ptr[filename.len] = 0;
    
    switch(fileInfo->Action) {
      case(FILE_ACTION_ADDED): {
        wprintf(L"file %ls got added\n", filename.ptr);
#if 0
        // NOTE(bumbread): code in this block shouldn't be in here for now
        {
          t_string16 fullName = win32_get_full_filepath_mem(filename);
          t_file_entry fileEntry;
          fileEntry.filename = fullName;
          fileEntry.extension = win32_get_file_extension(fullName);
          win32_add_file_entry(&directoryState, fileEntry);
        }
#endif
      } break;
      case(FILE_ACTION_REMOVED): {
        wprintf(L"file %ls got removed\n", filename.ptr);
      } break;
      case(FILE_ACTION_MODIFIED): {
        wprintf(L"file %ls got modified\n", filename.ptr);
        
      } break;
      case(FILE_ACTION_RENAMED_OLD_NAME): {
        
        // TODO(bumbread): check if this behaviour is documented
        // but winapi seems to be reliably sendind NEW_NAME after OLD_NAME
        // TODO(bumbread): in case winapi decides to insert another notify information in between these two
        // i should _actually_ write this in a loop leaving the information about
        // the old name and writing to a flag, and clearing that flag using the name,
        // every time NEW_NAME comes in.
        assert(fileInfo->NextEntryOffset != 0);
        infoPointer += fileInfo->NextEntryOffset;
        
        fileInfo = (struct _FILE_NOTIFY_INFORMATION*)infoPointer;
        assert(fileInfo->Action == FILE_ACTION_RENAMED_NEW_NAME);
        
        t_string16 newFilename;
        u32 newStringLength = fileInfo->FileNameLength/sizeof(char16);
        newFilename.len = newStringLength;
        newFilename.ptr = (char16*)fileInfo->FileName;
        newFilename.ptr[newFilename.len] = 0;
        
        wprintf(L"renamed from %ls to %ls\n", filename.ptr, newFilename.ptr);
      } break;
    }
    // TODO(bumbread): wait for single object
  }
}

//
//
//

// NOTE(bumbread): functions used in main thread

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

internal t_string16 win32_make_path_wildcard_mem(t_string16 fullPath) {
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

internal t_string16 win32_get_full_filepath_mem(t_string16 name) {
  t_string16 result;
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
  result.len = receiveLength;
  result.ptr = fullName;
  return(result);
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

//internal t_string16 win32_get_short_filename(t_string16 relativeName);

internal t_string16 win32_argstring_get_full_path(LPWSTR commandLine) {
  t_string16 result = {0};
  int argCount = 0;
  LPWSTR* args = CommandLineToArgvW(commandLine, &argCount);
  if(args) {
    if(argCount >= 1) {
      LPWSTR filePath = args[1];
      t_string16 filepathString = char16_to_string16((char16*)filePath);
      t_string16 fullPath = win32_get_full_filepath_mem(filepathString);
      return(fullPath);
    }
  }
  return(result);
}

internal t_string16 win32_get_path_mem(t_string16 fullPath) {
  t_string16 result;
  result.ptr = (char16*)malloc(fullPath.len);
  for(u32 charIndex = 0; charIndex < fullPath.len; charIndex += 1) {
    result.ptr[charIndex] = fullPath.ptr[charIndex];
  }
  // TODO(bumbread): feels janky, check one more time
  for(u32 charIndex = fullPath.len-1;;charIndex -= 1) {
    if(result.ptr[charIndex] == L'\\') {
      result.ptr[charIndex] = 0;
      result.len = charIndex;
      break;
    }
    if(charIndex == 0) break;
  }
  return(result);
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

#define use_windows_main 0

#if(use_windows_main != 0)
int wWinMain(HINSTANCE instance, HINSTANCE m_unused0, LPWSTR commandLine, int cmdShow) 
#else
int main(void)
#endif
{
#if(use_windows_main == 0)
  LPWSTR commandLine = GetCommandLineW();
  HINSTANCE instance = GetModuleHandle(0);
#endif
  
  // TODO(bumbread): See what happens if i input \\?\... in command line
  t_directory_state directoryState = {0};
  t_string16 baseFilename = win32_argstring_get_full_path(commandLine);
  // TODO(bumbread): test this more on invalid inputs
  // so far this seems to be more-less robust function
  // TODO(bumbread): this program crashes on baseFilename.ptr == 0
  // when commandLine doesn't have the second argument
  assert(baseFilename.ptr);
  
  // TODO(bumbread): check that search mask never has a trailing backslash
  // this function fails on trailing backslashes
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
      t_string16 fullPath = win32_get_full_filepath_mem(filename);
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
  
  t_string16 filePath = win32_get_path_mem(baseFilename);
  
  t_directory_updates updates;
  updates.updatesCount = 0;
  updates.maxUpdatesCount = 8;
  updates.updates = malloc(8 * sizeof(t_update));
  updates.filePath = filePath;
  CreateThread(0, 0, (LPTHREAD_START_ROUTINE)directories_listen_proc, &updates, 0, 0);
  
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
    
    // TODO(bumbread): process updates
    // TODO(bumbread): set object handle
    
    for(u32 keyIndex = 0; keyIndex < KEYBOARD_SIZE; keyIndex += 1) {
      global_keyboard[keyIndex].pressed = false;
      global_keyboard[keyIndex].released = false;
    }
  }
  
  return(0);
}
