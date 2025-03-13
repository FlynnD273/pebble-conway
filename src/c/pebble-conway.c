#include <pebble.h>

static Window *s_main_window;
static Layer *s_layer;
static int width;
static int height;
static int frame = 0;

static int fps = 12;

static uint16_t cell_size = 5;
static int rows;
static int cols;
static uint8_t wrap_edges = true;

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
  if (wrap_edges || (x >= 0 && x < cols && y >= 0 && y < rows)) {
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
          ctx, ((val >> --shift) & 1) ? GColorBlack : GColorWhite);
      graphics_fill_rect(
          ctx, GRect(x * cell_size, y * cell_size, cell_size, cell_size), 0, 0);
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
  app_timer_register(1000 / fps, new_frame, NULL);
  next_gen();
  layer_mark_dirty(s_layer);
}

static void reset() {
  frame = 0;
  cols = width / cell_size;
  rows = height / cell_size;
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

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorWhite);
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload,
                                            });
  window_stack_push(s_main_window, true);
}

int main(void) {
  init();
  app_event_loop();
}
