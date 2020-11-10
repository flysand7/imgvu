
#pragma warning(push, 1)
#include<stdlib.h>
#pragma warning(pop)

#include"imgvu/imgvu.h"

// NOTE(bumbread): Windows.h produces shit ton of 
// warnings starting /W4. I'm disabling them
#pragma warning(push, 1)
// NOTE(bumbread): MSVC's static assert macro
// messes with clang warnings. Apparently 
// the static asserts can be disabled by defining
// SORTPP_PASS
#define SORTPP_PASS
#include<Windows.h>
#include<gl/GL.h>
#include<Shlwapi.h> // PathFileExistsW
#include<UserEnv.h> // GetAllUsersProfileDirectoryW
#pragma warning(pop)

struct {
  HWND handle;
  HDC deviceContext;
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
    window->pixels = malloc(window->clientWidth * window->clientHeight * sizeof(u32));
  }
}

#include"windows/filesystem.c"
#include"windows/directory.c"

enum {GRAPHICS_GL, GRAPHICS_GDI} typedef t_graphics_provider;

global bool g_running;
global t_app_input g_app_input;
global t_window g_window;
global t_app_state g_app_state;
global t_graphics_provider g_graphics_provider = GRAPHICS_GL;

#include"windows/graphics.c"
#include"windows/platform.c"

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
      if(wasPressed != isPressed) {
        if(keyCode == VK_LEFT) g_app_input.prevImage = isPressed;
        if(keyCode == VK_RIGHT) g_app_input.nextImage = isPressed;
        
        if(keyCode == VK_ESCAPE) g_running = false;
        if(keyCode == VK_F4) {
          bool altPressed = GetKeyState(VK_MENU);
          if(altPressed) g_running = false;
        }
      }
      return(0);
    }
    
    case(WM_ACTIVATEAPP): {
      return(0);
    }
    
    case(WM_SIZE): {
      u32 newClientWidth = LOWORD(lp);
      u32 newClientHeight = HIWORD(lp);
      resize_window(&g_window, newClientWidth, newClientHeight);
      app_draw(&g_app_state);
      return(0);
    }
    
    case(WM_CLOSE): {
      g_running = false;
      return(0);
    }
    
    default: {
      return(DefWindowProcW(window, msg, wp, lp));
    }
  }
  return(0);
}

internal t_string16 win32_get_full_path_from_args(void) {
  int argCount = 0;
  LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &argCount);
  assert(args != 0);
  
  t_string16 filePath = {0};
  if(argCount > 1) {
    filePath = char16_copy_mem(args[1]);
    assert(filePath.ptr != 0);
    assert(filePath.len != 0);
    
    t_string16 fullPath = win32_get_file_path_mem(filePath);
    win32_remove_trailing_backslash(&fullPath);
    free(filePath.ptr);
    return(fullPath);
  }
  else {
    t_string16 result = win32_get_current_directory_mem();
    return(result);
  }
}

// TODO(bumbread): also add alt+enter for fullscreen,
// TODO(bumbread): and also add fullscreen support, yeah!
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
  directoryState.changed = true;
  {
    t_string16 watchDir = win32_get_path_mem(fileToOpen);
    platform_directory_set(&directoryState, watchDir);
  }
  
  {
    HINSTANCE instance = GetModuleHandle(0);
    WNDCLASSEXW windowClass = {0};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = window_proc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = L"imgvu_window_class";
    
    RegisterClassExW(&windowClass);
    g_window.handle= CreateWindowExW(0, windowClass.lpszClassName, L"imgvu", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                     0, 0, instance, 0);
    assert(g_window.handle);
    ShowWindow(g_window.handle, SW_SHOWDEFAULT);
  }
  
  g_window.deviceContext = GetDC(g_window.handle);
  g_running = true;
  
  r32 dt = 0;
  loop {
    {
      MSG message;
      while(PeekMessageW(&message, g_window.handle, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
      }
      if(!g_running) break;
    }
    
    bool stop = app_update(&g_app_state, &directoryState, &g_app_input, dt);
    if(stop) break;
    
    app_draw(&g_app_state);
    
    if(directoryState.changed) {
      directoryState.changed = false;
      bool fileExists = false;
      
      if(directoryState.fileCount != 0) {
        t_string16 currentFilename = directoryState.files[directoryState.currentFile].data.filename;
        if(currentFilename.ptr != 0) {
          SetWindowTextW(g_window.handle, (LPCWSTR)currentFilename.ptr);
          fileExists = true;
        }
      }
      if(!fileExists) {
        static_make_string16(prefix, L"[no file found]");
        SetWindowTextW(g_window.handle, (LPCWSTR)prefix.ptr);
      }
    }
    
    g_app_input = (t_app_input) {0};
  }
  
  return(0);
}
