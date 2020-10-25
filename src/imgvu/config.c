
struct {
  bool error;
  u32 backgroundColor;
} typedef t_app_config;

internal void config_load_default(t_app_config* appConfig) {
  appConfig->error = false;
  appConfig->backgroundColor = 0xffff00ff; //test
}

internal void app_write_config_to_file(t_app_config* appConfig, t_string16 filename) {
  t_file_data output = {0};
  output.filename = filename;
  platform_write_file(output);
  debug_variable_unused(appConfig);
}

enum {
  TYPE_INTEGER,
  TYPE_STRING,
  TYPE_FLOAT,
} typedef t_setting_type;

struct {
  t_string name;
  t_setting_type type;
  union {
    i32 value_i;
    r32 value_f;
    t_string value_s;
  };
} typedef t_setting;

struct {
  t_setting* v;
  u32 count;
  u32 alloc;
} typedef t_setting_list;

internal void settings_add(t_setting_list* list, t_setting setting) {
  if(list->count + 1 > list->alloc) {
    list->alloc *= 2;
    if(list->alloc == 0) list->alloc = 1;
    list->v = realloc(list->v, list->alloc * sizeof(t_setting));
  }
  list->v[list->count] = setting;
  list->count += 1;
}

internal void config_load_settings(t_app_config* appConfig, t_setting_list* settings) {
  for(u32 i = 0; i < settings->count; i += 1) {
    // TODO(bumbread): make those static or smth
    t_setting* currentSetting = settings->v + i;
    if(string_compare(char_count("settings"), settings->v[i].name)) {
      if(currentSetting->type != TYPE_INTEGER) goto error;
      appConfig->backgroundColor = (u32)currentSetting->value_i;
    }
  }
  
  return;
  error: {
    appConfig->error = true;
    return;
  }
}

internal void app_load_config(t_app_config* appConfig, t_string16 filename) {
  debug_variable_unused(appConfig);
  
  config_load_default(appConfig);
  // TODO(bumbread): this code is for testing.
  // remove this later.
  
  t_setting_list settings = {0};
  
  t_setting bgColorSetting = {0};
  bgColorSetting.name = char_copy("settings");
  bgColorSetting.type = TYPE_INTEGER;
  bgColorSetting.value_i = (i32)0xff222222;
  settings_add(&settings, bgColorSetting);
  
  config_load_settings(appConfig, &settings);
  
  t_file_data configData = platform_load_file(filename);
  if(configData.ptr) {
    assert(0);
  }
  else {
    // NOTE(bumbread): the file wasn't found. write a new one with the default config.
    app_write_config_to_file(appConfig, filename);
  }
}
