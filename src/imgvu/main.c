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

internal load_texture(t_file_entry* fileEntry) {
  if(!fileEntry->hasTexture) {
    
  }
}

internal bool update_app(t_button* keyboard, 
                         t_program_state* programState, 
                         t_directory_state* directoryState, 
                         r32 dt) {
  if(keyboard[VKEY_ESCAPE].pressed) return(true);
  
  if(!programState->shouldRender) {
    request_curr_image(directoryState);
    t_file_entry* file = directoryState->files + directoryState->fileIndex;
    load_texture(file);
  }
  
  return(false);
}
