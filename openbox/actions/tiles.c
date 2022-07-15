#include "openbox/actions.h"
#include "openbox/client.h"
#include "openbox/screen.h"
#include "openbox/frame.h"
#include "openbox/config.h"
#include "openbox/debug.h"
#include "openbox/openbox.h"

enum {
    CURRENT_MONITOR = -1,
    ALL_MONITORS = -2,
    NEXT_MONITOR = -3,
    PREV_MONITOR = -4
};

#define OPTS_FLAG_NONE		(0)
#define OPTS_FLAG_MAXIMIZE_FOCUSED		(1 << 0)

typedef struct {
	guint	num_rows;
	gint	monitor;
	gint	flags;
} Options;

static void free_func(gpointer o);
static gpointer setup_tiles_func(xmlNodePtr node);
static gboolean run_split_tiles_horiz_func(ObActionsData *data,
					   gpointer opts);
static gboolean run_split_tiles_vert_func(ObActionsData *data,
					  gpointer opts);
static gboolean run_tile_reader_mode_func(ObActionsData *data,
					  gpointer opts);

void action_tiles_startup(void)
{
	actions_register("SplitTilesVert", setup_tiles_func,
			free_func, run_split_tiles_vert_func);
	actions_register("SplitTilesHoriz", setup_tiles_func,
			free_func, run_split_tiles_horiz_func);
	actions_register("TileReaderMode", setup_tiles_func,
			free_func, run_tile_reader_mode_func);
}

static gpointer setup_tiles_func(xmlNodePtr node)
{
	xmlNodePtr n;
	Options *o;

	o = g_slice_new0(Options);
	o->flags = OPTS_FLAG_NONE;
	o->monitor = CURRENT_MONITOR;

	if ((n = obt_xml_find_node(node, "rows"))) {
		gchar *s = obt_xml_node_string(n);

		o->num_rows = strtol(s, &s, 10);
	}

	if ((n = obt_xml_find_node(node, "maximize_focused"))) {
		gchar *s = obt_xml_node_string(n);

		if (strcmp(s, "true") == 0)
			o->flags |= OPTS_FLAG_MAXIMIZE_FOCUSED;
	}
	return o;
}

static guint enum_clients(ObClient **focused)
{
	ObClient *last;
	GList *it;
	guint cnt = 0;

	for (it = client_list; it; it = g_list_next(it)) {
		ObClient *client = it->data;

		if (client->desktop != screen_desktop)
			continue;
		if (client->obwin.type != OB_WINDOW_CLASS_CLIENT)
			continue;
		if (client->iconic)
			continue;
		last = client;
		if (client_focused(client))
			*focused = client;
		cnt++;
	}

	if (*focused == NULL)
		*focused = last;

	return cnt;
}

static void offset_client_dimensions(ObClient *client,
				     guint *x, guint *y,
				     guint *w, guint *h)
{
	*w -= ob_rr_theme->fbwidth * 2;
	*h += ob_rr_theme->fbwidth * 2;
}

static gboolean resize_tile(ObClient *client,
			    guint x, guint y,
			    guint w, guint h)
{
	if (client->desktop != screen_desktop)
		return 0;
	if (client->obwin.type != OB_WINDOW_CLASS_CLIENT)
		return 0;
	if (client->iconic)
		return 0;

	offset_client_dimensions(client, &x, &y, &w, &h);
	client_maximize(client, FALSE, 0);
	client_maximize(client, FALSE, 2);
	client_move_resize(client, x, y, w, h);

	return 1;
}

static gboolean run_tile_reader_mode_func(ObActionsData *data, gpointer opts)
{
	Rect *screen_rect = NULL;
	int num_client = 0;
	ObClient *focused = NULL;

	num_client = enum_clients(&focused);
	if (!focused)
		return 0;

	if (num_client != 1)
		return 0;

	screen_rect = screen_area(focused->desktop,
				  client_monitor(focused),
				  NULL);

	resize_tile(focused,
		    screen_rect->x + screen_rect->width / 4,
		    screen_rect->y,
		    screen_rect->width / 2,
		    screen_rect->height);

	return 0;
}

