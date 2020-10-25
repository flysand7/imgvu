
struct {
  bool error;
  u32 backgroundColor;
} typedef t_app_config;

internal void app_write_config_to_file(t_app_config* appConfig, t_string16 filename) {
  t_file_data output = {0};
  output.filename = filename;
  platform_write_file(output);
  debug_variable_unused(appConfig);
}

internal void config_load_default(t_app_config* appConfig) {
  appConfig->error = false;
  appConfig->backgroundColor = 0xffff00ff; //test
}

internal void app_load_config(t_app_config* appConfig, t_string16 filename) {
  debug_variable_unused(appConfig);
  
  config_load_default(appConfig);
  t_file_data configData = platform_load_file(filename);
  if(configData.ptr) {
    
    assert(0);
    
  }
  else {
    // NOTE(bumbread): the file wasn't found. write a new one with the default config.
    app_write_config_to_file(appConfig, filename);
  }
}
