#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client.h>

#include "protocols/viewporter.h"
#include "protocols/single-pixel-buffer-v1.h"
#include "protocols/wlr-layer-shell-unstable-v1.h"

static struct wl_display *wl_display;
static struct wl_registry *wl_registry;
static struct wl_compositor *wl_compositor;
static struct wp_viewporter *wp_viewporter;
static struct zwlr_layer_shell_v1 *zwlr_layer_shell_v1;
static struct wp_single_pixel_buffer_manager_v1 *wp_single_pixel_buffer_manager_v1;

static struct wl_surface *wl_surface;
static struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1;
static struct wp_viewport *wp_viewport;
static struct wl_buffer *wl_buffer;

static int visible = 1;

const uint32_t default_width = 200;
const uint32_t default_height = 200;

static void noop() {}

static void registry_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		wl_compositor =
				wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	}

	else if (strcmp(interface, wp_viewporter_interface.name) == 0) {
		wp_viewporter =
				wl_registry_bind(registry, name, &wp_viewporter_interface, 1);
	}

	else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		zwlr_layer_shell_v1 =
				wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 4);
	}

	else if (strcmp(interface, wp_single_pixel_buffer_manager_v1_interface.name) == 0) {
		wp_single_pixel_buffer_manager_v1 =
				wl_registry_bind(registry, name, &wp_single_pixel_buffer_manager_v1_interface, 1);
	}
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = noop,
};

static void zwlr_layer_surface_v1_configure(void *data,
		struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1, uint32_t serial,
		uint32_t width, uint32_t height)
{
	zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface_v1, serial);

	width = width > 0 ? width : default_width;
	height = height > 0 ? height : default_height;

	if (visible) {
		wl_surface_attach(wl_surface, wl_buffer, 0, 0);
		wp_viewport_set_destination(wp_viewport, width, width);
	}

	wl_surface_commit(wl_surface);
}

static const struct zwlr_layer_surface_v1_listener zwlr_layer_surface_v1_listener = {
	.configure = zwlr_layer_surface_v1_configure,
	.closed = noop,
};

static void toggle(int signum)
{
	if (visible) {
		visible = 0;

		/*
		 * Attach a NULL buffer to unmap the surface.
		 */
		wl_surface_attach(wl_surface, NULL, 0, 0);
		wl_surface_commit(wl_surface);
	} else {
		visible = 1;

		/*
		 * Assume layer is unmapped, and we can't attach a buffer
		 * until it is first configured.
		 *
		 * Commit without a buffer and attach buffer in ack_configure.
		 */
		wl_surface_commit(wl_surface);
	}

	wl_display_flush(wl_display);
}

int main(void)
{
	wl_display = wl_display_connect(NULL);
	wl_registry = wl_display_get_registry(wl_display);
	wl_registry_add_listener(wl_registry, &registry_listener, NULL);
	wl_display_roundtrip(wl_display);

	assert(wl_compositor && wp_viewporter && zwlr_layer_shell_v1);

	wl_surface = wl_compositor_create_surface(wl_compositor);
	wp_viewport = wp_viewporter_get_viewport(wp_viewporter, wl_surface);
	zwlr_layer_surface_v1 = zwlr_layer_shell_v1_get_layer_surface(
			zwlr_layer_shell_v1, wl_surface, NULL,
			ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "layers");

	assert(wl_surface && wp_viewport && zwlr_layer_surface_v1);

	wl_buffer = wp_single_pixel_buffer_manager_v1_create_u32_rgba_buffer(
			wp_single_pixel_buffer_manager_v1,
			UINT32_MAX * 0.3,
			0,
			0,
			UINT32_MAX * 0.3);

	zwlr_layer_surface_v1_add_listener(zwlr_layer_surface_v1,
			&zwlr_layer_surface_v1_listener, NULL);
	zwlr_layer_surface_v1_set_size(zwlr_layer_surface_v1,
			default_width, default_height);
	wl_surface_commit(wl_surface);

	// pkill -USR1 layer_shell to toggle the surface visibility
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = toggle;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGUSR1, &sa, NULL);

	while (wl_display_dispatch(wl_display) != -1) {}
}
