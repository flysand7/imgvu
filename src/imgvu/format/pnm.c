/* date = October 17th 2020 9:04 pm */

#ifndef PNM_H
#define PNM_H

struct t_pnm_stream {
  byte* ptr;
  u64 pos;
  u64 size;
  bool error;
};

internal bool pnm_is_digit(byte character) {
  return(character >= '0' && character <= '9');
}

internal bool pnm_is_white(byte character) {
  return(character == 0x09 || character == 0x0a || character == 0x0d || character == 0x20);
}

// TODO(bumbread): skip comments
internal u32 pnm_next_number(struct t_pnm_stream* stream) {
  if(stream->error) return(0);
  
  loop {
    byte character = stream->ptr[stream->pos];
    if(stream->pos >= stream->size) goto error;
    else if(character == 0) goto error;
    else if(pnm_is_white(character)) stream->pos += 1;
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
    if(pnm_is_digit(character)) {
      u32 digit = (u32)(character - '0');
      result *= 10;
      result += digit;
    }
    else if(pnm_is_white(character)) break;
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

internal void try_parse_pnm(t_image_data* data, t_image* result) {
  assert(result->pixels == 0);
  if(result->success) return;
  
  struct t_pnm_stream stream;
  stream.ptr = (byte*)data->ptr;
  stream.pos = 0;
  stream.size = data->size;
  stream.error = false;
  
  if(stream.size > 2) {
    if(stream.ptr[0] == 'P') {
      bool isBitAscii   = (stream.ptr[1] == '1');
      bool isGreyAscii  = (stream.ptr[1] == '2');
      bool isPixelAscii = (stream.ptr[1] == '3');
      bool isBitBytes   = (stream.ptr[1] == '4');
      bool isGreyBytes  = (stream.ptr[1] == '5');
      bool isPixelBytes = (stream.ptr[1] == '6');
      
      if(isPixelAscii || isPixelBytes || isGreyBytes || isGreyAscii) {
        stream.pos += 2;
        
        u32 width = pnm_next_number(&stream);
        u32 height = pnm_next_number(&stream);
        u32 range = pnm_next_number(&stream);
        if(stream.error) goto error;
        if(width == 0 || height == '0' || range == '0') goto error;
        
        result->width = width;
        result->height = height;
        result->pixels = malloc(result->width*result->height * sizeof(u32));
        u32* pixels = result->pixels;
        
        if(isPixelAscii) {
          for(u32 row = height-1;; row -= 1) {
            for(u32 column = 0; column < width; column += 1) {
              
              u32 x = pnm_next_number(&stream);
              u32 y = pnm_next_number(&stream);
              u32 z = pnm_next_number(&stream);
              if(stream.error) goto error;
              if((x > range) | (y > range) | (z > range)) goto error;
              
              u32 r = (x*0xff) / range;
              u32 g = (y*0xff) / range;
              u32 b = (z*0xff) / range;
              
              u32 color = r | (g << 8) | (b << 16) | (0xff000000);
              pixels[column + row * width] = color;
            }
            if(row == 0) break;
          }
          result->success = true;
        }
        
        else if(isPixelBytes) {
          if(range >= 0x100) goto error;
          if(pnm_is_white(stream.ptr[stream.pos])) {
            stream.pos += 1;
          }
          for(u32 row = height-1;; row -= 1) {
            for(u32 column = 0; column < width; column += 1) {
              if(stream.pos + 3 > stream.size) goto error;
              u32 r = stream.ptr[stream.pos+0];
              u32 g = stream.ptr[stream.pos+1];
              u32 b = stream.ptr[stream.pos+2];
              stream.pos += 3;
              
              u32 color = r | (g << 8) | (b << 16) | (0xff000000);
              pixels[column + row * width] = color;
            }
            if(row == 0) break;
          }
          result->success = true;
        }
        
        else if(isGreyAscii) {
          if(range >= 0x1000) goto error;
          for(u32 row = height-1;; row -= 1) {
            for(u32 column = 0; column < width; column += 1) {
              
              u32 v = pnm_next_number(&stream);
              if(stream.error) goto error;
              if(v > range) goto error;
              
              u32 g = (v*0xff) / range;
              u32 color = g | (g << 8) | (g << 16) | (0xff000000);
              pixels[column + row * width] = color;
            }
            if(row == 0) break;
          }
          result->success = true;
        }
        
        else if(isGreyBytes) {
          if(range >= 0x100) goto error;
          if(pnm_is_white(stream.ptr[stream.pos])) {
            stream.pos += 1;
          }
          for(u32 row = height-1;; row -= 1) {
            for(u32 column = 0; column < width; column += 1) {
              if(stream.pos + 1 > stream.size) goto error;
              u32 v = stream.ptr[stream.pos];
              if(v > range) goto error;
              stream.pos += 1;
              
              u32 g = (v*0xff) / range;
              u32 color = g | (g << 8) | (g << 16) | (0xff000000);
              pixels[column + row * width] = color;
            }
            if(row == 0) break;
          }
          result->success = true;
        }
      }
      else if(isBitAscii || isBitBytes) {
        stream.pos += 2;
        
        u32 width = pnm_next_number(&stream);
        u32 height = pnm_next_number(&stream);
        if(stream.error) goto error;
        if(width == 0 || height == '0') goto error;
        
        result->width = width;
        result->height = height;
        result->pixels = malloc(result->width*result->height * sizeof(u32));
        u32* pixels = result->pixels;
        
        u32 columnCounter = 0;
        u32 rowCounter = height-1;
        
        if(isBitAscii) {
          loop {
            if(stream.pos + 1 > stream.size) goto error;
            byte character = stream.ptr[stream.pos];
            if(character == '0' || character == '1') {
              u32 color = ((character == '1')?(0xff000000):(0xffffffff));
              pixels[columnCounter + rowCounter * result->width] = color;
              columnCounter += 1;
              if(columnCounter == width) {
                columnCounter = 0;
                rowCounter -= 1;
                if(rowCounter == 0) break;
              }
              stream.pos += 1;
            }
            else if(character == '#') {
              loop {
                character = stream.ptr[stream.pos];
                if(character == '\n') break;
                if(character == 0) break;
                stream.pos += 1;
              }
            }
            else if(character == 0) break;
            else {
              stream.pos += 1;
            }
          }
          
          result->success = true;
        }
        else if(isBitBytes) {
          if(pnm_is_white(stream.ptr[stream.pos])) {
            stream.pos += 1;
          }
          u32 bitCounter = 7;
          u64 lastRowPos = stream.pos;
          u64 rowSize = (width+7)/8;
          loop {
            if(stream.pos >= stream.size) goto error;
            u32 lastByte = stream.ptr[stream.pos];
            u32 currentBit = (lastByte >> bitCounter) & 1;
            u32 color = (currentBit == 1) ? (0xff000000) : (0xffffffff);
            pixels[columnCounter + rowCounter * result->width] = color;
            
            columnCounter += 1;
            if(columnCounter == width) {
              if(rowCounter == 0) break;
              rowCounter -= 1;
              columnCounter = 0;
              
              bitCounter = 8;
              
              lastRowPos += rowSize;
              stream.pos = lastRowPos;
            }
            
            if(bitCounter == 0) {
              bitCounter = 8;
              stream.pos += 1;
            }
            bitCounter -= 1;
          }
          
          result->success = true;
        }
      }
    }
  }
  
  return;
  
  error: 
  result->success = false;
  if(result->pixels) free(result->pixels);
  result->pixels = 0;
  result->width = 0;
  result->height = 0;
  return;
}

#endif //PNM_H
