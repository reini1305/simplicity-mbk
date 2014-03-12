#include "pebble.h"

// TODOS:
// - Countdown next show
// - Invert color based on time
// - During rehearsal show time left ;)
// - Battery Indicator (check)

Window *window;
TextLayer *text_date_layer;
TextLayer *text_time_layer;
TextLayer *next_show_layer;
static BitmapLayer *image_layer;
static BitmapLayer *BT_icon_layer;
static BitmapLayer *battery_icon_layer;
static InverterLayer *inverter_layer;
static GBitmap *image;
static GBitmap *BT_image;
static GBitmap *battery_image;

enum {
  HOUR1_KEY = 0,
  HOUR2_KEY = 1,
  NEXT_SHOW_KEY = 2
};
static int hour1,hour2,current_hour,next_show;

Layer *line_layer;
static bool first_start=true;
static bool hidden=false;

void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void updateInverterLayer(void) {
  if((current_hour>=hour1)||(current_hour<=hour2) )
  {
    //if(!hidden) //not already hidden
    {
      hidden = true;
      layer_set_hidden(inverter_layer_get_layer(inverter_layer),hidden);
      layer_mark_dirty(inverter_layer_get_layer(inverter_layer));
    }
    //else{} // do nothing
  }
  else
  {
    //if(hidden) //not hidden
    {
      hidden = false;
      layer_set_hidden(inverter_layer_get_layer(inverter_layer),hidden);
      layer_mark_dirty(inverter_layer_get_layer(inverter_layer));
    }
    //else{} // do nothing
    
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Inverter Layer called %d",current_hour);
}

void updateNextShowLayer(void){
  static char next_text[] ="     ";
  if(next_show>0)
  {
    if(next_show>(60*60*24*7)){ // we are weeks away
      snprintf(next_text,5,"%dW",(int)(next_show/(60*60*24*7)));
    }
    else if(next_show>(60*60*24)){ // we are just days away
      snprintf(next_text,5,"%dD",(int)(next_show/(60*60*24)));
    }
  }
  text_layer_set_text(next_show_layer,next_text);
  layer_mark_dirty(text_layer_get_layer(next_show_layer));
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Saturday,   Xxxxxxxxx 00";

  char *time_format;


  // TODO: Only update the date when it's changed.
  strftime(date_text, sizeof(date_text), "%A\n%B %e", tick_time);
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
  current_hour = tick_time->tm_hour;
  updateNextShowLayer();
  updateInverterLayer();
}

void handle_bluetooth(bool connected)

{
  if(connected)
  {
    BT_image = gbitmap_create_with_resource(RESOURCE_ID_BT_CONNECTED);
    if(!first_start)
      vibes_double_pulse();
  }
  else
  {
    BT_image = gbitmap_create_with_resource(RESOURCE_ID_BT_OFF);
    vibes_long_pulse();
  }
  first_start = false;
  bitmap_layer_set_bitmap(BT_icon_layer, BT_image);
}

void handle_tap(AccelAxisType axis, int32_t direction) {
  // Process tap on ACCEL_AXIS_X, ACCEL_AXIS_Y or ACCEL_AXIS_Z
  // Direction is 1 or -1
  hidden=!hidden;
  layer_set_hidden(inverter_layer_get_layer(inverter_layer),hidden);
  layer_mark_dirty(inverter_layer_get_layer(inverter_layer));
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
		
		// TODO: Why is this needed to redraw the bitmap?
		//force_tick(ALL_UNITS);
}


static void in_handler(DictionaryIterator *iter, void *context) {
  Tuple *hour1_tuple = dict_find(iter, HOUR1_KEY);
  Tuple *hour2_tuple = dict_find(iter, HOUR2_KEY);
  Tuple *next_tuple = dict_find(iter, NEXT_SHOW_KEY);
  if (hour1_tuple) {
    hour1 = hour1_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got new configuration: %d %d",hour1,hour2 );
  }
  if (hour2_tuple) {
    hour2 = hour2_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got new configuration: %d %d",hour1,hour2 );
  }
  if(next_tuple) {
    next_show = next_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Next show in: %d",next_show);
    updateNextShowLayer();
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got new configuration: %d",(int)dict_size(iter));
  updateInverterLayer();
}

void get_next_show(){
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  Tuplet value = TupletInteger(1, 42);
  dict_write_tuplet(iter, &value);
  app_message_outbox_send();
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Gimme new next show!");
  app_message_outbox_send();
}


void handle_deinit(void) {
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  //accel_tap_service_unsubscribe();
  battery_state_service_unsubscribe();
  
  gbitmap_destroy(image);
  bitmap_layer_destroy(image_layer);
  gbitmap_destroy(BT_image);
  bitmap_layer_destroy(BT_icon_layer);
  gbitmap_destroy(battery_image);
  bitmap_layer_destroy(battery_icon_layer);
  
  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_time_layer);
  text_layer_destroy(next_show_layer);
  
  inverter_layer_destroy(inverter_layer);
  window_destroy(window);

}

void handle_init(void) {
  app_message_register_inbox_received(in_handler);
  const int inbound_size = 64;
  const int outbound_size = 64;
  app_message_open(inbound_size, outbound_size);
  
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  // This needs to be deinited on app exit which is when the event loop ends
  image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MBK);
  image_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(image_layer,image);
  bitmap_layer_set_alignment(image_layer,GAlignCenter);
  layer_add_child(window_layer,bitmap_layer_get_layer(image_layer));

  text_date_layer = text_layer_create(GRect(8, 168-95, 144-8, 80));
  text_layer_set_text_color(text_date_layer, GColorWhite);
  text_layer_set_background_color(text_date_layer, GColorClear);
  //text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_18)));
  text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

  text_time_layer = text_layer_create(GRect(7, 168-40, 144-7, 40));
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_33)));
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));
  
  next_show_layer = text_layer_create(GRect(0, 5, 38, 40));
  text_layer_set_text_color(next_show_layer, GColorWhite);
  text_layer_set_text_alignment(next_show_layer,GTextAlignmentRight);
  text_layer_set_background_color(next_show_layer, GColorClear);
  //text_layer_set_font(next_show_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_18)));
  text_layer_set_font(next_show_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(window_layer, text_layer_get_layer(next_show_layer));

  GRect line_frame = GRect(8, 168-37, 84, 2);
  line_layer = layer_create(line_frame);
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_layer, line_layer);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  // TODO: Update display here to avoid blank display on launch?
  
  bluetooth_connection_service_subscribe(handle_bluetooth);
  BT_icon_layer = bitmap_layer_create(GRect(144-10 , 2 , 8, 13));
  bitmap_layer_set_bitmap(BT_icon_layer, BT_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(BT_icon_layer));
  handle_bluetooth(bluetooth_connection_service_peek());
  bitmap_layer_set_bitmap(BT_icon_layer, BT_image);
  
  battery_icon_layer = bitmap_layer_create(GRect(2,2,16,10));
  layer_add_child(window_layer,bitmap_layer_get_layer(battery_icon_layer));
  battery_state_service_subscribe(handle_battery);
  handle_battery(battery_state_service_peek());
  
  inverter_layer = inverter_layer_create(bounds);
  layer_add_child(window_layer,inverter_layer_get_layer(inverter_layer));
  layer_set_hidden(inverter_layer_get_layer(inverter_layer),true);
  hidden=true;
  //accel_tap_service_subscribe(&handle_tap);
  
  // get new show information
  get_next_show();
}


int main(void) {
  handle_init();

  app_event_loop();
  
  handle_deinit();
}
