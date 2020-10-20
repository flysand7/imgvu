/* date = October 17th 2020 4:58 am */

#ifndef BMP_H
#define BMP_H

struct t_bmp_stream {
  byte* ptr;
  u32 pos;
  u64 size;
  bool error;
};

internal u32 bmp_read_u2(struct t_bmp_stream* stream) {
  if(stream->error) return(0);
  if(stream->pos + 2 > stream->size) {
    stream->error = true;
    return(0);
  }
  u32 x = stream->ptr[stream->pos+0];
  u32 y = stream->ptr[stream->pos+1];
  stream->pos += 2;
  u32 result = x | (y << 8);
  return(result);
}

internal u32 bmp_read_u4(struct t_bmp_stream* stream) {
  if(stream->error) return(0);
  if(stream->pos + 4 > stream->size) {
    stream->error = true;
    return(0);
  }
  u32 x = stream->ptr[stream->pos+0];
  u32 y = stream->ptr[stream->pos+1];
  u32 z = stream->ptr[stream->pos+2];
  u32 w = stream->ptr[stream->pos+3];
  stream->pos += 4;
  u32 result = x | (y << 8) | (z << 16) | (w << 24);
  return(result);
}

internal i32 bmp_read_i4(struct t_bmp_stream* stream) {
  if(stream->error) return(0);
  if(stream->pos + 4 > stream->size) {
    stream->error = true;
    return(0);
  }
  u32 x = stream->ptr[stream->pos+0];
  u32 y = stream->ptr[stream->pos+1];
  u32 z = stream->ptr[stream->pos+2];
  u32 w = stream->ptr[stream->pos+3];
  stream->pos += 4;
  u32 result_u4 = x | (y << 8) | (z << 16) | (w << 24);
  bool sign = (result_u4>>31);
  i32 result = (result_u4 & 0x7fffffff);
  if(sign == true) {
    result= -result;
  }
  return(result);
}

