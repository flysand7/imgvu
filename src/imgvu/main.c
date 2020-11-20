//
// bumbread's imgvu
//

#include"format/stream.c"
#include"format/color.c"
#include"format/bmp.c"
#include"format/pnm.c"

internal t_image app_decode_file(t_file_data data) {
  t_image result = {0};
  result.success = false;
  try_parse_pnm(&data, &result);
  try_parse_bmp(&data, &result);
  return(result);
}

internal bool app_update(t_app_state* appState, struct t_directory_state_s* dirState, t_app_input* input, r32 dt) {
  if(!appState->initialized) {
    app_load_config(&app_config, platform_get_config_filename());
    platform_chose_graphics_provider(app_config.graphicsProvider);
    platform_initialize_graphics_provider();
    
    appState->initialized = true;
    appState->dirState = dirState;
    
    //appState->imageLocation.angle = PI32/4.0f;
    appState->imageLocation.scale = 3.0f;
  }
  
  //appState->imageLocation.angle += 0.007f;
  debug_variable_unused(dt);
  
  if(input->prevImage && !input->nextImage) {
    platform_directory_previous_file(dirState);
    app_config.backgroundColor += 1;
    if(app_config.backgroundColor >= app_config.colorCycle.len) {
      app_config.backgroundColor = 0;
    }
  }
  if(input->nextImage && !input->prevImage) {
    platform_directory_next_file(dirState);
    app_config.backgroundColor += 1;
    if(app_config.backgroundColor >= app_config.colorCycle.len) {
      app_config.backgroundColor = 0;
    }
  }
  
  return(false);
}

internal void app_draw(t_app_state* appState) {
  if(app_config.colorCycle.ptr) {
    u32 color = (u32)app_config.colorCycle.ptr[app_config.backgroundColor];
    platform_clear_screen(color);
  }
  
  if(appState->dirState != 0) {
    t_image* currentImage = platform_get_current_image(appState->dirState);
    if(currentImage != 0) {
      platform_draw_image(&appState->imageLocation, currentImage);
    }
  }
  
  platform_show();
}
