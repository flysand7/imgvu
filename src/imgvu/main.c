//
// bumbread's imgvu
//
// el manifesto
//
// This is a fast powerful image viewer that allows you to
// use your mouse as little as possible, while getting the maximum
// out of viewing images.
// 
// Goals: 
// - Robust
// - Instanteneous (as fas as user's perception is concerned)
// - Easy to port.
// - Simple control
// - Supporting lots of image formats.
// NOTE(bumbread): the following formats are planned to be supported:
// ppm bmp tga gif jpeg png jpeg2000 openexr heif avif flif bpg
// NOTE(bumbread): the following formats are already supported:
// <none>
// 
// 


internal bool app_update(t_button* keyboard, r32 dt) {
  if(keyboard[VKEY_ESCAPE].pressed) return(true);
  debug_variable_unused(dt);
  
  return(false);
}
