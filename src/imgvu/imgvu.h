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
#include"input.h"
#include"platform.h"

internal bool app_update(struct t_directory_state_s* state, t_button* keyboard, r32 dt);

#include"imgvu/main.c"

#endif // IMGVU_H
