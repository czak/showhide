#define _GNU_SOURCE
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include "protocols/xdg-shell.h"

static struct wl_display *wl_display;
static struct wl_registry *wl_registry;
static struct wl_compositor *wl_compositor;
static struct wl_shm *wl_shm;
static struct xdg_wm_base *xdg_wm_base;

static struct wl_surface *wl_surface;
static struct xdg_surface *xdg_surface;
static struct xdg_toplevel *xdg_toplevel;
static struct wl_buffer *wl_buffer;

const int width = 200;
const int height = 200;

static int visible = 1;

static void registry_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		wl_compositor =
				wl_registry_bind(registry, name, &wl_compositor_interface, 5);
	}

	if (strcmp(interface, wl_shm_interface.name) == 0) {
		wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}

	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 2);
	}
}

static void registry_global_remove() {
	/* No op */
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove,
};

static void init_buffer()
{
	if (wl_buffer != NULL) {
		return;
	}

	int stride = width * sizeof(uint32_t);
	int size = stride * height;

	int fd = memfd_create("buffer-pool", 0);
	ftruncate(fd, size);

	struct wl_shm_pool *wl_shm_pool = wl_shm_create_pool(wl_shm, fd, size);
	wl_buffer = wl_shm_pool_create_buffer(
			wl_shm_pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
	uint32_t *pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	// Fill with gray
	memset(pixels, 0x7f, size);

	wl_shm_pool_destroy(wl_shm_pool);
	close(fd);
}

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
		uint32_t serial)
{
	xdg_surface_ack_configure(xdg_surface, serial);

	if (visible) {
		init_buffer();
		wl_surface_attach(wl_surface, wl_buffer, 0, 0);
	}

	wl_surface_commit(wl_surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
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
		 * Commit without a buffer and attach buffer in configure.
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

	assert(wl_compositor && wl_shm && xdg_wm_base);

	wl_surface = wl_compositor_create_surface(wl_compositor);
	xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, wl_surface);
	xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
	xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
	xdg_toplevel_set_min_size(xdg_toplevel, width, height);
	xdg_toplevel_set_max_size(xdg_toplevel, width, height);
	wl_surface_commit(wl_surface);

	// pkill -USR1 layer_shell to toggle the surface visibility
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = toggle;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGUSR1, &sa, NULL);

	while (wl_display_dispatch(wl_display) != -1) {}
}
