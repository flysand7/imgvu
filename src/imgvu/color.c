
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



