
struct { u32 len; char16* ptr; } typedef t_string16;
struct { u32 len; char* ptr; } typedef t_string;

internal t_string16 char16_to_string16(char16* chars) {
  char16* charPointer = chars;
  u32 length = 0;
  loop {
    if(*charPointer == 0) break; 
    length += 1;
    charPointer += 1;
  }
  t_string16 result;
  result.len = length;
  result.ptr = chars;
  return(result);
}

internal bool has_substring_char16(t_string16 string, char16* chars) {
  for(u32 charIndex = 0; charIndex < string.len; charIndex += 1) {
    if(chars[charIndex] == 0) return(true);
    if(string.ptr[charIndex] != chars[charIndex]) return(false);
  }
  return(false);
}

internal bool has_substring_string16(t_string16 string, t_string16 sub) {
  if(string.len < sub.len) return(false);
  for(u32 charIndex = 0; charIndex < string.len; charIndex += 1) {
    if(string.ptr[charIndex] != sub.ptr[charIndex]) return(false);
  }
  return(false);
}

internal bool string16_compare_char16(t_string16 string, char16* chars) {
  for(u32 charIndex = 0; charIndex < string.len; charIndex += 1) {
    if(chars[charIndex] == 0) return(true);
    if(string.ptr[charIndex] != chats[charIndex]) return(false);
  }
  return(true);
}

internal bool string16_compare_string16(t_string16 string1, t_string16 string2) {
  if(string1.len != string2.len) return(false);
  for(u32 charIndex = 0; charIndex < string1.len; charIndex += 1) {
    if(string1.ptr[charIndex] != string2.ptr[charIndex]) return(false);
  }
  return(true);
}