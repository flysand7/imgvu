
  struct {
  union {
    struct {
      r32 r;
      r32 g;
      r32 b;
    };
    struct {
      v3 rgb;
    };
  };
  r32 a;
} typedef t_colorf;

static t_colorf color_black = {{{0.0f, 0.0f, 0.0f}}, 0.0f};
static t_colorf color_white = {{{1.0f, 1.0f, 1.0f}}, 1.0f};

internal t_colorf bytes_to_colorf(u32 red, u32 green, u32 blue, u32 alpha) {
  assert((red < 256) && (blue < 256) && (green < 256) && (alpha < 256));
  t_colorf result;
  result.r = (r32)red / 255.0f;
  result.g = (r32)green / 255.0f;
  result.b = (r32)blue / 255.0f;
  result.a = (r32)alpha / 255.0f;
  return(result);
}

internal u32 colorf_to_rgba(t_colorf color) {
  u32 red = (u32)round32(color.r * 255.0f);
  u32 blue = (u32)round32(color.b * 255.0f);
  u32 green = (u32)round32(color.g * 255.0f);
  u32 alpha = (u32)round32(color.a * 255.0f);
  u32 result = (red) | (green << 8) | (blue << 16) | (alpha << 24);
  return(result);
}

internal u32 colorf_to_bgra(t_colorf color) {
  u32 red = (u32)round32(color.r * 255.0f);
  u32 blue = (u32)round32(color.b * 255.0f);
  u32 green = (u32)round32(color.g * 255.0f);
  u32 alpha = (u32)round32(color.a * 255.0f);
  u32 result = (blue) | (green << 8) | (red << 16) | (alpha << 24);
  return(result);
}

internal inline r64 cvt_f16p16_r64(u32 fp) {
  r64 result = (r64)fp / (r64)(1 << 16);
  return(result);
}

internal inline r64 cvt_f2p30_r64(u32 fp) {
  r64 result = (r64)fp / (r64)(1 << 30);
  return(result);
}

internal inline m3 color_transform_create_rgbtoxyz(void) {
  m3 toXYZ;
  toXYZ.r = (v3) {{ 0.4124564f, 0.2126729f, 0.0193339f }};
  toXYZ.g = (v3) {{ 0.3575761f, 0.7151522f, 0.1191920f }};
  toXYZ.b = (v3) {{ 0.1804375f, 0.0721750f, 0.9503041f }};
  return(toXYZ);
}

internal inline m3 color_transform_create_from_f2p30(u32 rx, u32 ry, u32 rz,
                                                     u32 gx, u32 gy, u32 gz,
                                                     u32 bx, u32 by, u32 bz) {
  m3 result;
  result.r.x = (r32)cvt_f2p30_r64(rx);
  result.r.y = (r32)cvt_f2p30_r64(ry);
  result.r.z = (r32)cvt_f2p30_r64(rz);
  result.g.x = (r32)cvt_f2p30_r64(gx);
  result.g.y = (r32)cvt_f2p30_r64(gy);
  result.g.z = (r32)cvt_f2p30_r64(gz);
  result.b.x = (r32)cvt_f2p30_r64(bx);
  result.b.y = (r32)cvt_f2p30_r64(by);
  result.b.z = (r32)cvt_f2p30_r64(bz);
  return(result);
}
