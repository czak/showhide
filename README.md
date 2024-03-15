Example Wayland clients to test surface unmapping/remapping behavior.

# How to test

```shell
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

# Demo: `sway`

Recorded in `sway version 1.10-dev-dc9f2173 (Mar 15 2024, branch 'master')`.

Result: behaves as described above

https://github.com/czak/showhide/assets/14021/598f285b-098d-45dd-87b7-00d6ac4df2f6

# Demo: `weston`

Recorded in `commit f4c69abc577f36e33da2f2bfe81c51efcac2ff01 (HEAD -> main)`

Result: first `commit` after unmapping doesn't trigger `configure`.

https://github.com/czak/showhide/assets/14021/e0289a3c-00a8-4992-89d2-fc16c1bdc5fc




