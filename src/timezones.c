/* This is a port of code written by Trammell Hudson <hudson@osresearch.net> so that
it can be build using the ./waf build system that is used by most Pebble watch faces. 
This code works on cloudpebble.net. The resources below are needed for the code to run.

/resources/src/fonts/Arial.tff - Resource ID FONT_ARIAL_12
/resources/src/fonts/Arial-Black.tff - Resource ID FONT_ARIAL_BLACK_20
/resources/src/images/menu_icon.png - Resource ID IMAGE_MENU_ICON

The original code for this and other watch faces can be found at https://bitbucket.org/hudson/pebble
*/
/** \file
 * Four time zone clock;
 *
 * When it is day, the text and background are white.
 * When it is night, they are black.
 *
 * Rather than use text layers, it draws the entire frame once per minute.
 *
 * Inspired by a design on RichardG's site that I can't find again to
 * credit the designer. Based on code written by Trammell Hudson <hudson@osresearch.net>
 */
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define UUID { 0x48, 0x92, 0x55, 0xB6, 0x36, 0x8D, 0x48, 0xB9, 0xB8, 0x2E, 0x41, 0x17, 0x2C, 0x98, 0x50, 0x4F }

//When adding the resources, menu_icon.png resource identifier is IMAGE_MENU_ICON
PBL_APP_INFO(
			 UUID,
			 "Timezones",
			 "hutch",
			 1, 0, // Version
			 RESOURCE_ID_IMAGE_MENU_ICON,
			 APP_INFO_WATCH_FACE
			 );

typedef struct
{
	const char * name;
	int offset;
	Layer layer;
} timezone_t;


// Local timezone GMT offset. Change this to your local timezone. If you are frequently in different timezones,
// You will have to make a different watchface for each one with the appropriate offsets. This is because the
// Pebble can only read the current local time and is not location aware as to your current timezone.
static const int gmt_offset = +9 * 60;

#define PEBBLE_SCREEN_WIDTH 144
#define PEBBLE_SCREEN_HEIGHT 168
#define NUM_TIMEZONES 3 //You can change this to whatever you want, but will have to change font sizes to make things fit
#define LAYER_HEIGHT (PEBBLE_SCREEN_HEIGHT / NUM_TIMEZONES)

#define container_of(ptr, type, member) ({ \
char * __mptr = (char*)(uintptr_t) (ptr); \
(type *)(__mptr - offsetof(type,member) ); \
})
// There need to be the same number of timezones as specified above. .offset is the GMT offset for the timezones you want.
// DST will throw things off, so check the actual time in your locations and make sure they are correct. Adjust .offset
// accordingly if the time is off.
static timezone_t timezones[NUM_TIMEZONES] =
{
	// un/comment time zone depending on reuired number of items
	// { .name = "Los Angeles", .offset = -7 * 60 },
	{ .name = "Sao Paulo", .offset = -3 * 60 },
	{ .name = "Tokyo", .offset = +9 * 60 },
	{ .name = "Eindhoven", .offset = +1 * 60 },
};


static Window window;
static PblTm now;
static GFont font_thin;
static GFont font_thick;


static void
timezone_layer_update(
					  Layer * const me,
					  GContext * ctx
					  )
{
	const timezone_t * const tz = container_of(me, timezone_t, layer);
	
	const int orig_hour = now.tm_hour;
	const int orig_min = now.tm_min;
	
	now.tm_min += (tz->offset - gmt_offset) % 60;
	if (now.tm_min > 60)
	{
		now.tm_hour++;
		now.tm_min -= 60;
	} else
        if (now.tm_min < 0)
        {
			now.tm_hour--;
			now.tm_min += 60;
        }
	
	now.tm_hour += (tz->offset - gmt_offset) / 60;
	if (now.tm_hour > 24)
		now.tm_hour -= 24;
	if (now.tm_hour < 0)
		now.tm_hour += 24;
	
	char buf[32];
	string_format_time(
					   buf,
					   sizeof(buf),
					   "%H:%M",
					   &now
					   );
	
	//Swaps the background/foreground colors for night and day. Day=6 and later. Night=18 and later.
	const int night_time = (now.tm_hour > 17 || now.tm_hour < 6);
	now.tm_hour = orig_hour;
	now.tm_min = orig_min;
	
	const int w = me->bounds.size.w;
	const int h = me->bounds.size.h;
	
	// it is night there, draw in black video
	graphics_context_set_fill_color(ctx, night_time ? GColorBlack : GColorWhite);
	graphics_context_set_text_color(ctx, !night_time ? GColorBlack : GColorWhite);
	graphics_fill_rect(ctx, GRect(0, 0, w, h), 0, 0);
	
	graphics_text_draw(ctx,
					   tz->name,
					   font_thin,
					   GRect(0, 0, w, h/3),
					   GTextOverflowModeTrailingEllipsis,
					   GTextAlignmentCenter,
					   NULL
					   );
	
	graphics_text_draw(ctx,
					   buf,
					   font_thick,
					   GRect(0, h/3, w, 2*h/3),
					   GTextOverflowModeTrailingEllipsis,
					   GTextAlignmentCenter,
					   NULL
					   );
}


/** Called once per minute */
static void
handle_tick(
			AppContextRef ctx,
			PebbleTickEvent * const event
			)
{
	(void) ctx;
	
	now = *event->tick_time;
	
	for (int i = 0 ; i < NUM_TIMEZONES ; i++)
		layer_mark_dirty(&timezones[i].layer);
}


static void
handle_init(
			AppContextRef ctx
			)
{
	(void) ctx;
	get_time(&now);
	
	window_init(&window, "Main");
	window_stack_push(&window, true);
	window_set_background_color(&window, GColorBlack);
	
	resource_init_current_app(&APP_RESOURCES);
	
	// If you change the number of timezones, you need to update the ttf resources and these names to the new font size
	// The resource identifier for the fonts are defined here. e.g. "FONT_ARIAL_12"
	if ( NUM_TIMEZONES == 4 ) {
		font_thin = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ARIAL_12));
		font_thick = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ARIAL_BLACK_20));
	} else {
		font_thin = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ARIAL_16));
		font_thick = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ARIAL_BLACK_30));	
	}
	
	for (int i = 0 ; i < NUM_TIMEZONES ; i++)
	{
		timezone_t * const tz = &timezones[i];
		layer_init(
				   &tz->layer,
				   GRect(0, i * LAYER_HEIGHT, PEBBLE_SCREEN_WIDTH, LAYER_HEIGHT)
				   );
		
		tz->layer.update_proc = timezone_layer_update;
		layer_add_child(&window.layer, &tz->layer);
		layer_mark_dirty(&tz->layer);
	}
}


static void
handle_deinit(
			  AppContextRef ctx
			  )
{
	(void) ctx;
	
	fonts_unload_custom_font(font_thin);
	fonts_unload_custom_font(font_thick);
}


void
pbl_main(
		 void * const params
		 )
{
	PebbleAppHandlers handlers = {
		.init_handler   = &handle_init,
		.deinit_handler = &handle_deinit,
		.tick_info      = {
			.tick_handler = &handle_tick,
			.tick_units = MINUTE_UNIT,
		},
	};
	
	app_event_loop(params, &handlers);
}