#include "openbox/actions.h"
#include "openbox/client.h"
#include "openbox/screen.h"
#include "openbox/frame.h"
#include "openbox/config.h"
#include "openbox/debug.h"
#include "openbox/openbox.h"

typedef struct {
	guint num_rows;
} Options;

static void free_func(gpointer o);
static gpointer setup_tiles_func(xmlNodePtr node);
static gboolean run_split_tiles_vert_func(ObActionsData *data,
					  gpointer options);
static gboolean run_focus_tile_func(ObActionsData *data,
					  gpointer options);
static gboolean run_split_tiles_rows_func(ObActionsData *data,
					  gpointer options);

void action_tiles_startup(void)
{
	actions_register("SplitTilesVertically", setup_tiles_func,
			free_func, run_split_tiles_vert_func);
	actions_register("FocusTile", setup_tiles_func,
			free_func, run_focus_tile_func);
	actions_register("SplitTilesRows", setup_tiles_func,
			free_func, run_split_tiles_rows_func);
}

static gpointer setup_tiles_func(xmlNodePtr node)
{
	xmlNodePtr n;
	Options *o;

	o = g_slice_new0(Options);
	if ((n = obt_xml_find_node(node, "rows"))) {
		gchar *s = obt_xml_node_string(n);

		o->num_rows = strtol(s, &s, 10);
	}
	return o;
}

static guint adjusted_height(ObClient *client, guint h)
{
	if (client->undecorated)
		return h;
	return h - ob_rr_theme->title_height;
}

static guint enum_clients(ObClient **focused, guint *new_y)
{
	GList *it;
	guint cnt = 0;

	for (it = client_list; it; it = g_list_next(it)) {
		ObClient *client;

		client = it->data;
		if (client->desktop != screen_desktop)
			continue;
		if (client->obwin.type != OB_WINDOW_CLASS_CLIENT)
			continue;
		if (client->iconic)
			continue;

		if (client->area.y < *new_y)
			*new_y = client->area.y;

		if (client_focused(client))
			*focused = client;

		cnt++;
	}
	return cnt;
}

static gboolean resize_tile(ObClient *client, guint x, guint y,
			    guint w, guint h)
{
	if (client->desktop != screen_desktop)
		return 0;
	if (client->obwin.type != OB_WINDOW_CLASS_CLIENT)
		return 0;
	if (client->iconic)
		return 0;

	h = adjusted_height(client, h);
	client_maximize(client, FALSE, 0);
	client_maximize(client, FALSE, 2);
	client_move_resize(client, x, y, w, h);

	return 1;
}

static gboolean run_split_tiles_rows_func(ObActionsData *data, gpointer options)
{
	Options *o = options;
	GList *it;
	int num_client = 0;
	int new_width = 0;
	int new_height = 0;
	Size screen_sz = get_screen_physical_size();
	int new_y = INT_MAX;
	ObClient *focused = NULL;
	guint clients_per_row;
	guint clients_last_row;
	guint current_row = 0;

	if (!o->num_rows)
		return 0;

	num_client = enum_clients(&focused, &new_y);
	if (num_client < 2) {
		client_maximize(focused, TRUE, 0);
		return 0;
	}

	screen_sz.height -= new_y;
	new_height = screen_sz.height / o->num_rows;
	clients_per_row = num_client / o->num_rows;
	if (!clients_per_row)
		clients_per_row = 1;
	clients_last_row = clients_per_row;
	if (num_client % o->num_rows)
		clients_last_row += 1;

	num_client = 0;
	for (it = client_list; it; it = g_list_next(it)) {
		ObClient *client;

		if (current_row == o->num_rows - 1) {
			new_width = screen_sz.width / clients_last_row;
			clients_per_row = clients_last_row;
		} else {
			new_width = screen_sz.width / clients_per_row;
		}

		client = it->data;
		if (!resize_tile(client,
				num_client * new_width,
				new_y + current_row * new_height,
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

static gboolean run_split_tiles_vert_func(ObActionsData *data, gpointer options)
{
	GList *it;
	int num_client = 0;
	int new_width = 0;
	Size screen_sz = get_screen_physical_size();
	int new_y = INT_MAX;
	ObClient *focused = NULL;

	num_client = enum_clients(&focused, &new_y);
	if (num_client < 2) {
		client_maximize(focused, TRUE, 0);
		return 0;
	}

	screen_sz.height -= new_y;
	new_width = screen_sz.width / num_client;
	num_client = 0;
	if (focused) {
		resize_tile(focused,
			    0, new_y,
			    new_width,
			    screen_sz.height - new_y);
		num_client = 1;
	}

	for (it = client_list; it; it = g_list_next(it)) {
		ObClient *client;

		client = it->data;
		if (client == focused)
			continue;

		if (resize_tile(client,
				num_client * new_width,
				new_y,
				new_width,
				screen_sz.height)) {
			num_client++;
		}
	}

	return 0;
}

static gboolean run_focus_tile_func(ObActionsData *data, gpointer options)
{
	GList *it;
	int num_client = 0;
	int new_width = 0;
	Size screen_sz = get_screen_physical_size();
	int new_y = INT_MAX;
	ObClient *focused = NULL;

	num_client = enum_clients(&focused, &new_y);
	if (!focused)
		return 0;

	if (num_client < 2) {
		client_maximize(focused, TRUE, 0);
		return 0;
	}

	screen_sz.height -= new_y;
	new_width = screen_sz.width / 2;
	num_client--;
	new_width /= num_client;
	num_client = 0;

	resize_tile(focused, 0, new_y,
		    screen_sz.width / 2,
		    screen_sz.height - new_y);

	for (it = client_list; it; it = g_list_next(it)) {
		ObClient *client;

		client = it->data;
		if (client == focused)
			continue;

		if (resize_tile(client,
				screen_sz.width / 2 + num_client * new_width,
				new_y,
				new_width,
				screen_sz.height)) {
			num_client++;
		}
	}

	return 0;
}

static void free_func(gpointer o)
{
	g_slice_free(Options, o);
}
