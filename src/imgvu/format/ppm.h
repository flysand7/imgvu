/* date = October 17th 2020 9:04 pm */

#ifndef PPM_H
#define PPM_H

struct t_ppm_stream {
  byte* ptr;
  u64 pos;
  u64 size;
  bool error;
};

internal bool ppm_is_digit(byte character) {
  return(character >= '0' && character <= '9');
}

internal bool ppm_is_white(byte character) {
  return(character == 0x09 || character == 0x0a || character == 0x0d || character == 0x20);
}

// TODO(bumbread): skip comments
internal u32 ppm_next_number(struct t_ppm_stream* stream) {
  if(stream->error) return(0);
  
  loop {
    byte character = stream->ptr[stream->pos];
    if(stream->pos >= stream->size) goto error;
    else if(character == 0) goto error;
    else if(ppm_is_white(character)) stream->pos += 1;
    else if(character == '#') {
      loop {
        stream->pos += 1;
        character = stream->ptr[stream->pos];
        if(character == '\n') break;
      }
    }
    else break;
  }
  
  u32 result = 0;
  loop {
    if(stream->pos >= stream->size) goto error;
    byte character = stream->ptr[stream->pos];
    if(ppm_is_digit(character)) {
      u32 digit = (u32)(character - '0');
      result *= 10;
      result += digit;
    }
    else if(ppm_is_white(character)) break;
    else if(character == 0) break;
    else if(character == '#') break;
    else goto error;
    
    stream->pos += 1;
  }
  return(result);
  error:
  stream->error = true;
  return(0);
}

internal void try_parse_ppm(t_image_data* data, t_image* result) {
  assert(result->pixels == 0);
  if(result->skip) return;
  
  struct t_ppm_stream stream;
  stream.ptr = (byte*)data->ptr;
  stream.pos = 0;
  stream.size = data->size;
  
  if(stream.size > 2) {
    if(stream.ptr[0] == 'P') {
      bool isGreyAscii  = (stream.ptr[1] == '2');
      bool isPixelAscii = (stream.ptr[1] == '3');
      bool isGreyBytes  = (stream.ptr[1] == '5');
      bool isPixelBytes = (stream.ptr[1] == '6');
      
      if(isPixelAscii || isPixelBytes || isGreyBytes || isGreyAscii) {
        stream.pos += 2;
        
        u32 width = ppm_next_number(&stream);
        u32 height = ppm_next_number(&stream);
        u32 range = ppm_next_number(&stream);
        if(stream.error) goto error;
        if(width == 0 || height == '0' || range == '0') goto error;
        
        result->width = width;
        result->height = height;
        result->pixels = malloc(result->width*result->height * sizeof(u32));
        u32* pixels = result->pixels;
        
        if(isPixelAscii) {
          for(u32 column = 0; column < width; column += 1) {
            for(u32 row = 0; row < height; row += 1) {
              
              u32 x = ppm_next_number(&stream);
              u32 y = ppm_next_number(&stream);
              u32 z = ppm_next_number(&stream);
              if(stream.error) goto error;
              if((x > range) | (y > range) | (z > range)) goto error;
              
              u32 r = (x*0xff) / range;
              u32 g = (y*0xff) / range;
              u32 b = (z*0xff) / range;
              
              u32 color = r | (g << 8) | (b << 16) | (0xff000000);
              pixels[column + row * width] = color;
            }
          }
        }
        
        else if(isPixelBytes) {
          if(range >= 0x100) goto error;
          
          // NOTE(bumbread): only single whitespace character is allowed
          if(ppm_is_white(stream.ptr[stream.pos])) {
            stream.pos += 1;
          }
          for(u32 column = 0; column < width; column += 1) {
            for(u32 row = 0; row < height; row += 1) {
              if(stream.pos + 3 > stream.size) goto error;
              u32 r = stream.ptr[stream.pos+0];
              u32 g = stream.ptr[stream.pos+1];
              u32 b = stream.ptr[stream.pos+2];
              stream.pos += 3;
              
              u32 color = r | (g << 8) | (b << 16) | (0xff000000);
              pixels[column + row * width] = color;
            }
          }
        }
        
        else if(isGreyAscii) {
          if(range >= 0x1000) goto error;
          for(u32 column = 0; column < width; column += 1) {
            for(u32 row = 0; row < height; row += 1) {
              
              u32 v = ppm_next_number(&stream);
              if(stream.error) goto error;
              if(v > range) goto error;
              
              u32 g = (v*0xff) / range;
              u32 color = g | (g << 8) | (g << 16) | (0xff000000);
              pixels[column + row * width] = color;
            }
          }
        }
        
        else if(isGreyBytes) {
          if(range >= 0x100) goto error;
          for(u32 column = 0; column < width; column += 1) {
            for(u32 row = 0; row < height; row += 1) {
              
              if(stream.pos + 1 > stream.size) goto error;
              u32 v = stream.ptr[stream.pos];
              if(v > range) goto error;
              stream.pos += 1;
              
              u32 g = (v*0xff) / range;
              u32 color = g | (g << 8) | (g << 16) | (0xff000000);
              pixels[column + row * width] = color;
            }
          }
        }
      }
    }
  }
  
  return;
  
  error: 
  result->skip = true;
  result->width = 0;
  result->height = 0;
  if(result->pixels) free(result->pixels);
  return;
}

#endif //PPM_H
