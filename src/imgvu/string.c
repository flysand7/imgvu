
struct { u32 len; char16* ptr; } typedef t_string16;
struct { u32 len; char* ptr; } typedef t_string;

#define static_make_string16(nm, str) t_string16 nm; nm.ptr=(str); nm.len=sizeof(str)/sizeof(char16) - 1
#define static_make_string(nm, str) t_string nm; nm.ptr=(str); nm.len=sizeof(str)/sizeof(char) - 1

// string16 functions

internal t_string16 char16_copy(char16* chars) {
  t_string16 result = {0};
  for(u32 i = 0; chars[i] != 0; i += 1) result.len += 1;
  result.ptr = chars;
  return(result);
}

internal t_string16 char16_copy_mem(char16* string) {
  assert(string != 0);
  t_string16 result = {0};
  for(u32 i = 0; string[i] != 0; i += 1) result.len += 1;
  result.ptr = malloc((result.len + 1) * sizeof(char16));
  for(u32 i = 0; i < result.len; i += 1) result.ptr[i] = string[i];
  result.ptr[result.len] = 0;
  return(result);
}

internal t_string16 char16_count(char16* string) {
  t_string16 result;
  result.ptr = string;
  result.len = 0;
  for(u32 i = 0; string[i] != 0; i += 1) result.len += 1;
  return(result);
}

internal t_string16 string16_copy_mem(t_string16 string) {
  t_string16 result;
  result.len = string.len;
  result.ptr = malloc((result.len+1)* sizeof(char16));
  for(u32 i = 0; i < result.len; i += 1) result.ptr[i] = string.ptr[i];
  result.ptr[result.len] = 0;
  return(result);
}

internal t_string16 string16_concatenate_mem(t_string16 first, t_string16 second) {
  t_string16 result;
  result.len = first.len + second.len;
  result.ptr = malloc((result.len + 1) * sizeof(char16));
  u32 resultIndex = 0;
  for(u32 charIndex = 0; charIndex < first.len; charIndex += 1) {
    result.ptr[resultIndex] = first.ptr[charIndex];
    resultIndex += 1;
  }
  for(u32 charIndex = 0; charIndex < second.len; charIndex += 1) {
    result.ptr[resultIndex] = second.ptr[charIndex];
    resultIndex += 1;
  }
  result.ptr[result.len] = 0;
  return(result);
}

internal bool string16_begins_with(t_string16 string, t_string16 sub) {
  if(string.len < sub.len) return(false);
  for(u32 charIndex = 0; charIndex < string.len; charIndex += 1) {
    if(string.ptr[charIndex] != sub.ptr[charIndex]) return(false);
  }
  return(false);
}

internal bool string16_compare(t_string16 string1, t_string16 string2) {
  if(string1.len != string2.len) return(false);
  for(u32 charIndex = 0; charIndex < string1.len; charIndex += 1) {
    if(string1.ptr[charIndex] != string2.ptr[charIndex]) return(false);
  }
  return(true);
}

// string functions
internal t_string char_copy(char* chars) {
  t_string result = {0};
  for(u32 i = 0; chars[i] != 0; i += 1) result.len += 1;
  result.ptr = chars;
  return(result);
}

internal t_string char_copy_mem(char* string) {
  assert(string != 0);
  t_string result = {0};
  for(u32 i = 0; string[i] != 0; i += 1) result.len += 1;
  result.ptr = malloc((result.len + 1) * sizeof(char));
  for(u32 i = 0; i < result.len; i += 1) result.ptr[i] = string[i];
  result.ptr[result.len] = 0;
  return(result);
}

internal t_string char_count(char* string) {
  t_string result;
  result.ptr = string;
  result.len = 0;
  for(u32 i = 0; string[i] != 0; i += 1) result.len += 1;
  return(result);
}


internal t_string string_copy_mem(t_string string) {
  t_string result;
  result.len = string.len;
  result.ptr = malloc((result.len+1)* sizeof(char));
  for(u32 i = 0; i < result.len; i += 1) result.ptr[i] = string.ptr[i];
  result.ptr[result.len] = 0;
  return(result);
}

#if 0
internal t_string string_concatenate_mem(t_string first, t_string second) {
  t_string result;
  result.len = first.len + second.len;
  result.ptr = malloc((result.len + 1) * sizeof(char));
  u32 resultIndex = 0;
  for(u32 charIndex = 0; charIndex < first.len; charIndex += 1) {
    result.ptr[resultIndex] = first.ptr[charIndex];
    resultIndex += 1;
  }
  for(u32 charIndex = 0; charIndex < second.len; charIndex += 1) {
    result.ptr[resultIndex] = second.ptr[charIndex];
    resultIndex += 1;
  }
  result.ptr[result.len] = 0;
  return(result);
}

internal bool string_begins_with(t_string string, t_string sub) {
  if(string.len < sub.len) return(false);
  for(u32 charIndex = 0; charIndex < string.len; charIndex += 1) {
    if(string.ptr[charIndex] != sub.ptr[charIndex]) return(false);
  }
  return(false);
}
#endif

internal bool string_compare(t_string string1, t_string string2) {
  if(string1.len != string2.len) return(false);
  for(u32 charIndex = 0; charIndex < string1.len; charIndex += 1) {
    if(string1.ptr[charIndex] != string2.ptr[charIndex]) return(false);
  }
  return(true);
}
