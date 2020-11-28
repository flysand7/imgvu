
internal void gdi_clear_screen(u32 color) {
  for(u32 row = 0; row < g_window.clientHeight; row += 1) {
    for(u32 column = 0; column < g_window.clientWidth; column += 1) {
      g_window.pixels[row*g_window.clientWidth + column] = color;
    }
  }
}

internal v2 gdi_transform(t_location* loc, v2 halfImageSize, v2 halfWindowSize, v2 pos) {
  v2 result = pos;
  result = v2_sub(result, halfImageSize);
  if(loc->flippedX) result.x = -result.x;
  if(loc->flippedY) result.y = -result.y;
  result = v2_mul(result, loc->scale);
  result = v2_rotate(result, loc->angle);
  result = v2_add(result, loc->position);
  result = v2_add(result, halfWindowSize);
  return(result);
}

internal void gdi_draw_image(t_location* loc, t_image* image) {
  i32 windowWidth = (i32)g_window.clientWidth;
  i32 windowHeight = (i32)g_window.clientHeight;
  
  i32 width = (i32)image->width;
  i32 height = (i32)image->height;
  
  v2 halfWindowSize = {(r32)windowWidth/2.0f, (r32)windowHeight/2.0f};
  v2 halfImageSize = {(r32)image->width/2.0f, (r32)image->height/2.0f};
  
  v2 angles[] = {
    {-1.0f, -1.0f},
    {(r32)(width + 1), -1.0f},
    {-1.0f, (r32)(height + 1)},
    {(r32)(width + 1), (r32)(height + 1)}
  };
  
  angles[0] = gdi_transform(loc, halfImageSize, halfWindowSize, angles[0]);
  angles[1] = gdi_transform(loc, halfImageSize, halfWindowSize, angles[1]);
  angles[2] = gdi_transform(loc, halfImageSize, halfWindowSize, angles[2]);
  angles[3] = gdi_transform(loc, halfImageSize, halfWindowSize, angles[3]);
  
  r32 minX = angles[0].x;
  r32 maxX = angles[0].x;
  r32 minY = angles[0].y;
  r32 maxY = angles[0].y;
  
  for(u32 i = 1; i < array_length(angles); i += 1) {
    v2 item = angles[i];
    if(item.x < minX) minX = item.x;
    if(item.y < minY) minY = item.y;
    if(item.x > maxX) maxX = item.x;
    if(item.y > maxY) maxY = item.y;
  }
  
  if(maxX <= 0.0f) return;
  if(maxY <= 0.0f) return;
  if(minX > (r32)windowWidth) return;
  if(minY > (r32)windowHeight) return;
  
  if(minX < 0.0f) minX = 0.0f;
  if(minY < 0.0f) minY = 0.0f;
  if(maxX > (r32)windowWidth) maxX = (r32)windowWidth;
  if(maxY > (r32)windowHeight) maxY = (r32)windowHeight;
  
  u32 rowMin = (u32)floor32(minY);
  u32 rowMax = (u32)ceil32(maxY);
  u32 columnMin = (u32)floor32(minX);
  u32 columnMax = (u32)ceil32(maxX);
  
  r32 r_cos = cos32(-loc->angle);
  r32 r_sin = sin32(-loc->angle);
  r32 r_scl = 1.0f / loc->scale;
  r32 r_sclx = r_scl;
  r32 r_scly = r_scl;
  if(loc->flippedX) r_sclx = -r_sclx;
  if(loc->flippedY) r_scly = -r_scly;
  
  for(u32 row = rowMin; row < rowMax; row += 1) {
    for(u32 column = columnMin; column < columnMax; column += 1) {
      v2 reversePosition = { (r32)column, (r32)row };
      
      reversePosition.x -= halfWindowSize.x;
      reversePosition.y -= halfWindowSize.y;
      
      reversePosition.x -= loc->position.x;
      reversePosition.y -= loc->position.y;
      
      r32 rx = reversePosition.x;
      r32 ry = reversePosition.y;
      reversePosition.x = rx*r_cos - ry*r_sin;
      reversePosition.y = rx*r_sin + ry*r_cos;
      
      reversePosition.x *= r_sclx;
      reversePosition.y *= r_scly;
      
      reversePosition.x += halfImageSize.x;
      reversePosition.y += halfImageSize.y;
      
      i32 intPixelX = (i32)reversePosition.x;
      i32 intPixelY = (i32)reversePosition.y;
      if(intPixelX >= 0 && intPixelY >= 0 && intPixelX < width && intPixelY < height) {
        t_colorf imagePixel = image->pixels[intPixelX + intPixelY * width];
        u32 color = colorf_to_rgba(imagePixel);
        g_window.pixels[column + row*g_window.clientWidth] = color;
      }
    }
  }
}

