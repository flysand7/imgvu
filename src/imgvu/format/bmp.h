/* date = October 17th 2020 4:58 am */

#ifndef BMP_H
#define BMP_H

// TODO(bumbread): support more modes for this format

#pragma pack(push, 1)
struct {
  char sign[2];
  u32 bitmapSize;
  u32 reserved;
  u32 offset;
  
  u32 infoHeaderSize;
  i32 width;
  i32 height;
  u16 planes;
  u16 bitsPerPixel;
  u32 compression;
  u32 imageSize;
  u32 h_pixelsPerMeter;
  u32 v_pixelsPerMeter;
  u32 paletteColorsNum;
  u32 colorsImportant;
} typedef t_bitmap_header;
#pragma pack(pop)

internal bool try_parse_bmp(t_image_data* file, t_image* result) {
  byte* stream = (byte*)file->ptr;
  
  if(stream[0] == 'B' && stream[1] == 'M') {
    t_bitmap_header* bitmapHeader = (t_bitmap_header*)stream;
    
    bool flipped = false;
    u32 imageWidth = (u32)bitmapHeader->width;
    u32 imageHeight = 0;
    if(bitmapHeader->height < 0) {
      flipped = true;
      imageHeight = (u32)(-bitmapHeader->height);
    }
    else imageHeight = (u32)bitmapHeader->height;
    u32 rowSize = 4*((imageWidth * bitmapHeader->bitsPerPixel + 31) / 32);
    
    u32 imageSize = bitmapHeader->imageSize;
    if(imageSize == 0) {
      imageSize = imageHeight*rowSize;
    }
    
    if(imageSize <= file->size) {
      if(bitmapHeader->offset < file->size) {
        if(bitmapHeader->bitsPerPixel == 24) {
          
          result->width = imageWidth;
          result->height = imageHeight;
          result->pixels = malloc(result->width*result->height*sizeof(u32));
          
          u32* targetRow = result->pixels;
          byte* pixelRow = stream + bitmapHeader->offset;
          for(u32 row = 0; row < imageHeight; row += 1) {
            byte* channel = pixelRow;
            u32* targetPixel = targetRow;
            for(u32 column = 0; column < imageWidth; column += 1) {
              u32 r = *channel; channel += 1;
              u32 g = *channel; channel += 1;
              u32 b = *channel; channel += 1;
              u32 color = (b<<16) | (g<<8) | (r<<0) | (0xFF000000);
              *targetPixel = color;
              targetPixel += 1;
            }
            targetRow += result->width;
            pixelRow += rowSize;
          }
          return(0);
        } else return(1);
      } else return(1);
    } else return(1);
  } else return(1);
}

#endif //BMP_H