static gboolean run_split_tiles_vert_func(ObActionsData *data, gpointer opts)
{
	Options *o = opts;
	GList *it;
	int num_client = 0;
	int new_width = 0;
	int new_height = 0;
	Rect *screen_rect = NULL;
	ObClient *focused = NULL;
	guint clients_per_row;
	guint clients_last_row;
	guint current_row;

	if (!o->num_rows || o->num_rows < 2)
		return 0;

	num_client = enum_clients(&focused);
	if (!focused)
		return 0;

	if (num_client < 2) {
		client_maximize(focused, TRUE, 0);
		return 0;
	}

	screen_rect = screen_area(focused->desktop,
				  client_monitor(focused),
				  NULL);

	if (num_client >= o->num_rows)
		new_height = screen_rect->height / o->num_rows;
	else
		new_height = screen_rect->height / num_client;

	if (o->flags & OPTS_FLAG_MAXIMIZE_FOCUSED) {
		num_client--;
		clients_per_row = num_client / (o->num_rows - 1);
		if (!clients_per_row)
			clients_per_row = 1;
		clients_last_row = clients_per_row;
		if (num_client > (o->num_rows - 1) &&
				num_client % (o->num_rows - 1)) {
			clients_last_row += (num_client -
					(o->num_rows - 1) * clients_per_row);
		}

		resize_tile(focused,
			    screen_rect->x,
			    screen_rect->y,
			    screen_rect->width,
			    new_height);

		current_row = 1;
		num_client = 0;
	} else {
		clients_per_row = num_client / o->num_rows;
		if (!clients_per_row)
			clients_per_row = 1;
		clients_last_row = clients_per_row;
		if (num_client > o->num_rows && num_client % o->num_rows) {
			clients_last_row += (num_client -
					o->num_rows * clients_per_row);
		}

		resize_tile(focused,
			    screen_rect->x,
			    screen_rect->y,
			    screen_rect->width / clients_per_row,
			    new_height);

		if (clients_per_row == 1) {
			current_row = 1;
			num_client = 0;
		} else {
			current_row = 0;
			num_client = 1;
		}
	}

	for (it = client_list; it; it = g_list_next(it)) {
		ObClient *client = it->data;

		if (client == focused)
			continue;

		if (current_row == o->num_rows - 1) {
			new_width = screen_rect->width / clients_last_row;
			clients_per_row = clients_last_row;
		} else {
			new_width = screen_rect->width / clients_per_row;
		}

		if (!resize_tile(client,
				num_client * new_width,
				screen_rect->y + current_row * new_height,
				new_width,
				new_height))
			continue;

		num_client++;
		if (num_client >= clients_per_row) {
			num_client = 0;
			current_row++;
		}
	}
	return 0;
}

static gboolean run_split_tiles_horiz_func(ObActionsData *data, gpointer opts)
{
	Options *o = opts;
	GList *it;
	int num_client = 0;
	int new_width = 0;
	int start_point = 0;
	Rect *screen_rect = NULL;
	ObClient *focused = NULL;

	num_client = enum_clients(&focused);
	if (!focused)
		return 0;

	if (num_client < 2) {
		client_maximize(focused, TRUE, 0);
		return 0;
	}

	screen_rect = screen_area(focused->desktop,
				  client_monitor(focused),
				  NULL);

	if (o->flags & OPTS_FLAG_MAXIMIZE_FOCUSED) {
		new_width = screen_rect->width / 2;

		resize_tile(focused,
			    screen_rect->x,
			    screen_rect->y,
			    new_width,
			    screen_rect->height);

		num_client--;
		start_point = new_width;
		new_width /= num_client;
	} else {
		new_width = screen_rect->width / num_client;

		resize_tile(focused,
			    screen_rect->x,
			    screen_rect->y,
			    new_width,
			    screen_rect->height);

		start_point = new_width;
	}

	num_client = 0;
	for (it = client_list; it; it = g_list_next(it)) {
		ObClient *client = it->data;

		if (client == focused)
			continue;

		if (resize_tile(client,
				start_point + num_client * new_width,
				screen_rect->y,
				new_width,
				screen_rect->height)) {
			num_client++;
		}
	}

	return 0;
}

static void free_func(gpointer o)
{
	g_slice_free(Options, o);
}
