#define PI32 3.1415926535f

#include<math.h>

internal inline i32 floor32(r32 a) {
  return((i32)(floorf(a)));
}

internal inline i32 ceil32(r32 a) {
  return((i32)(ceilf(a)));
}

internal inline r32 sin32(r32 angle) {
  return(sinf(angle));
}

internal inline r32 cos32(r32 angle) {
  return(cosf(angle));
}

internal inline r32 deg_to_rad(r32 deg) {
  return(deg / 180.0f * PI32);
}

internal inline r32 rad_to_deg(r32 rad) {
  return(rad/PI32 * 180.0f);
}

internal inline bool is_power_of_two(u32 i) {
  return((i & (i-1)) == 0);
}

internal inline u32 u32_align_forward(u32 value, u32 alignment) {
  assert(is_power_of_two(alignment));
  u32 alignedValue = value;
  if(alignedValue % alignment != 0) {
    // TODO(bumbread): there was a more neat way to write this.
    alignedValue = value + ((alignment - (value % alignment)) & (alignment - 1));
  }
  return(value);
}

struct {
  r32 x;
  r32 y;
} typedef v2;

internal inline v2 v2_rotate(v2 vector, r32 angle) {
  v2 result;
  result.x = vector.x*cos32(angle) - vector.y*sin32(angle);
  result.y = vector.x*sin32(angle) + vector.y*cos32(angle);
  return(result);
}

internal inline v2 v2_add(v2 a, v2 b) {
  v2 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return(result);
}

internal inline v2 v2_sub(v2 a, v2 b) {
  v2 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return(result);
}

internal inline v2 v2_mul(v2 a, r32 b) {
  v2 result;
  result.x = a.x * b;
  result.y = a.y * b;
  return(result);
}

internal inline v2 v2_div(v2 a, r32 b) {
  assert(b != 0.0f);
  v2 result;
  result.x = a.x / b;
  result.y = a.y / b;
  return(result);
}

union {
  struct {r32 x,y,z;};
  struct {r32 r,g,b;};
  r32 v[3];
} typedef v3;

union {
  struct {v3 r,g,b;};
  struct {v3 x,y,z;};
  v3 v[3];
} typedef m3;

internal m3 inverse(m3 m) {
  m3 result;
  r32 rx =   m.g.y*m.b.z - m.b.y*m.g.z;
  r32 ry = -(m.r.y*m.b.z - m.b.y*m.r.z);
  r32 rz =   m.r.y*m.g.z - m.g.y*m.r.z;
  
  r32 gx = -(m.b.x*m.g.z - m.g.x*m.b.z);
  r32 gy =   m.r.x*m.b.z - m.b.x*m.r.z;
  r32 gz = -(m.r.x*m.g.z - m.g.x*m.r.z);
  
  r32 bx =   m.g.x*m.b.y - m.g.y*m.b.x;
  r32 by = -(m.r.x*m.b.y - m.r.y*m.b.x);
  r32 bz =   m.r.x*m.g.y - m.r.y*m.b.x;
  
  r32 d = 1.0f / (m.r.x*rx + m.g.x*ry + m.b.x*rz);
  
  result.r = (v3) {{ rx*d, ry*d, rz*d }};
  result.g = (v3) {{ gx*d, gy*d, gz*d }};
  result.b = (v3) {{ bx*d, by*d, bz*d }};
  
  return(result);
}

internal inline v3 transform(m3 m, v3 p) {
  v3 result;
  result.x = m.r.x*p.x + m.g.x*p.y + m.b.x*p.z;
  result.y = m.r.y*p.x + m.g.y*p.y + m.b.y*p.z;
  result.z = m.r.z*p.x + m.g.z*p.y + m.b.z*p.z;
  return(result);
}
