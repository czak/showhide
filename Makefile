LDLIBS = -lwayland-client

protocols = protocols/single-pixel-buffer-v1.c \
						protocols/viewporter.c \
						protocols/wlr-layer-shell-unstable-v1.c \
						protocols/xdg-shell.c

all: layer_shell xdg_shell

layer_shell: layer_shell.c $(protocols)

xdg_shell: xdg_shell.c $(protocols)
