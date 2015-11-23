#include "pebble.h"

// TODOS:
// - During rehearsal show time left ;)

Window *window;
TextLayer *text_date_layer;
TextLayer *text_time_layer;
//TextLayer *next_show_layer;
static BitmapLayer *image_layer;
static BitmapLayer *BT_icon_layer;
static BitmapLayer *battery_icon_layer;
//static InverterLayer *inverter_layer;
static GBitmap *image;
static GBitmap *BT_image;
static GBitmap *battery_image;
static GFont font;

enum {
  HOUR1_KEY = 0,
  HOUR2_KEY = 1,
  NEXT_SHOW_KEY = 2
};
//static int current_hour;

Layer *line_layer;
static bool first_start=true;
//static bool hidden=false;

void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBlack,GColorWhite));
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Saturday,   Xxxxxxxxx 00";

  char *time_format;


  // TODO: Only update the date when it's changed.
  strftime(date_text, sizeof(date_text), "%A\n%e. %B", tick_time);
  text_layer_set_text(text_date_layer, date_text);


  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(text_time_layer, time_text);

}

void handle_bluetooth(bool connected)

{
  if(connected)
  {
    layer_set_hidden(bitmap_layer_get_layer(BT_icon_layer),false);
    if(!first_start)
      vibes_double_pulse();
  }
  else
  {
    layer_set_hidden(bitmap_layer_get_layer(BT_icon_layer),true);
    vibes_long_pulse();
  }
  first_start = false;
}


uint32_t get_resource_for_battery_state(BatteryChargeState battery) {
	if((battery.is_charging || battery.is_plugged) && battery.charge_percent > 99)
		return RESOURCE_ID_CHARGED;
	
	if(battery.charge_percent >= 99)
		return battery.is_charging ? RESOURCE_ID_CHARGING_100 : RESOURCE_ID_BATTERY_100;
	if(battery.charge_percent >= 91)
		return battery.is_charging ? RESOURCE_ID_CHARGING_91 : RESOURCE_ID_BATTERY_91;
	if(battery.charge_percent >= 82)
		return battery.is_charging ? RESOURCE_ID_CHARGING_82 : RESOURCE_ID_BATTERY_82;
	if(battery.charge_percent >= 73)
		return battery.is_charging ? RESOURCE_ID_CHARGING_73 : RESOURCE_ID_BATTERY_73;
	if(battery.charge_percent >= 64)
		return battery.is_charging ? RESOURCE_ID_CHARGING_64 : RESOURCE_ID_BATTERY_64;
	if(battery.charge_percent >= 55)
		return battery.is_charging ? RESOURCE_ID_CHARGING_55 : RESOURCE_ID_BATTERY_55;
	if(battery.charge_percent >= 46)
		return battery.is_charging ? RESOURCE_ID_CHARGING_46 : RESOURCE_ID_BATTERY_46;
	if(battery.charge_percent >= 37)
		return battery.is_charging ? RESOURCE_ID_CHARGING_37 : RESOURCE_ID_BATTERY_37;
	if(battery.charge_percent >= 28)
		return battery.is_charging ? RESOURCE_ID_CHARGING_28 : RESOURCE_ID_BATTERY_28;
	if(battery.charge_percent >= 19)
		return battery.is_charging ? RESOURCE_ID_CHARGING_19 : RESOURCE_ID_BATTERY_19;
	if(battery.charge_percent >= 9)
		return battery.is_charging ? RESOURCE_ID_CHARGING_9 : RESOURCE_ID_BATTERY_9;
	
	return battery.is_charging ? RESOURCE_ID_CHARGING_0 : RESOURCE_ID_BATTERY_0;
}

void handle_battery(BatteryChargeState battery) {
	uint32_t new_battery_resource = get_resource_for_battery_state(battery);
  battery_image= gbitmap_create_with_resource(new_battery_resource);
  bitmap_layer_set_bitmap(battery_icon_layer, battery_image);
		
}

void handle_deinit(void) {
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  
  gbitmap_destroy(image);
  bitmap_layer_destroy(image_layer);
  gbitmap_destroy(BT_image);
  bitmap_layer_destroy(BT_icon_layer);
  gbitmap_destroy(battery_image);
  bitmap_layer_destroy(battery_icon_layer);
  
  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_time_layer);
  
  fonts_unload_custom_font(font);
  
  layer_destroy(line_layer);

  window_destroy(window);

}

void handle_init(void) {
  
  setlocale(LC_TIME, "");
  
  window = window_create();
  window_stack_push(window, true /* Animated */);
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  // This needs to be deinited on app exit which is when the event loop ends
  image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MBK);
  image_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(image_layer,image);
  bitmap_layer_set_compositing_mode(image_layer,GCompOpSet);
  bitmap_layer_set_alignment(image_layer,GAlignCenter);
  layer_add_child(window_layer,bitmap_layer_get_layer(image_layer));

  text_date_layer = text_layer_create(GRect(4, 168-95, 144-8, 80));
  text_layer_set_text_color(text_date_layer, PBL_IF_COLOR_ELSE(GColorBlack,GColorWhite));
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

  text_time_layer = text_layer_create(GRect(3, 168-40, 144-7, 40));
  text_layer_set_text_color(text_time_layer, PBL_IF_COLOR_ELSE(GColorBlack,GColorWhite));
  text_layer_set_background_color(text_time_layer, GColorClear);
  font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_33));
  text_layer_set_font(text_time_layer, font);
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

  GRect line_frame = GRect(4, 168-37, 80, 2);
  line_layer = layer_create(line_frame);
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_layer, line_layer);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  // TODO: Update display here to avoid blank display on launch?
  
  BT_icon_layer = bitmap_layer_create(GRect(144-10 , 2 , 8, 13));
  bitmap_layer_set_bitmap(BT_icon_layer, gbitmap_create_with_resource(RESOURCE_ID_BT_CONNECTED));
  layer_add_child(window_layer, bitmap_layer_get_layer(BT_icon_layer));
  bluetooth_connection_service_subscribe(handle_bluetooth);
  handle_bluetooth(bluetooth_connection_service_peek());
  
  battery_icon_layer = bitmap_layer_create(GRect(2,2,16,10));
  layer_add_child(window_layer,bitmap_layer_get_layer(battery_icon_layer));
  battery_state_service_subscribe(handle_battery);
  handle_battery(battery_state_service_peek());
}


int main(void) {
  handle_init();

  app_event_loop();
  
  handle_deinit();
}
