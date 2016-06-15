#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer_h;
static TextLayer *s_time_layer_m;
static Layer *s_battery_layer;
static int s_battery_level;
static GFont s_time_font_h;
static GFont s_time_font_m;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer_h[21];
  strftime(s_buffer_h, sizeof(s_buffer_h), "%H:%M:%S\n%I:%M:%S %P", tick_time);


  static char s_buffer_d[12];
  strftime(s_buffer_d, sizeof(s_buffer_d), "%a %b %d", tick_time);

  static char s_buffer_d2[14];
	s_buffer_d2[0] = '\0';
	
	// Has the date one or two numbers?
	if (s_buffer_d[8] == '0')
	{
		static char s_buffer_de[11];
  		strftime(s_buffer_de, sizeof(s_buffer_de), "%a %b%e", tick_time);
		strcat(s_buffer_d2,s_buffer_de); 
	} else {
		// Display this time on the TextLayer
		strcat(s_buffer_d2,s_buffer_d); 
	}

	// append suffix
	if (s_buffer_d[9] == '1')
	{	
		strcat(s_buffer_d2,"st"); 
	} else if (s_buffer_d[9] == '2') {	
		strcat(s_buffer_d2,"nd"); 
	} else if (s_buffer_d[9] == '3') {	
		strcat(s_buffer_d2,"rd"); 
	} else {	
		strcat(s_buffer_d2,"th"); 
	}

  // Display this time on the TextLayer
  	text_layer_set_text(s_time_layer_h, s_buffer_h);
  	text_layer_set_text(s_time_layer_m, s_buffer_d2);	

}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * bounds.size.w);

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorClear);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorBlack);
  // top
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, 1), 0, GCornerNone);
  // battery
  graphics_fill_rect(ctx, GRect(0, 2, width, 1), 0, GCornerNone);
  // bottom
  graphics_fill_rect(ctx, GRect(0, 4, bounds.size.w, 1), 0, GCornerNone);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
// Update meter
layer_mark_dirty(s_battery_layer);
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
	
  // Create the TextLayer with specific bounds
  s_time_layer_h = text_layer_create(
      GRect(0, (bounds.size.h/2)-40, bounds.size.w, 48));
	
  s_time_layer_m = text_layer_create(
      GRect(0, (bounds.size.h/2)+20, bounds.size.w, 20));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer_h, GColorClear);
  text_layer_set_text_color(s_time_layer_h, GColorBlack);
  text_layer_set_text(s_time_layer_h, "0");
  text_layer_set_text_alignment(s_time_layer_h, GTextAlignmentCenter);

  text_layer_set_background_color(s_time_layer_m, GColorClear);
  text_layer_set_text_color(s_time_layer_m, GColorBlack);
  text_layer_set_text(s_time_layer_m, "00");
  text_layer_set_text_alignment(s_time_layer_m, GTextAlignmentCenter);

	
  // Create GFont
  s_time_font_h = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_HOUR_24));
  s_time_font_m = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MIN_16));

  // Apply to TextLayer
  text_layer_set_font(s_time_layer_h, s_time_font_h);
  text_layer_set_font(s_time_layer_m, s_time_font_m);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer_h));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer_m));


// Create battery meter Layer
s_battery_layer = layer_create(GRect(bounds.size.w/3, bounds.size.h - 20, bounds.size.w/3, 5));
layer_set_update_proc(s_battery_layer, battery_update_proc);

// Add to Window
layer_add_child(window_get_root_layer(window), s_battery_layer);


}

	
static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer_h);
  text_layer_destroy(s_time_layer_m);

  // Unload GFont
  fonts_unload_custom_font(s_time_font_h);
  fonts_unload_custom_font(s_time_font_m);

// Destroy BatteryLayer
layer_destroy(s_battery_layer);

}


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set the background color
  window_set_background_color(s_main_window, GColorWhite);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

// Register for battery level updates
battery_state_service_subscribe(battery_callback);

// Ensure battery level is displayed from the start
battery_callback(battery_state_service_peek());
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
