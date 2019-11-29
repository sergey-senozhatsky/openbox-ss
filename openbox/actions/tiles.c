#include "openbox/actions.h"
#include "openbox/client.h"
#include "openbox/screen.h"
#include "openbox/frame.h"
#include "openbox/config.h"
#include "openbox/debug.h"

enum {
    CURRENT_MONITOR = -1,
    ALL_MONITORS = -2,
    NEXT_MONITOR = -3,
    PREV_MONITOR = -4
};

typedef struct {
    GravityCoord x;
    GravityCoord y;
    gint w;
    gint w_denom;
    gint h;
    gint h_denom;
    gint monitor;
    gboolean w_sets_client_size;
    gboolean h_sets_client_size;
} Options;

static void free_func(gpointer o);
static gpointer setup_tiles_func(xmlNodePtr node);
static gboolean run_split_tiles_vert_func(ObActionsData *data,
					  gpointer options);
static gboolean run_focus_tile_vert_func(ObActionsData *data,
					  gpointer options);

void action_tiles_startup(void)
{
    actions_register("SplitTilesVertically", setup_tiles_func,
		    free_func, run_split_tiles_vert_func);
    actions_register("FocusTileVerrtically", setup_tiles_func,
		    free_func, run_focus_tile_vert_func);
}

static gpointer setup_tiles_func(xmlNodePtr node)
{
	Options *o;

	o = g_slice_new0(Options);
	o->monitor = CURRENT_MONITOR;
	return o;
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

		ob_debug("enum %s\n", client->name);

		cnt++;
	}
	return cnt;
}

static gboolean resize_tile(ObClient *client, guint x, guint y, guint w, guint h)
{
	ob_debug("attempt to resize %s\n", client->name);

	if (client->desktop != screen_desktop)
		return 0;
	if (client->obwin.type != OB_WINDOW_CLASS_CLIENT)
		return 0;
	if (client->iconic)
		return 0;

	client_maximize(client, FALSE, 1);
	client_move_resize(client, x, y, w, h);
	return 1;
}

static gboolean run_split_tiles_vert_func(ObActionsData *data, gpointer options)
{
	GList *it;
	int cnum = 0;
	int new_width = 0;
	Size screen_sz = get_screen_physical_size();
	int new_y = INT_MAX;
	ObClient *focused = NULL;

	cnum = enum_clients(&focused, &new_y);
	ob_debug(">>> %d %d\n", cnum, new_y);
	if (cnum < 2)
		return 0;

	new_width = screen_sz.width / cnum;
	cnum = 0;
	if (focused) {
		resize_tile(focused,
			    0, new_y,
			    new_width,
			    screen_sz.height - new_y);
		cnum = 1;
	}

	for (it = client_list; it; it = g_list_next(it)) {
		ObClient *client;

		client = it->data;
		if (client == focused)
			continue;

		ob_debug("resize %s\n", client->name);

		if (resize_tile(client,
				cnum * new_width, new_y,
				new_width,
				screen_sz.height - new_y)) {
			cnum++;
		}
	}

	return 0;
}

static gboolean run_focus_tile_vert_func(ObActionsData *data, gpointer options)
{
	GList *it;
	int cnum = 0;
	int new_width = 0;
	Size screen_sz = get_screen_physical_size();
	int new_y = INT_MAX;
	ObClient *focused = NULL;

	cnum = enum_clients(&focused, &new_y);
	if (!focused)
		return 0;

	if (cnum < 2)
		return 0;

	new_width = screen_sz.width / 2;
	cnum--;
	new_width /= cnum;
	cnum = 0;

	resize_tile(focused, 0, new_y,
		    screen_sz.width / 2,
		    screen_sz.height - new_y);

	for (it = client_list; it; it = g_list_next(it)) {
		ObClient *client;

		client = it->data;
		if (client == focused)
			continue;

		if (resize_tile(client,
				screen_sz.width / 2 + cnum * new_width,
				new_y,
				new_width,
				screen_sz.height - new_y)) {
			cnum++;
		}
	}

	return 0;
}

static void free_func(gpointer o)
{
	g_slice_free(Options, o);
}
