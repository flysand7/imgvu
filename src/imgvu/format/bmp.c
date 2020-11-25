/* date = October 17th 2020 4:58 am */

#ifndef BMP_H
#define BMP_H

#define BMP_FIRST_WORD 0x4D42

#define BMP_COMPRESSION_RGB           0x00
#define BMP_COMPRESSION_RLE8          0x01
#define BMP_COMPRESSION_RLE4          0x02
#define BMP_COMPRESSION_BITFIELDS     0x03
#define BMP_COMPRESSION_PNG           0x04 //unsupported
#define BMP_COMPRESSION_JPEG          0x05 //unsupported

#define BMP_COLORSPACE_CAL_RGB        0x00
#define BMP_COLORSPACE_sRGB           0x01
#define BMP_COLORSPACE_WINDOWS        0x02
#define BMP_COLORSPACE_PROFILE_LINKED 0x03 // unsupported
#define BMP_COLORSPACE_PROFILE_EMBED  0x04 // unsupported

#define BMP_DIB_VERSION_CORE          12
#define BMP_DIB_VERSION_V1            40
#define BMP_DIB_VERSION_V2            52 //not officially supported
#define BMP_DIB_VERSION_V3            56 //not officially supported
#define BMP_DIB_VERSION_V4            108
#define BMP_DIB_VERSION_V5            124

struct {
  u32 fileSize;
  u32 dataOffset;
  u32 dataSize;
  u32 planes;
  
  u32 bitsPerPixel;
  u32 bitmapWidth;
  u32 bitmapHeight;
  u32 pitch;
  u32 imageSize;
  
  u32 colorsUsed;
  byte* palette;
  
  u32 redMask;
  u32 greenMask;
  u32 blueMask;
  u32 alphaMask;
  
  u32 compressionMethod;
  u32 colorSpace;
  m3  colorSpaceTransform;
  
  r32 redGamma;
  r32 blueGamma;
  r32 greenGamma;
  
  bool flipped;
} typedef t_bmp_data;

internal inline bool bmp_check_bits_per_pixel(u32 bitsPerPixel) {
  if(0
     || (bitsPerPixel == 1)
     || (bitsPerPixel == 4)
     || (bitsPerPixel == 8)
     || (bitsPerPixel == 16)
     || (bitsPerPixel == 24)
     || (bitsPerPixel == 32)
     ) {
    return(true);
  }
  return(false);
}