internal void gdi_show(void) {
  assert(g_window.pixels);
  
  BITMAPINFO bitmapInfo = {0};
  bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
  bitmapInfo.bmiHeader.biWidth = (LONG)g_window.clientWidth;
  bitmapInfo.bmiHeader.biHeight = (LONG)g_window.clientHeight;
  bitmapInfo.bmiHeader.biPlanes = 1;
  bitmapInfo.bmiHeader.biBitCount = 32;
  bitmapInfo.bmiHeader.biCompression = BI_RGB;
  
  StretchDIBits(g_window.deviceContext,
                0, 0, (LONG)g_window.clientWidth, (LONG)g_window.clientHeight,
                0, 0, (LONG)g_window.clientWidth, (LONG)g_window.clientHeight,
                g_window.pixels, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

internal void win_create_gl_context(void) {
  platform_profile_state_push("gl_init");
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
  
  platform_profile_state_pop();
}

internal void gl_clear_screen(u32 color) {
  u32 r = (color >> 0)  & 0xff;
  u32 g = (color >> 8)  & 0xff;
  u32 b = (color >> 16) & 0xff;
  glClearColor((r32)r / 255.0f, (r32)g/255.0f, (r32)b/255.0f, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
}

internal void gl_draw_image(t_location* loc, t_image* image) {
  glEnable(GL_TEXTURE_2D);
  
  u32 texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  {
    u32* temp = (u32*)malloc(image->width * image->height * sizeof(u32));
    for(u32 pixelIndex = 0; pixelIndex < image->width*image->height; pixelIndex += 1) {
      temp[pixelIndex] = colorf_to_rgba(image->pixels[pixelIndex]);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (i32)image->width, (i32)image->height, 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, (u32*)temp);
    free(temp);
  }
  
  glViewport(0, 0, (i32)g_window.clientWidth, (i32)g_window.clientHeight);
  glLoadIdentity();
  
  r64 halfWidth = (r64)g_window.clientWidth / 2.0;
  r64 halfHeight = (r64)g_window.clientHeight / 2.0;
  glOrtho(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0, 1.0);
  glTranslatef(loc->position.x, loc->position.y, 0);
  glRotatef(rad_to_deg(loc->angle), 0,0,1);
  r32 imageWidth = (r32)image->width;
  r32 imageHeight = (r32)image->height;
  if(loc->flippedX) imageWidth = -imageWidth;
  if(loc->flippedY) imageHeight = -imageHeight;
  imageWidth *= loc->scale;
  imageHeight *= loc->scale;
  glScalef(imageWidth, imageHeight, 1.0f);
  
  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0); glVertex3f(-0.5f, -0.5f, 0.0f);
  glTexCoord2f(0.0, 1.0); glVertex3f(-0.5f, 0.5f, 0.0f);
  glTexCoord2f(1.0, 1.0); glVertex3f(0.5f, 0.5f, 0.0f);
  glTexCoord2f(1.0, 0.0); glVertex3f(0.5f, -0.5f, 0.0f);
  glEnd();
  
  glLoadIdentity();
  glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
  
  glDeleteTextures(1, &texture);
}

internal void gl_show(void) {
  glFlush();
  SwapBuffers(g_window.deviceContext);
}
