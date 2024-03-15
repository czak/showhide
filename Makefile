LDLIBS = -lwayland-client

protocols = protocols/single-pixel-buffer-v1.c \
						protocols/viewporter.c \
						protocols/wlr-layer-shell-unstable-v1.c \
						protocols/xdg-shell.c

layer_shell: layer_shell.c $(protocols) 