internal inline void bitmap_load_headers(t_bmp_data* data, t_stream* stream) {
  assert(stream->error == false);
  
  data->flipped = false;
  
  u32 dibHeaderVersion = stream_read_u32_le(stream);
  if(dibHeaderVersion == BMP_DIB_VERSION_CORE) {
    data->bitmapWidth  = (u32)stream_read_u16_le(stream);
    data->bitmapHeight = (u32)stream_read_u16_le(stream);
  }
  else {
    i32 width  = stream_read_i32_le(stream);
    i32 height = stream_read_i32_le(stream);
    if(width < 0) {stream->error = true; return; }
    if(height < 0) {data->flipped = true;height = -height;}
    data->bitmapWidth  = (u32)width;
    data->bitmapHeight = (u32)height;
  }
  
  data->planes = (u32)stream_read_u16_le(stream);
  if(data->planes != 1) { stream->error = true; return; }
  data->bitsPerPixel = (u32)stream_read_u16_le(stream);
  if(bmp_check_bits_per_pixel(data->bitsPerPixel) == false) { stream->error = true; return; }
  
  data->pitch = ((data->bitsPerPixel*data->bitmapWidth + 31) / 32) * 4;
  data->imageSize = data->bitmapHeight * data->pitch;
  
  if(data->bitsPerPixel <= 8) {
    data->colorsUsed = (1 << data->bitsPerPixel);
  }
  
  switch(data->bitsPerPixel) {
    case(16): {
      data->redMask   = 0xf8000000; // 1111 1000.0000 0000.0000 0000.0000 0000
      data->greenMask = 0x07c00000; // 0000 0111.1100 0000.0000 0000.0000 0000
      data->blueMask  = 0x003e0000; // 0000 0000.0011 1110.0000 0000.0000 0000
      data->alphaMask = 0x00000000; // 0000 0000.0000 0000.0000 0000.0000 0000
    } break;
    case(24): {
      data->redMask   = 0xff000000; // 1111 1111.0000 0000.0000 0000.0000 0000
      data->greenMask = 0x00ff0000; // 0000 0000.1111 1111.0000 0000.0000 0000
      data->blueMask  = 0x0000ff00; // 0000 0000.0000 0000.1111 1111.0000 0000
      data->alphaMask = 0x00000000; // 0000 0000.0000 0000.0000 0000.0000 0000
    } break;
    case(32): {
      data->redMask   = 0xff000000; // 1111 1111.0000 0000.0000 0000.0000 0000
      data->greenMask = 0x00ff0000; // 0000 0000.1111 1111.0000 0000.0000 0000
      data->blueMask  = 0x0000ff00; // 0000 0000.0000 0000.1111 1111.0000 0000
      data->alphaMask = 0x000000ff; // 0000 0000.0000 0000.0000 0000.1111 1111
    } break;
    default: {
      data->redMask   = 0x00000000; // 0000 0000.0000 0000.0000 0000.0000 0000
      data->greenMask = 0x00000000; // 0000 0000.0000 0000.0000 0000.0000 0000
      data->blueMask  = 0x00000000; // 0000 0000.0000 0000.0000 0000.0000 0000
      data->alphaMask = 0x00000000; // 0000 0000.0000 0000.0000 0000.0000 0000
    }
  }
  
  data->compressionMethod = BMP_COMPRESSION_RGB;
  data->colorSpace = BMP_COLORSPACE_CAL_RGB;
  data->colorSpaceTransform = color_transform_create_rgbtoxyz();
  data->redGamma   = 2.2f;
  data->greenGamma = 2.2f;
  data->blueGamma  = 2.2f;
  
  if(dibHeaderVersion >= BMP_DIB_VERSION_V1) {
    data->compressionMethod = stream_read_u32_le(stream);
    stream_read_u32_le(stream); // image size
    stream_read_u32_le(stream); // x pixels per meter
    stream_read_u32_le(stream); // y pixels per meter
    u32 colorsUsed = stream_read_u32_le(stream);
    if(colorsUsed != 0) {data->colorsUsed = colorsUsed;}
    stream_read_u32_le(stream); // colors important
    
    // NOTE(bumbread): unsupported compression methods
    if((data->compressionMethod == BMP_COMPRESSION_JPEG)
       || (data->compressionMethod == BMP_COMPRESSION_PNG)) {
      stream->error = true;
    }
    
    if(stream->error == true) {return;}
  }
  
  if(dibHeaderVersion >= BMP_DIB_VERSION_V4) {
    u32 bm = stream_read_u32_le(stream);
    u32 gm = stream_read_u32_le(stream);
    u32 rm = stream_read_u32_le(stream);
    u32 am = stream_read_u32_le(stream);
    if(data->compressionMethod == BMP_COMPRESSION_BITFIELDS) {
      data->blueMask = bm;
      data->greenMask = gm;
      data->redMask = rm;
      data->alphaMask = am;
    }
    data->colorSpace = stream_read_u32_le(stream);
    u32 rx = stream_read_u32_le(stream);
    u32 ry = stream_read_u32_le(stream);
    u32 rz = stream_read_u32_le(stream);
    u32 gx = stream_read_u32_le(stream);
    u32 gy = stream_read_u32_le(stream);
    u32 gz = stream_read_u32_le(stream);
    u32 bx = stream_read_u32_le(stream);
    u32 by = stream_read_u32_le(stream);
    u32 bz = stream_read_u32_le(stream);
    u32 gammaRed   = stream_read_u32_le(stream);
    u32 gammaGreen = stream_read_u32_le(stream);
    u32 gammaBlue  = stream_read_u32_le(stream);
    
    if(data->colorSpace == BMP_COLORSPACE_CAL_RGB) {
      data->colorSpaceTransform = color_transform_create_from_f2p30
        (rx,ry,rz,
         gx,gy,gz,
         bx,by,bz);
      data->redGamma   = (r32)cvt_f16p16_r64(gammaRed);
      data->greenGamma = (r32)cvt_f16p16_r64(gammaGreen);
      data->blueGamma  = (r32)cvt_f16p16_r64(gammaBlue);
    }
    
    // NOTE(bumbread): unsupported color spaces
    if(data->colorSpace == BMP_COLORSPACE_PROFILE_EMBED
       || data->colorSpace == BMP_COLORSPACE_PROFILE_LINKED) {
      stream->error = true;
    }
    
    if(stream->error == true) {return;}
  }
  
  if(dibHeaderVersion >= BMP_DIB_VERSION_V5) {}
  
  data->palette = 0;
  u32 paletteOffset = stream->offset;
  if(paletteOffset > stream->size) {stream->error = true;}
  else {data->palette = stream->start + paletteOffset;}
  stream_offset(stream, data->dataOffset);
}

