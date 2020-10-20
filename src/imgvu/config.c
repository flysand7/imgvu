
#pragma warning(push, 1)
#include<stdio.h>
#pragma warning(pop)

internal void app_load_default_config(struct t_app_config* appConfig, t_string16 filename) {
  // load the default values into appConfig struct and then save as filename
  debug_variable_unused(appConfig);
  debug_variable_unused(filename);
}

internal void app_load_config(struct t_app_config* appConfig, t_string16 filename) {
  debug_variable_unused(appConfig);
  wprintf(filename.ptr);
  t_file_data configData = platform_load_file(filename);
  if(configData.size) {
    
  }
  else {
    app_load_default_config(appConfig, filename);
  }
}
