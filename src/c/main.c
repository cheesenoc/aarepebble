#include <pebble.h>

#define REFRESH_INTERVAL_MS (5 * 60 * 1000)

enum {
  APP_KEY_REQUEST = 0,
  APP_KEY_TEMPERATURE = 1,
  APP_KEY_TIME = 2,
  APP_KEY_SHORT_TEXT = 3
};

static Window *s_main_window;
static TextLayer *s_title_layer;
static TextLayer *s_temp_layer;
static TextLayer *s_time_layer;
static TextLayer *s_short_text_layer;
static AppTimer *s_refresh_timer;

static void schedule_refresh(void);

static void set_time_text(const char *text) {
  text_layer_set_text(s_time_layer, text);
}

static void set_short_text(const char *text) {
  text_layer_set_text(s_short_text_layer, text);
}

static void request_temperature(void) {
  DictionaryIterator *out_iter;
  AppMessageResult result = app_message_outbox_begin(&out_iter);

  if (result != APP_MSG_OK) {
    set_time_text("Phone link unavailable");
    set_short_text("");
    schedule_refresh();
    return;
  }

  dict_write_uint8(out_iter, APP_KEY_REQUEST, 1);
  result = app_message_outbox_send();

  if (result != APP_MSG_OK) {
    set_time_text("Request failed");
    set_short_text("");
    schedule_refresh();
  }
}

static void refresh_timer_callback(void *context) {
  request_temperature();
}

static void schedule_refresh(void) {
  if (s_refresh_timer) {
    app_timer_cancel(s_refresh_timer);
  }

  s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *temperature = dict_find(iterator, APP_KEY_TEMPERATURE);
  Tuple *time_text = dict_find(iterator, APP_KEY_TIME);
  Tuple *short_text = dict_find(iterator, APP_KEY_SHORT_TEXT);

  if (temperature) {
    text_layer_set_text(s_temp_layer, temperature->value->cstring);
  }

  if (short_text) {
    set_time_text(short_text->value->cstring);
  }

  if (time_text) {
    set_short_text(time_text->value->cstring);
  }

  schedule_refresh();
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  set_time_text("Message dropped");
  set_short_text("");
  schedule_refresh();
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  set_time_text("Phone request failed");
  set_short_text("");
  schedule_refresh();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  set_time_text("Refreshing...");
  set_short_text("");
  request_temperature();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_title_layer = text_layer_create(GRect(0, 22, bounds.size.w, 30));
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_text_color(s_title_layer, GColorCeleste);
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text(s_title_layer, "Aare Bern");
  layer_add_child(window_layer, text_layer_get_layer(s_title_layer));

  s_temp_layer = text_layer_create(GRect(0, 78, bounds.size.w, 64));
  text_layer_set_background_color(s_temp_layer, GColorClear);
  text_layer_set_text_color(s_temp_layer, GColorWhite);
  text_layer_set_text_alignment(s_temp_layer, GTextAlignmentCenter);
  text_layer_set_font(s_temp_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text(s_temp_layer, "--.- C");
  layer_add_child(window_layer, text_layer_get_layer(s_temp_layer));

  s_time_layer = text_layer_create(GRect(24, 130, bounds.size.w - 48, 24));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_time_layer, "");
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_short_text_layer = text_layer_create(GRect(12, 168, bounds.size.w - 24, 28));
  text_layer_set_background_color(s_short_text_layer, GColorClear);
  text_layer_set_text_color(s_short_text_layer, GColorCeleste);
  text_layer_set_text_alignment(s_short_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_short_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text(s_short_text_layer, "--:--");
  layer_add_child(window_layer, text_layer_get_layer(s_short_text_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_temp_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_short_text_layer);
}

static void init(void) {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorOxfordBlue);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_stack_push(s_main_window, true);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_open(128, 128);

  request_temperature();
}

static void deinit(void) {
  if (s_refresh_timer) {
    app_timer_cancel(s_refresh_timer);
  }

  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
