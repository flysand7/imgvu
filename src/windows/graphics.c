
internal void gdi_clear_screen(u32 color) {
  for(u32 row = 0; row < g_window.clientHeight; row += 1) {
    for(u32 column = 0; column < g_window.clientWidth; column += 1) {
      g_window.pixels[row*g_window.clientWidth + column] = color;
    }
  }
}

internal v2 gdi_transform(t_location* loc, v2 halfImageSize, v2 halfWindowSize, v2 pos) {
  v2 result = pos;
  if(loc->flippedX) result.x = -result.x;
  if(loc->flippedY) result.y = -result.y;
  result = v2_sub(result, halfImageSize);
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
  v2 imagePosition = v2_add(loc->position, halfWindowSize);
  
  v2 angles[] = {
    {0.0f, 0.0f},
    {(r32)width, 0.0f},
    {0.0f, (r32)height},
    {(r32)width, (r32)height}
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
  
  for(u32 row = (u32)floor32(minY); row < (u32)ceil32(maxY); row += 1) {
    for(u32 column = (u32)floor32(minX); column < (u32)ceil32(maxX); column += 1) {
      v2 screenPixel = { (r32)column, (r32)row };
      
      v2 reversePosition;
      reversePosition = v2_sub(screenPixel, imagePosition);
      reversePosition = v2_rotate(reversePosition, -loc->angle);
      reversePosition = v2_mul(reversePosition, 1.0f / loc->scale);
      if(loc->flippedX) reversePosition.x = -reversePosition.x;
      if(loc->flippedY) reversePosition.y = -reversePosition.y;
      v2 pixelPos = v2_add(reversePosition, halfImageSize);
      
      i32 intPixelX = (i32)pixelPos.x;
      i32 intPixelY = (i32)pixelPos.y;
      if(intPixelX >= 0 && intPixelY >= 0 && intPixelX < width && intPixelY < height) {
        g_window.pixels[column + row*g_window.clientWidth] = image->pixels[intPixelX + intPixelY * width];
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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (i32)image->width, (i32)image->height, 
               0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
  
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
