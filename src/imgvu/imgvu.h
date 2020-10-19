/* date = October 5th 2020 9:15 pm */

#ifndef IMGVU_H
#define IMGVU_H

#include<stdint.h>
int8_t typedef i8;
uint8_t typedef u8;
int16_t typedef i16;
uint16_t typedef u16;
int32_t typedef i32;
uint32_t typedef u32;
int64_t typedef i64;
uint64_t typedef u64;

enum {false, true} typedef bool;

float typedef r32;
double typedef r64;

u8 typedef byte;
u16 typedef word;
wchar_t typedef char16;

#define internal static
#define global static
#define persist static
#define loop for(;;)

#ifdef MODE_DEBUG
#define assert(x) do { if(!(x)) { __debugbreak(); } } while(0)
#define debug_variable_unused(lvalue_) do{{void* tmp=&(lvalue_);tmp = 0;}}while(0)
#else
#define assert(x) 
#define debug_variable_unused(lvalue_) 
#endif

#define array_length(a) (sizeof(a) / sizeof((a)[0]))

#include"string.c"

struct {
  bool prevImage;
  bool nextImage;
} typedef t_app_input;

// NOTE(bumbread): The services platform layer provides to the app.

struct {
  t_string16 filename;
  u64 size;
  void* ptr;
} typedef t_image_data;

struct {
  bool success;
  u32 width;
  u32 height;
  u32* pixels;
} typedef t_image;

struct {
  r32 posX;
  r32 posY;
  r32 scale;
  r32 angle; // in radians!
  bool flippedX;
  bool flippedY;
  // NOTE(bumbread): transformation order
  // flip -> rotate -> scale -> translate
} typedef t_location;

struct t_directory_state_s;
internal void platform_directory_set(struct t_directory_state_s* state, t_string16 path);
internal void platform_directory_next_file(struct t_directory_state_s* state);
internal void platform_directory_previous_file(struct t_directory_state_s* state);

internal t_image* platform_get_current_image(struct t_directory_state_s* dirState);
internal void platform_draw_image(t_location* loc, t_image* image);

// NOTE(bumbread): The services the app provides to the platform layer.

struct {
  bool initialized;
  struct t_directory_state_s* dirState;
  t_location imageLocation;
} typedef t_app_state;

internal bool app_update(t_app_state* appState, struct t_directory_state_s* state, t_app_input* input, r32 dt);
internal void app_draw(t_app_state* state);

// NOTE(bumbread): the application interface realisation

#include"main.c"

#endif // IMGVU_H