internal u32 get_sample_from_mask(u32 data, u32 mask) {
  u32 maskBit = 0;
  bool found = false;
  u32 maskStart = 0;
  u32 maskOneAfterEnd = 0;
  loop {
    bool bitValue = (mask >> maskBit)&1;
    if(!found && bitValue) {found = true; maskStart = maskBit;}
    if(found && !bitValue) {maskOneAfterEnd = maskBit; break;}
    
    maskBit += 1;
    if(maskBit == 32) break;
  }
  u32 maskLength = maskOneAfterEnd - maskStart;
  
  u32 bits = (data & mask) >> maskStart;
  u32 sample = (bits * 255u) / (1 << maskLength);
  return(sample);
}

internal t_image bmp_load_data(t_bmp_data* bmp, t_stream data) {
  t_image image;
  image.width = bmp->bitmapWidth;
  image.height = bmp->bitmapHeight;
  image.pixels = (t_colorf*)malloc(bmp->bitmapWidth * bmp->bitmapHeight * sizeof(t_colorf));
  
  // NOTE(bumbread): initializing pixels to black because in the case of RLE compression
  // not every pixel will be overwritten.
  for(u32 index = 0; index < image.height*image.width; index += 1) {
    image.pixels[index] = color_black;
  }
  
  if(bmp->bitsPerPixel == 1) {
    if(stream_can_read_size(&data, bmp->imageSize) == false) {goto error;}
    
    u32 currentBit = 8;
    byte currentByte = stream_read_byte(&data);
    u32 colCounter = 0;
    u32 rowCounter = 0;
    loop {
      currentBit -= 1;
      u32 value = 1&(currentByte >> currentBit);
      // TODO(bumbread): out of bounds checking
      u32 blue = bmp->palette[4*value + 0];
      u32 green = bmp->palette[4*value + 1];
      u32 red = bmp->palette[4*value + 2];
      u32 alpha = 0xff;
      t_colorf pixel = bytes_to_colorf(red, green, blue, alpha);
      image.pixels[rowCounter*image.width + colCounter] = pixel;
      colCounter += 1;
      if(colCounter == image.width) {
        colCounter = 0;
        rowCounter += 1;
        if(rowCounter == image.height) {
          rowCounter = 0;
          goto finish;
        }
        stream_align(&data, 4);
        assert(data.error == false);
        currentBit = 0;
      }
      
      if(currentBit == 0) {
        currentBit = 8;
        currentByte = stream_read_byte(&data);
      }
    }
  }
  else if(bmp->bitsPerPixel == 4) {
    if(bmp->compressionMethod == BMP_COMPRESSION_RLE4) {
      
      u32 colCounter = 0;
      u32 rowCounter = 0;
      loop {
        stream_align(&data, 2);
        byte currentByte = stream_read_byte(&data);
        if(currentByte == 0) { //escape byte
          currentByte = stream_read_byte(&data); // end of line
          if(data.error == true) {goto error;}
          if(currentByte == 0) {
            colCounter = 0;  // TODO(bumbread): does new line make cursor on a new line? (assuming it does)
            rowCounter += 1;
          }
          else if(currentByte == 1) {goto finish;} // read stop code
          else if(currentByte == 2) { // shift position
            i32 shiftX = (i32)stream_read_byte(&data);
            i32 shiftY = (i32)stream_read_byte(&data);
            if(data.error == true) {goto error;}
            if((i32)colCounter > shiftX && (i32)rowCounter > shiftY) {
              colCounter = (u32)((i32)colCounter + shiftX);
              rowCounter = (u32)((i32)colCounter + shiftY);
            }
            else {goto error;}
          }
          else { // uncoded bytes
            if(colCounter == image.width) {goto error;}
            for(u32 count = 0; count < currentByte; count += 1) {
              u32 value = stream_read_byte(&data);
              u32 value_1 = (value >> 4)&0xf;
              u32 value_2 = (value >> 0)&0xf;
              t_colorf colors[2];
              u32 r = bmp->palette[4*value_1 + 0];
              u32 g = bmp->palette[4*value_1 + 1];
              u32 b = bmp->palette[4*value_1 + 2]; // TODO(bumbread): support alpha in bitmaps?
              u32 a = 0xff;
              colors[0] = bytes_to_colorf(r,g,b,a);
              r = bmp->palette[4*value_2 + 0];
              g = bmp->palette[4*value_2 + 1];
              b = bmp->palette[4*value_2 + 2];
              a = 0xff;
              colors[1] = bytes_to_colorf(r,g,b,a);
              for(u32 _=0;_<2;_+=1) {
                image.pixels[colCounter + rowCounter*image.width] = colors[_];
                colCounter += 1;
                if(colCounter == image.width) {
                  break;
                }
              }
            }
            if(data.error) {goto error;}
          }
        }
        else { // current byte != 0, coded linear section
          u32 repeatCount = 2*currentByte;
          u32 value = stream_read_byte(&data);
          u32 value_1 = (value >> 4)&0xf;
          u32 value_2 = (value >> 0)&0xf;
          t_colorf colors[2];
          u32 r = bmp->palette[4*value_1 + 0];
          u32 g = bmp->palette[4*value_1 + 1];
          u32 b = bmp->palette[4*value_1 + 2]; // TODO(bumbread): support alpha in bitmaps?
          u32 a = 0xff;
          colors[0] = bytes_to_colorf(r,g,b,a);
          r = bmp->palette[4*value_2 + 0];
          g = bmp->palette[4*value_2 + 1];
          b = bmp->palette[4*value_2 + 2];
          a = 0xff;
          colors[1] = bytes_to_colorf(r,g,b,a);
          for(u32 count = 0; count < repeatCount; count += 1) {
            image.pixels[colCounter + rowCounter*image.width] = colors[count%2];
            colCounter += 1;
            if(colCounter == image.width) {
              break;
            }
          }
        }
      }
    }
    else {
      if(stream_can_read_size(&data, bmp->imageSize) == false) {goto error;}
      
      u32 rowCounter = 0;
      u32 colCounter = 0;
      byte currentByte = stream_read_byte(&data);
      u32 order = 2;
      loop {
        //stream_align(&data, 2);
        order -= 1;
        u32 value = 0xf&(currentByte>>(4*order));
        // TODO(bumbread): out of bounds check
        u32 b = bmp->palette[4*value + 0];
        u32 g = bmp->palette[4*value + 1];
        u32 r = bmp->palette[4*value + 2];
        u32 a = 0xff;
        t_colorf color = bytes_to_colorf(r,g,b,a);
        image.pixels[colCounter + rowCounter*image.width] = color;
        colCounter += 1;
        if(colCounter == bmp->bitmapWidth) {
          colCounter = 0;
          rowCounter += 1;
          if(rowCounter == bmp->bitmapHeight) {
            rowCounter = 0;
            goto finish;
          }
          stream_align(&data, 4);
          assert(data.error == false);
          order = 0;
        }
        if(order == 0) {
          order = 2;
          currentByte = stream_read_byte(&data);
        }
      }
    }
  }
  else if(bmp->bitsPerPixel == 8) {
    if(bmp->compressionMethod == BMP_COMPRESSION_RLE8) {
      
      u32 colCounter = 0;
      u32 rowCounter = 0;
      stream_align(&data, 2);
      byte currentByte = stream_read_byte(&data);
      if(currentByte == 0) { //escape byte
        currentByte = stream_read_byte(&data); // end of line
        if(data.error == true) {goto error;}
        if(currentByte == 0) {
          colCounter = 0;  // TODO(bumbread): does new line make cursor on a new line? (assuming it does)
          rowCounter += 1;
        }
        else if(currentByte == 1) {goto finish;} // read stop code
        else if(currentByte == 2) { // shift position
          i32 shiftX = (i32)stream_read_byte(&data);
          i32 shiftY = (i32)stream_read_byte(&data);
          if(data.error == true) {goto error;}
          if((i32)colCounter > shiftX && (i32)rowCounter > shiftY) {
            colCounter = (u32)((i32)colCounter + shiftX);
            rowCounter = (u32)((i32)colCounter + shiftY);
          }
          else {goto error;}
        }
        else { // uncoded bytes
          if(colCounter == image.width) {goto error;}
          for(u32 count = 0; count < currentByte; count += 1) {
            u32 value = stream_read_byte(&data);
            u32 b = bmp->palette[4*value + 0];
            u32 g = bmp->palette[4*value + 1];
            u32 r = bmp->palette[4*value + 2];
            u32 a = 0xff;
            t_colorf color = bytes_to_colorf(r,g,b,a);
            image.pixels[colCounter + rowCounter*image.width] = color;
            colCounter += 1;
            if(colCounter == image.width) {
              break;
            }
          }
          if(data.error) {goto error;}
        }
      }
      else { // current byte != 0, coded linear section
        if(colCounter == image.width) {goto error;}
        u32 repeatCount = currentByte;
        u32 value = stream_read_byte(&data);
        u32 b = bmp->palette[4*value + 0];
        u32 g = bmp->palette[4*value + 1];
        u32 r = bmp->palette[4*value + 2];
        u32 a = 0xff;
        t_colorf color = bytes_to_colorf(r,g,b,a);
        for(u32 count = 0; count < repeatCount; count += 1) {
          image.pixels[colCounter + rowCounter*image.width] = color;
          colCounter += 1;
          if(colCounter == image.width) {
            break;
          }
        }
        if(data.error) {goto error;}
        stream_align(&data, 4);
      }
    }
    else {
      if(stream_can_read_size(&data, bmp->imageSize) == false) {goto error;}
      
      u32 colCounter = 0;
      u32 rowCounter = 0;
      loop {
        u32 value = (u32)stream_read_byte(&data);
        u32 b = bmp->palette[4*value + 0];
        u32 g = bmp->palette[4*value + 1];
        u32 r = bmp->palette[4*value + 2];
        u32 a = 0xff;
        t_colorf color = bytes_to_colorf(r,g,b,a);
        image.pixels[rowCounter*image.width + colCounter] = color;
        colCounter += 1;
        if(colCounter == image.width) {
          colCounter = 0;
          rowCounter += 1;
          if(rowCounter == image.height) {
            rowCounter = 0;
            goto finish;
          }
          stream_align(&data, 4);
          assert(data.error == false);
        }
      }
    }
  }
  else if(bmp->bitsPerPixel == 16) {
    if(stream_can_read_size(&data, bmp->imageSize) == false) {goto error;}
    
    u32 colCounter = 0;
    u32 rowCounter = 0;
    loop {
      u32 nextWord = (u32)stream_read_u16_le(&data);
      u32 red = get_sample_from_mask(nextWord << 16, bmp->redMask);
      u32 blue = get_sample_from_mask(nextWord << 16, bmp->blueMask);
      u32 green = get_sample_from_mask(nextWord << 16, bmp->greenMask);
      u32 alpha = 0xff;
      t_colorf color = bytes_to_colorf(red,green,blue,alpha);
      image.pixels[rowCounter*image.width + colCounter] = color;
      colCounter += 1;
      if(colCounter == image.width) {
        colCounter = 0;
        rowCounter += 1;
        if(rowCounter == image.height) {
          rowCounter = 0;
          goto finish;
        }
        assert(data.error == false);
        stream_align(&data, 4);
      }
    }
  }
  else if(bmp->bitsPerPixel == 24) {
    if(stream_can_read_size(&data, bmp->imageSize) == false) {goto error;}
    
    u32 colCounter = 0;
    u32 rowCounter = 0;
    loop {
      u32 blue = stream_read_byte(&data);
      u32 green = stream_read_byte(&data);
      u32 red = stream_read_byte(&data);
      t_colorf color = bytes_to_colorf(red,green,blue,0xffu);
      image.pixels[rowCounter*image.width + colCounter] = color;
      colCounter += 1;
      if(colCounter == image.width) {
        colCounter = 0;
        rowCounter += 1;
        if(rowCounter == image.height) {
          rowCounter = 0;
          goto finish;
        }
        assert(data.error == false);
        stream_align(&data, 4);
      }
    }
  }
  else if(bmp->bitsPerPixel == 32) {
    if(stream_can_read_size(&data, bmp->imageSize) == false) {goto error;}
    
    u32 colCounter = 0;
    u32 rowCounter = 0;
    loop {
      u32 color = stream_read_u32_le(&data);
      u32 red = get_sample_from_mask(color, bmp->redMask);
      u32 green = get_sample_from_mask(color, bmp->greenMask);
      u32 blue = get_sample_from_mask(color, bmp->blueMask);
      u32 alpha = get_sample_from_mask(color, bmp->alphaMask);
      t_colorf pixel = bytes_to_colorf(red, green, blue, alpha);
      image.pixels[rowCounter*image.width + colCounter] = pixel;
      colCounter += 1;
      if(colCounter == image.width) {
        colCounter = 0;
        rowCounter += 1;
        if(rowCounter == image.height) {
          rowCounter = 0;
          goto finish;
        }
        assert(data.error == false);
        stream_align(&data, 4);
      }
    }
  }
  
  finish:
  if(data.error) {goto error;}
  
  // TODO(bumbread): color profiles.
  
  image.success = true;
  return(image);
  error:
  image.success = false;
  return(image);
}

