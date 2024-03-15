Example Wayland clients to test surface unmapping/remapping behavior.

# How to test

```
$ make
$ WAYLAND_DEBUG=1 ./xdg_shell

# A white toplevel surface should appear

$ pkill -USR1 xdg_shell

# The surface should disappear

$ pkill -USR1 xdg_shell

# The surface should appear again
```

Replace `xdg_shell` with `layer_shell` to test the hide/show behavior
for `zwlr_layer_shell_v1`.

# Hiding

Hiding is done by:

* attaching a `NULL` buffer 
* committing the surface

The surface should now be unmapped and disappear from the screen.

# Showing

The surface is unmapped, so according to the protocol documentation:

> A newly-unmapped surface is considered to have met condition (1) out
> of the 3 required conditions for mapping a surface if its role surface
> has not been destroyed, i.e. the client **must perform the initial commit
> again before attaching a buffer**.

(emphasis mine)

Therefore, showing is done by:

* committing the surface **without a buffer**
* waiting for `configure`
* attaching the buffer after (well, _in_) the initial `configure`

The surface should now be mapped and show up on screen again.