internal void try_parse_bmp(t_image_data* file, t_image* result) {
  if(result->success) return;
  
  struct t_bmp_stream stream;
  stream.ptr = (byte*)file->ptr;
  stream.pos = 0;
  stream.size = file->size;
  stream.error = false;
  
  if(stream.size >= 2) {
    if(stream.ptr[0] == 'B' && stream.ptr[1] == 'M') {
      stream.pos += 2;
      
      u32 fileSize = bmp_read_u4(&stream);
      if(stream.error == true) goto error;
      if(fileSize == 0) goto error;
      if(fileSize != stream.size) goto error;
      bmp_read_u4(&stream);
      
      u32 dataOffset = bmp_read_u4(&stream);
      if(stream.error == true) goto error;
      if(dataOffset <= fileSize) {
        
        u32 headerSize = bmp_read_u4(&stream);
        if(headerSize == 40) {
          i32 width = bmp_read_i4(&stream);
          i32 height = bmp_read_i4(&stream);
          u32 planes = bmp_read_u2(&stream);
          if(planes != 1) goto error;
          u32 bitsPerPixel = bmp_read_u2(&stream);
          u32 compression = bmp_read_u4(&stream);
          u32 imageSize = bmp_read_u4(&stream);
          bmp_read_u4(&stream);
          bmp_read_u4(&stream);
          u32 colorsUsed = bmp_read_u4(&stream);
          u32 importantColors = bmp_read_u4(&stream);
          
          if(stream.error) goto error;
          if((width == 0) || (height == 0)) goto error;
          
          u32 paletteStartPos = stream.pos;
          u32 paletteMaxSize = dataOffset - paletteStartPos;
          if(colorsUsed * sizeof(u32) > paletteMaxSize) goto error;
          
          bool usesPalette = (0
                              || (bitsPerPixel == 1)
                              || (bitsPerPixel == 2)
                              || (bitsPerPixel == 4)
                              || (bitsPerPixel == 8)
                              );
          if(usesPalette && bitsPerPixel * sizeof(u32) > paletteMaxSize) goto error;
          u32* palette = (u32*)(void*)(stream.ptr + stream.pos);
          
          assert(width > 0);
          u32 rowSize = (((((u32)width * bitsPerPixel) + 31) / 32) * 4);
          
          // NOTE(bumbread): 
          // https://docs.microsoft.com/en-us/previous-versions/ms969901(v=msdn.10)?redirectedfrom=MSDN
          // for more information
          
          // TODO(bumbread): make sure height is positive;
          // go the opposite way otherwise
          // TODO(bumbread): 4 and 8bit RLE compressed data.
          
          debug_variable_unused(compression);
          debug_variable_unused(imageSize);
          debug_variable_unused(importantColors);
          
          result->width = (u32)width;
          result->height = (u32)height;
          result->pixels = malloc(result->width * result->height * sizeof(u32));
          
          u32 columnCounter = 0;
          u32 rowCounter = 0;
          
          switch(bitsPerPixel) {
            case(1):
            case(2):
            case(4):
            case(8): {
              
              u32 dataCounter = dataOffset;
              u32 bitCounter = 6;
              
              loop {
                
                if(dataCounter >= stream.size) goto error;
                u32 bitValues = (dataCounter >> bitCounter) & ((1<<bitsPerPixel)-1);
                u32 color = palette[bitValues];
                result->pixels[rowCounter*result->width + columnCounter] = color;
                
                columnCounter += 1;
                if(columnCounter == result->width) {
                  rowCounter += 1;
                  if(rowCounter == result->height) break;
                  columnCounter = 0;
                  
                  bitCounter = 8;
                  dataCounter = rowCounter * rowSize;
                }
                
                if(bitCounter == 0) {
                  bitCounter = 8;
                  dataCounter += 1;
                }
                
                bitCounter -= bitsPerPixel;
              }
              
              result->success = true;
              return;
              
            } break;
            case(16): {
              
              byte* data = stream.ptr + dataOffset;
              u32 dataCounter = 0;
              loop {
                u32 x = data[0];
                u32 y = data[1];
                data += 2;
                dataCounter += 2;
                if(dataCounter >= stream.size) goto error;
                
                u32 word_ = (x) | (y);
                
                u32 max = ((1<<5)-1);
                u32 r = ((word_ >> 1)  & ((1<<5)-1)) * 255 / max;
                u32 g = ((word_ >> 5)  & ((1<<5)-1)) * 255 / max;
                u32 b = ((word_ >> 10) & ((1<<5)-1)) * 255 / max;
                u32 color = (r) | (g << 8) | (b << 16) | (0xff000000);
                result->pixels[columnCounter + rowCounter * result->width] = color;
                
                columnCounter += 1;
                if(columnCounter == result->width) {
                  rowCounter += 1;
                  if(rowCounter == result->height) break;
                  columnCounter = 0;
                }
              }
              
              result->success = true;
              return;
              
            } break;
            case(24): {
              
              // TODO(bumbread): 64K boundary handling
              
              byte* data = stream.ptr + dataOffset;
              u32 dataCounter = 0;
              loop {
                u32 b = data[0];
                u32 g = data[1];
                u32 r = data[2];
                data += 3;
                dataCounter += 3;
                if(dataCounter >= stream.size) goto error;
                
                u32 color = (r) | (g<<8) | (b<<16) | (0xff000000);
                result->pixels[columnCounter + rowCounter * result->width] = color;
                
                columnCounter += 1;
                if(columnCounter == result->width) {
                  rowCounter += 1;
                  if(rowCounter == result->height) break;
                  columnCounter = 0;
                }
              }
              
              result->success = true;
              return;
              
            } break;
            case(32): {
              
              byte* data = stream.ptr + dataOffset;
              u32 dataCounter = 0;
              loop {
                u32 b = data[0];
                u32 g = data[1];
                u32 r = data[2];
                u32 a = data[3];
                data += 4;
                dataCounter += 4;
                if(dataCounter >= stream.size) goto error;
                
                u32 color = (r) | (g<<8) | (b<<16) | (a<<24);
                result->pixels[columnCounter + rowCounter * result->width] = color;
                
                columnCounter += 1;
                if(columnCounter == result->width) {
                  rowCounter += 1;
                  if(rowCounter == result->height) break;
                  columnCounter = 0;
                }
              }
              
              result->success = true;
              return;
              
            } break;
            
            default: goto error;
          }
        }
      }
    }
  }
  
  error:
  result->success = false;
  if(result->pixels) free(result->pixels);
  result->pixels = 0;
  result->width = 0;
  result->height = 0;
}

#endif //BMP_H
