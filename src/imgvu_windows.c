
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
    window->pixels = malloc(window->clientWidth * window->clientHeight * sizeof(u32));
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

// TODO(bumbread): get rid of as many globals as possible
// without making the control flow confusing
global bool g_running;
global t_app_input g_app_input;
global t_window g_window;
global t_app_state g_app_state;

internal void
win32_draw_app(t_window* window, HDC deviceContext) {
  for(u32 pixelIndex = 0; 
      pixelIndex < window->clientWidth*window->clientHeight; 
      pixelIndex += 1) {
    window->pixels[pixelIndex] = 0;
  }
  app_draw(&g_app_state);
  paint_window_gdi(window, deviceContext);
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
      return(0);
    }
    
    case(WM_PAINT): {
      PAINTSTRUCT paintStruct;
      HDC paintDC = BeginPaint(g_window.handle, &paintStruct);
      win32_draw_app(&g_window, paintDC);
      EndPaint(g_window.handle, &paintStruct);
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
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
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
  
  HDC deviceContext = GetDC(g_window.handle);
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
    
    win32_draw_app(&g_window, deviceContext);
    
    if(directoryState.changed) {
      directoryState.changed = false;
      t_string16 currentFilename = directoryState.files[directoryState.currentFile].data.filename;
      if(currentFilename.ptr != 0) {
        SetWindowTextW(g_window.handle, (LPCWSTR)currentFilename.ptr);
      }
      else {
        static_make_string16(prefix, L"[no file found]");
        SetWindowTextW(g_window.handle, (LPCWSTR)prefix.ptr);
      }
    }
    
    g_app_input = (t_app_input) {0};
  }
  
  return(0);
}
