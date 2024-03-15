#define _GNU_SOURCE
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include "protocols/wlr-layer-shell-unstable-v1.h"

static struct wl_display *wl_display;
static struct wl_registry *wl_registry;
static struct wl_compositor *wl_compositor;
static struct wl_shm *wl_shm;
static struct zwlr_layer_shell_v1 *zwlr_layer_shell_v1;

static struct wl_surface *wl_surface;
static struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1;

const int width = 200;
const int height = 200;

static int visible = 1;

static void registry_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		wl_compositor =
				wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	}

	if (strcmp(interface, wl_shm_interface.name) == 0) {
		wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}

	else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		zwlr_layer_shell_v1 =
				wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 4);
	}
}

static void registry_global_remove() {
	/* No op */
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove,
};

struct wl_buffer *draw_frame()
{
	static struct wl_buffer *wl_buffer;

	if (wl_buffer != NULL) {
		return wl_buffer;
	}

	int stride = width * sizeof(uint32_t);
	int size = stride * height;

	int fd = memfd_create("buffer-pool", 0);
	ftruncate(fd, size);

	struct wl_shm_pool *wl_shm_pool = wl_shm_create_pool(wl_shm, fd, size);
	wl_buffer = wl_shm_pool_create_buffer(
			wl_shm_pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);

	// Fill with white
	uint32_t *pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memset(pixels, 0xff, size);

	wl_shm_pool_destroy(wl_shm_pool);
	close(fd);

	return wl_buffer;
}

static void zwlr_layer_surface_v1_configure(void *data,
		struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1, uint32_t serial,
		uint32_t width, uint32_t height)
{
	zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface_v1, serial);

	if (visible) {
		struct wl_buffer *wl_buffer = draw_frame();
		wl_surface_attach(wl_surface, wl_buffer, 0, 0);
		wl_surface_damage_buffer(wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	}

	wl_surface_commit(wl_surface);
}

static void zwlr_layer_surface_v1_closed() {
	/* No op */
}

static const struct zwlr_layer_surface_v1_listener zwlr_layer_surface_v1_listener = {
	.configure = zwlr_layer_surface_v1_configure,
	.closed = zwlr_layer_surface_v1_closed,
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
		 * The layer is unmapped, and we can't attach a buffer
		 * until it is first configured.
		 *
		 * Commit without a buffer and wait for first configure.
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

	assert(wl_compositor && wl_shm && zwlr_layer_shell_v1);

	wl_surface = wl_compositor_create_surface(wl_compositor);
	zwlr_layer_surface_v1 = zwlr_layer_shell_v1_get_layer_surface(
			zwlr_layer_shell_v1, wl_surface, NULL,
			ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "layers");

	assert(wl_surface && zwlr_layer_surface_v1);

	zwlr_layer_surface_v1_add_listener(zwlr_layer_surface_v1,
			&zwlr_layer_surface_v1_listener, NULL);
	zwlr_layer_surface_v1_set_size(zwlr_layer_surface_v1, width, height);
	wl_surface_commit(wl_surface);

	// pkill -USR1 layer_shell to toggle the surface visibility
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = toggle;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGUSR1, &sa, NULL);

	while (wl_display_dispatch(wl_display) != -1) {}
}