internal void try_parse_bmp(t_file_data* file, t_image* result) {
  platform_profile_state_push("bmp_parse");
  
  if(result->success) {return;}
  debug_variable_unused(file);
  
  t_stream stream = stream_from_file_data(file);
  t_bmp_data bitmapData = {0};
  
  if(stream_read_u16_le(&stream) != BMP_FIRST_WORD) {goto error;}
  u32 fileSize = stream_read_u32_le(&stream);
  stream_read_u32_le(&stream);
  u32 dataOffset = stream_read_u32_le(&stream);
  
  if(stream.error || (fileSize != file->size) 
     || (stream_is_pointer_within(&stream, dataOffset) == false)) {
    goto error;
  }
  
  bitmapData.fileSize = fileSize;
  bitmapData.dataOffset = dataOffset;
  
  bitmap_load_headers(&bitmapData, &stream);
  if(stream.error) {goto error;}
  
  t_stream dataStream;
  dataStream.error = false;
  dataStream.start = (byte*)file->ptr;
  dataStream.size = (u32)file->size;
  stream_offset(&dataStream, bitmapData.dataOffset);
  
  *result = bmp_load_data(&bitmapData, dataStream);
  if(!result->success || (dataStream.error == true)) {goto error;}
  
  result->success = true;
  
  goto end;
  error:
  result->success = false;
  if(result->pixels != 0) {
    result->pixels = 0;
    free(result->pixels);
  }
  result->width = 0;
  result->height = 0;
  
  end:
  platform_profile_state_pop();
  return;
}

#endif //BMP_H
