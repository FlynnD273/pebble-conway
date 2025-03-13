#include <pebble.h>

#define SETTINGS_KEY 1

typedef struct ClaySettings {
  uint8_t fps;
  uint8_t cell_size;
  bool wrap_edges;
  GColor fg_color;
  GColor bg_color;
} ClaySettings;

static ClaySettings settings;

static Window *s_main_window;
static Layer *s_layer;
static int width;
static int height;
static int frame = 0;

static int rows;
static int cols;

#define MAX(a, b) a > b ? a : b
#define MIN(a, b) a < b ? a : b

static uint8_t *cells;
static uint8_t *cells2;

static uint8_t get_byte(size_t i) {
  if (frame % 2) {
    return cells[i];
  } else {
    return cells2[i];
  }
}

static uint8_t get_cell(int x, int y) {
  if (settings.wrap_edges || (x >= 0 && x < cols && y >= 0 && y < rows)) {
    size_t idx = ((y + rows) % rows) * cols + ((x + cols) % cols);
    size_t actual_idx = idx / 8;
    size_t shift = 7 - (idx % 8);
    if (frame % 2) {
      return (cells[actual_idx] >> shift) & 1;
    }
    return (cells2[actual_idx] >> shift) & 1;
  }
  return 0;
}

static void set_byte(size_t i, uint8_t val) {
  if (frame % 2) {
    cells2[i] = val;
  } else {
    cells[i] = val;
  }
}

static void frame_redraw(Layer *layer, GContext *ctx) {
  size_t i = 0;
  uint8_t val = 0;
  uint8_t shift = 0;
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      if (shift == 0) {
        val = get_byte(i++);
        shift = 8;
      }
      graphics_context_set_fill_color(
          ctx, ((val >> --shift) & 1) ? settings.fg_color : settings.bg_color);
      graphics_fill_rect(ctx,
                         GRect(x * settings.cell_size, y * settings.cell_size,
                               settings.cell_size, settings.cell_size),
                         0, 0);
    }
  }
}

static void next_gen() {
  size_t i = 0;
  uint8_t val = 0;
  uint8_t shift = 8;
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      uint8_t n_count = 0;
      for (int16_t dx = -1; dx <= 1; dx++) {
        for (int16_t dy = -1; dy <= 1; dy++) {
          if (dx == 0 && dy == 0) {
            continue;
          }
          if (get_cell(x + dx, y + dy)) {
            n_count++;
          }
        }
      }
      uint8_t newval = 0;
      if (get_cell(x, y)) {
        if (n_count == 2 || n_count == 3) {
          newval = 1;
        }
      } else {
        if (n_count == 3) {
          newval = 1;
        }
      }
      val |= newval << --shift;
      if (shift == 0) {
        set_byte(i++, val);
        shift = 8;
        val = 0;
      }
    }
  }
  if (shift < 8) {
    set_byte(i, val);
  }

  frame++;
}

static void new_frame(void *data) {
  app_timer_register(1000 / settings.fps, new_frame, NULL);
  next_gen();
  layer_mark_dirty(s_layer);
}

static void reset() {
  frame = 0;
  cols = width / settings.cell_size;
  rows = height / settings.cell_size;
  size_t len = (rows * cols + 7) / 8;
  free(cells);
  free(cells2);
  cells = malloc(len);
  cells2 = malloc(len);
  for (size_t i = 0; i < len; i++) {
    uint8_t val = rand() % 256;
    cells[i] = val;
    cells2[i] = val;
  }
}

static void accel_data_handler(AccelAxisType axis, int32_t count) { reset(); }

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = layer_get_bounds(window_layer);
  s_layer = layer_create(frame);
  width = frame.size.w;
  height = frame.size.h;
  reset();

  layer_add_child(window_layer, s_layer);
  layer_set_update_proc(s_layer, frame_redraw);
  layer_mark_dirty(s_layer);
  accel_tap_service_subscribe(accel_data_handler);
  new_frame(NULL);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_layer);
  window_destroy(s_main_window);
  free(cells);
  free(cells2);
}

static void default_settings() {
  settings.fps = 12;
  settings.cell_size = 5;
  settings.wrap_edges = true;
  settings.fg_color = GColorBlack;
  settings.bg_color = GColorWhite;
}

static void load_settings() {
  default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  bool should_reset = false;

  Tuple *wrap_edge_t = dict_find(iter, MESSAGE_KEY_EdgeWrap);
  if (wrap_edge_t) {
    settings.wrap_edges = wrap_edge_t->value->int32 == 1;
  }
  Tuple *fps_t = dict_find(iter, MESSAGE_KEY_FPS);
  if (fps_t) {
    settings.fps = fps_t->value->int32;
  }
  Tuple *cell_size_t = dict_find(iter, MESSAGE_KEY_CellSize);
  if (cell_size_t) {
    settings.cell_size = cell_size_t->value->int32;
    should_reset = true;
  }
  Tuple *fg_color_t = dict_find(iter, MESSAGE_KEY_FGColor);
  if (fg_color_t) {
    settings.fg_color = GColorFromHEX(fg_color_t->value->int32);
  }
  Tuple *bg_color_t = dict_find(iter, MESSAGE_KEY_BGColor);
  if (bg_color_t) {
    settings.bg_color = GColorFromHEX(bg_color_t->value->int32);
    window_set_background_color(s_main_window, settings.bg_color);
  }

  if (should_reset) {
    reset();
  }
  save_settings();
}

static void init() {
  load_settings();
  s_main_window = window_create();
  window_set_background_color(s_main_window, settings.bg_color);
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload,
                                            });
  window_stack_push(s_main_window, true);
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(128, 128);
}

int main(void) {
  init();
  app_event_loop();
}
