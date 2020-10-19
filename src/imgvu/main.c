//
// bumbread's imgvu
//

#include"format/bmp.c"
#include"format/pnm.c"

internal t_image app_decode_file(t_image_data data) {
  t_image result = {0};
  result.success = false;
  try_parse_pnm(&data, &result);
  try_parse_bmp(&data, &result);
  debug_variable_unused(data);
  return(result);
}

internal bool app_update(t_app_state* appState, struct t_directory_state_s* dirState, t_button* keyboard, r32 dt) {
  if(!appState->initialized) {
    appState->initialized = true;
    appState->dirState = dirState;
  }
  
  if(keyboard[VKEY_ESCAPE].pressed) return(true);
  debug_variable_unused(dt);
  
  {
    bool left = keyboard[VKEY_LEFT].pressed;
    bool right = keyboard[VKEY_RIGHT].pressed;
    if(left && !right) {
      platform_directory_previous_file(dirState);
    }
    if(right && !left) {
      platform_directory_next_file(dirState);
    }
  }
  
  return(false);
}

internal void app_draw(t_app_state* appState) {
  if(appState->dirState != 0) {
    t_image* currentImage = platform_get_current_image(appState->dirState);
    if(currentImage != 0) {
      platform_draw_image(&appState->imageLocation, currentImage);
    }
  }
}
