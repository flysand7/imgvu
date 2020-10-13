
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
#include"windows/directory.c"

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
  directoryState.cacheOffset = 4;
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
