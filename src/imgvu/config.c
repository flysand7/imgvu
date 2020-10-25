
#include<stdio.h>

struct {
  u32 len;
  u32* ptr;
} typedef t_array_i;

struct {
  u32 len;
  r32* ptr;
} typedef t_array_f;

struct {
  u32 len;
  t_string* ptr;
} typedef t_array_s;

struct {
  bool error;
  t_array_i colorCycle;
  u32 backgroundColor;
} typedef t_app_config;

internal void config_load_default(t_app_config* appConfig) {
  appConfig->error = false;
  appConfig->colorCycle.len = 1;
  appConfig->colorCycle.ptr = malloc(appConfig->colorCycle.len * sizeof(u32));
  appConfig->colorCycle.ptr[0] = 0xffff00ff;
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
    t_array_i value_ai;
    t_array_f value_af;
    t_array_s value_as;
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

struct {
  t_setting_type type;
  t_string name;
  union {
    i32* value_i;
    r32* value_f;
    t_string* value_s;
    t_array_i* value_ai;
    t_array_f* value_af;
    t_array_s* value_as;
  };
} typedef t_setting_link;

struct {
  t_setting_link* v;
  u32 count;
  u32 alloc;
} typedef t_link_list;

internal void link_add(t_link_list* list, t_setting_link link) {
  if(list->count + 1 > list->alloc) {
    list->alloc *= 2;
    if(list->alloc == 0) list->alloc = 1;
    list->v = realloc(list->v, list->alloc * sizeof(t_setting_link));
  }
  list->v[list->count] = link;
  list->count += 1;
}

#if 0
internal void link_create_i(t_link_list* list, t_string name, i32* value) {
  t_setting_link result = {0};
  result.type = TYPE_INTEGER;
  result.name = name;
  result.value_i = value;
  link_add(list, result);
}

internal void link_create_f(t_link_list* list, t_string name, r32* value) {
  t_setting_link result = {0};
  result.type = TYPE_FLOAT;
  result.name = name;
  result.value_f = value;
  link_add(list, result);
}

internal void link_create_s(t_link_list* list, t_string name, t_string* value) {
  t_setting_link result = {0};
  result.type = TYPE_STRING;
  result.name = name;
  result.value_s = value;
  link_add(list, result);
}
#endif

internal void link_create_ai(t_link_list* list, t_string name, t_array_i* value) {
  t_setting_link result = {0};
  result.type = TYPE_ARRAY_INTEGER;
  result.name = name;
  result.value_ai = value;
  link_add(list, result);
}

#if 0
internal void link_create_af(t_link_list* list, t_string name, t_array_f* value) {
  t_setting_link result = {0};
  result.type = TYPE_ARRAY_FLOAT;
  result.name = name;
  result.value_af = value;
  link_add(list, result);
}

internal void link_create_as(t_link_list* list, t_string name, t_array_s* value) {
  t_setting_link result = {0};
  result.type = TYPE_ARRAY_STRING;
  result.name = name;
  result.value_as = value;
  link_add(list, result);
}
#endif

internal void config_load_settings(t_setting_list* settings, t_link_list* links) {
  for(u32 settingIndex = 0; settingIndex < settings->count; settingIndex += 1) {
    // TODO(bumbread): make those static or smth
    t_setting* currentSetting = settings->v + settingIndex;
    for(u32 linkIndex = 0; linkIndex < links->count; linkIndex += 1) {
      t_setting_link* link = links->v + linkIndex;
      
      if(string_compare(link->name, currentSetting->name)) {
        if(link->type == currentSetting->type) {
          switch(link->type) {
            case(TYPE_INTEGER): {
              *link->value_i = currentSetting->value_i;
            } break;
            case(TYPE_FLOAT): {
              *link->value_f = currentSetting->value_f;
            } break;
            case(TYPE_STRING): {
              *link->value_s = string_copy_mem(currentSetting->value_s);
            } break;
            case(TYPE_ARRAY_INTEGER): {
              link->value_ai->len = currentSetting->value_ai.len;
              if(link->value_ai->ptr) free(link->value_ai->ptr);
              link->value_ai->ptr = malloc(link->value_ai->len * sizeof(u32));
              for(u32 valueIndex = 0; valueIndex < currentSetting->value_ai.len; valueIndex += 1) {
                link->value_ai->ptr[valueIndex] = currentSetting->value_ai.ptr[valueIndex];
              }
            } break;
            case(TYPE_ARRAY_FLOAT): {
              link->value_af->len = currentSetting->value_af.len;
              if(link->value_af->ptr) free(link->value_af->ptr);
              link->value_af->ptr = malloc(link->value_af->len * sizeof(r32));
              for(u32 valueIndex = 0; valueIndex < currentSetting->value_af.len; valueIndex += 1) {
                link->value_af->ptr[valueIndex] = currentSetting->value_af.ptr[valueIndex];
              }
            } break;
            case(TYPE_ARRAY_STRING): {
              link->value_as->len = currentSetting->value_as.len;
              if(link->value_as->ptr) free(link->value_as->ptr);
              link->value_as->ptr = malloc(link->value_as->len * sizeof(u32));
              for(u32 valueIndex = 0; valueIndex < currentSetting->value_as.len; valueIndex += 1) {
                link->value_as->ptr[valueIndex] = string_copy_mem(currentSetting->value_as.ptr[valueIndex]);
              }
            } break;
          }
        }
        else {
          // TODO(bumbread): logging
        }
      }
      
    }
    
  }
}

internal void config_free_settings(t_setting_list* settings) {
  for(u32 settingIndex = 0; settingIndex < settings->count; settingIndex += 1) {
    t_setting* setting = settings->v + settingIndex;
    if(setting->type == TYPE_ARRAY_INTEGER) {
      if(setting->value_ai.ptr) free(setting->value_ai.ptr);
    }
    else if(setting->type == TYPE_STRING) {
      if(setting->value_s.ptr) free(setting->value_s.ptr);
    }
    else if(setting->type == TYPE_ARRAY_FLOAT) {
      if(setting->value_af.ptr) free(setting->value_af.ptr);
    }
    else if(setting->type == TYPE_ARRAY_STRING) {
      if(setting->value_as.ptr) {
        for(u32 stringIndex = 0; stringIndex < setting->value_as.len; stringIndex += 1) {
          free(setting->value_as.ptr[stringIndex].ptr);
        }
        free(setting->value_as.ptr);
      }
    }
  }
  free(settings->v);
}

internal void app_load_config(t_app_config* appConfig, t_string16 filename) {
  debug_variable_unused(appConfig);
  
  config_load_default(appConfig);
  // TODO(bumbread): this code is for testing.
  // remove this later.
  
  t_setting_list settings = {0};
  t_link_list links = {0};
  
  t_setting colorCycleSetting = {0};
  colorCycleSetting.name = char_copy("color_cycle");
  colorCycleSetting.type = TYPE_ARRAY_INTEGER;
  colorCycleSetting.value_ai.len = 2;
  colorCycleSetting.value_ai.ptr = malloc(2 * sizeof(u32));
  colorCycleSetting.value_ai.ptr[0] = 0xff222222;
  colorCycleSetting.value_ai.ptr[1] = 0xffeeeeee;
  settings_add(&settings, colorCycleSetting);
  
  link_create_ai(&links, char_count("color_cycle"), &appConfig->colorCycle);
  
  config_load_settings(&settings, &links);
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
