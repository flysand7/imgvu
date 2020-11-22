
struct {
  bool prevImage;
  bool nextImage;
} typedef t_app_input;

// NOTE(bumbread): The services platform layer provides to the app.

struct {
  t_string16 filename;
  u64 size;
  void* ptr;
} typedef t_file_data;

struct {
  bool success;
  u32 width;
  u32 height;
  t_colorf* pixels;
} typedef t_image;

struct {
  v2 position;
  r32 scale;
  r32 angle; // in radians!
  bool flippedX;
  bool flippedY;
  // NOTE(bumbread): transformation order
  // flip & scale -> rotate -> translate
} typedef t_location;

// NOTE(bumbread): file management subsystem
struct t_directory_state_s;
internal void platform_directory_set(struct t_directory_state_s* state, t_string16 path);
internal void platform_directory_next_file(struct t_directory_state_s* state);
internal void platform_directory_previous_file(struct t_directory_state_s* state);
internal t_file_data platform_load_file(t_string16 fullFilename);
internal bool platform_write_file(t_file_data file);
internal t_string16 platform_get_config_filename(void);

// NOTE(bumbread): graphics subsystem
internal t_image* platform_get_current_image(struct t_directory_state_s* dirState);
internal void platform_chose_graphics_provider(t_string provider);
internal void platform_initialize_graphics_provider(void);
internal void platform_clear_screen(u32 color);
internal void platform_draw_image(t_location* loc, t_image* image);
internal void platform_show(void);

// NOTE(bumbread): timing subsystem
struct t_clock;
internal struct t_clock platform_clock(void);
internal r64 platform_clock_diff(struct t_clock end, struct t_clock start);
internal void platform_profile_state_push(char const *name);
internal void platform_profile_state_pop(void);

struct {
  bool initialized;
  struct t_directory_state_s* dirState;
  t_location imageLocation;
} typedef t_app_state;

// NOTE(bumbread): The services the app provides to the platform layer.

internal bool app_update(t_app_state* appState, struct t_directory_state_s* state, t_app_input* input, r32 dt);
internal void app_draw(t_app_state* state);
internal t_image app_decode_file(t_file_data data);
