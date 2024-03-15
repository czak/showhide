LDLIBS = -lwayland-client

protocols = protocols/xdg-shell.c \
						protocols/xdg-shell.h \
						protocols/wlr-layer-shell-unstable-v1.c \
						protocols/wlr-layer-shell-unstable-v1.h

all: layer_shell xdg_shell

layer_shell: layer_shell.c $(protocols)

xdg_shell: xdg_shell.c $(protocols)

protocols/%.c: protocols/%.xml
	wayland-scanner private-code < $< > $@

protocols/%.h: protocols/%.xml
	wayland-scanner client-header < $< > $@

.PHONY: clean
clean:
	rm -f $(protocols)
	rm -f layer_shell
	rm -f xdg_shell
