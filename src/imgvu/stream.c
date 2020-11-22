
struct {
  bool error;
  byte* start;
  u32 offset;
  u32 size;
} typedef t_stream;

internal inline t_stream stream_from_file_data(t_file_data* file) {
  t_stream result;
  result.error = false;
  result.start = file->ptr;
  result.offset = 0;
  result.size = (u32)file->size;
  return(result);
}

internal inline void stream_reset(t_stream* stream) {
  stream->offset = 0;
  stream->error = false;
}

internal inline bool stream_is_pointer_within(t_stream* stream, u32 offset) {
  return(offset < stream->size);
}

internal inline bool stream_can_read_size(t_stream* stream, u32 size) {
  return((stream->size - stream->offset) >= size);
}

internal inline bool stream_offset(t_stream* stream, u32 offset) {
  if(stream->error == false) {
    if(offset < stream->size) {
      stream->start += offset;
      stream->size -= offset;
      stream->offset = 0;
      stream->error = false;
    }
    else {stream->error = true;}
  }
  
  return(stream->error == false);
}

internal inline bool stream_align(t_stream* stream, u32 alignment) {
  if(stream->error == false) {
    u32 alignedOffset = u32_align_forward(stream->offset, alignment);
    if(alignedOffset < stream->size) {
      stream->offset = alignedOffset;
    }
    else {stream->error = true;}
  }
  return(stream->error == false);
}

internal inline void* stream_read(t_stream* stream, u32 size) {
  if(!stream->error) {
    if(stream->offset + size <= stream->size) {
      byte* oldPointer = stream->start + stream->offset;
      stream->offset = stream->offset + size;
      return(oldPointer);
    }
    stream->error = true;
  }
  return(0);
}

internal inline byte stream_read_byte(t_stream* stream) {
  byte* result = stream_read(stream, 1);
  if(!stream->error) {
    return(*result);
  }
  return(0);
}

internal inline i16 stream_read_i16_le(t_stream* stream) {
  byte* b = stream_read(stream, 2);
  if(!stream->error) {
    i16 result = ((i16)b[0]) | (i16)((i16)b[1] << 8);
    return(result);
  }
  return(0);
}

internal inline i16 stream_read_i16_be(t_stream* stream) {
  byte* b = stream_read(stream, 2);
  if(!stream->error) {
    i16 result = (i16)((i16)b[0] << 8) | ((i16)b[1]);
    return(result);
  }
  return(0);
}

internal inline u16 stream_read_u16_le(t_stream* stream) {
  byte* b = stream_read(stream, 2);
  if(!stream->error) {
    u16 result = ((u16)b[0]) | (u16)((u16)b[1] << 8);
    return(result);
  }
  return(0);
}

internal inline u16 stream_read_u16_be(t_stream* stream) {
  byte* b = stream_read(stream, 2);
  if(!stream->error) {
    u16 result = (u16)((u16)b[0] << 8) | ((u16)b[1]);
    return(result);
  }
  return(0);
}

internal inline i32 stream_read_i32_le(t_stream* stream) {
  byte* b = stream_read(stream, 4);
  if(!stream->error) {
    i32 result = ((i32)b[0]) | ((i32)b[1] << 8) | ((i32)b[2] << 16) | ((i32)b[3] << 24);
    return(result);
  }
  return(0);
}

internal inline i32 stream_read_i32_be(t_stream* stream) {
  byte* b = stream_read(stream, 4);
  if(!stream->error) {
    i32 result = ((i32)b[0] << 24) | ((i32)b[1] << 16) | ((i32)b[2] << 8) | ((i32)b[3]);
    return(result);
  }
  return(0);
}

internal inline u32 stream_read_u32_le(t_stream* stream) {
  byte* b = stream_read(stream, 4);
  if(!stream->error) {
    u32 result = ((u32)b[0]) | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24);
    return(result);
  }
  return(0);
}

internal inline u32 stream_read_u32_be(t_stream* stream) {
  byte* b = stream_read(stream, 4);
  if(!stream->error) {
    u32 result = ((u32)b[0] << 24) | ((u32)b[1] << 16) | ((u32)b[2] << 8) | ((u32)b[3]);
    return(result);
  }
  return(0);
}

