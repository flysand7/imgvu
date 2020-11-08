#define PI32 3.1415926535f

#include<math.h>

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

