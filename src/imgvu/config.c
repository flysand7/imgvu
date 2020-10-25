
#include<stdio.h>

struct {
  bool error;
  u32 colorCycleCount;
  u32* colors;
  u32 backgroundColor;
} typedef t_app_config;

internal void config_load_default(t_app_config* appConfig) {
  appConfig->error = false;
  appConfig->colorCycleCount = 1;
  appConfig->colors = malloc(appConfig->colorCycleCount * sizeof(u32));
  appConfig->colors[0] = 0xffff00ff;
  appConfig->backgroundColor = 0;
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
  TYPE_ARRAY_INTEGER,
  TYPE_ARRAY_STRING,
  TYPE_ARRAY_FLOAT,
} typedef t_setting_type;

struct {
  t_string name;
  t_setting_type type;
  union {
    i32 value_i;
    r32 value_f;
    t_string value_s;
    struct {
      u32 value_len;
      union {
        u32* value_ai;
        t_string* value_as;
        r32* value_af;
      };
    };
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
    
    if(string_compare(char_count("color_cycle"), currentSetting->name)) {
      if(currentSetting->type == TYPE_ARRAY_INTEGER) {
        appConfig->colorCycleCount = currentSetting->value_len;
        if(appConfig->colors) free(appConfig->colors);
        appConfig->colors = malloc(appConfig->colorCycleCount * sizeof(u32));
        for(u32 valueIndex = 0; valueIndex < currentSetting->value_len; valueIndex += 1) {
          appConfig->colors[valueIndex] = currentSetting->value_ai[valueIndex];
        }
      }
      else {
        // TODO(bumbread): logging
      }
    }
  }
}

internal void config_free_settings(t_setting_list* settings) {
  for(u32 settingIndex = 0; settingIndex < settings->count; settingIndex += 1) {
    t_setting* setting = settings->v + settingIndex;
    if(setting->type == TYPE_ARRAY_INTEGER) {
      if(setting->value_ai) free(setting->value_ai);
    }
    else if(setting->type == TYPE_ARRAY_FLOAT) {
      if(setting->value_af) free(setting->value_af);
    }
    else if(setting->type == TYPE_ARRAY_STRING) {
      if(setting->value_as) free(setting->value_as);
    }
  }
}

internal void app_load_config(t_app_config* appConfig, t_string16 filename) {
  debug_variable_unused(appConfig);
  
  config_load_default(appConfig);
  // TODO(bumbread): this code is for testing.
  // remove this later.
  
  t_setting_list settings = {0};
  
  t_setting colorCycleSetting = {0};
  colorCycleSetting.name = char_copy("color_cycle");
  colorCycleSetting.type = TYPE_ARRAY_INTEGER;
  colorCycleSetting.value_len = 2;
  colorCycleSetting.value_ai = malloc(2 * sizeof(u32));
  colorCycleSetting.value_ai[0] = 0xff222222;
  colorCycleSetting.value_ai[1] = 0xffeeeeee;
  settings_add(&settings, colorCycleSetting);
  
  config_load_settings(appConfig, &settings);
  config_free_settings(&settings);
  
  t_file_data configData = platform_load_file(filename);
  if(configData.ptr) {
    assert(0);
  }
  else {
    // NOTE(bumbread): the file wasn't found. write a new one with the default config.
    app_write_config_to_file(appConfig, filename);
  }
}
