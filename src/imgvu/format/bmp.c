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
  u32* palette;
  
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

internal void bitmap_load_headers(t_bmp_data* data, t_stream* stream) {
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
  
  data->colorsUsed = (1 << data->bitsPerPixel);
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
  else {data->palette = (u32*)(void*)(stream->start + paletteOffset);}
  stream_offset(stream, data->dataOffset);
}

internal void try_parse_bmp(t_file_data* file, t_image* result) {
  if(result->success) return;
  
  t_stream stream = stream_from_file_data(file);
  t_bmp_data bitmapData = {0};
  
  if(stream_read_u16_le(&stream) == BMP_FIRST_WORD) {
    u32 fileSize  = stream_read_u32_le(&stream);
    stream_read_u32_le(&stream);
    u32 pixels    = stream_read_u32_le(&stream);
    
    if(stream.error || (fileSize != file->size) 
       || (stream_is_pointer_within(&stream, pixels) == false)) {
      goto error;
    }
    
    bitmapData.fileSize = fileSize;
    bitmapData.dataOffset = pixels;
    
    bitmap_load_headers(&bitmapData, &stream);
    if(stream.error) {goto error;}
  }
  
  return;
  
  error:
  result->success = false;
  if(result->pixels) free(result->pixels);
  result->pixels = 0;
  result->width = 0;
  result->height = 0;
}

#endif //BMP_H
