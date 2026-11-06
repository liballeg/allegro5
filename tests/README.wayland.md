# Wayland backend regressions

Configure with `WANT_WAYLAND=ON` and `WANT_TESTS=ON`.

Headless clipboard and scroll-frame checks:

```
cmake --build build --target run_wayland_unit_tests
ctest --test-dir build --output-on-failure
```

These tests do not read or change the desktop clipboard. They exercise MIME
selection (including Latin-1 `STRING` decoding), successful transfers,
selection replacement during a transfer, stalled/closed recipients, transfer
limits, cancellation, timeouts, and paired/high-resolution wheel events.

Rendering and display-lifetime checks require a running Wayland compositor:

```
cmake --build build --target run_wayland_tests
```

`test_wayland_render` checks ordinary textures and sub-bitmaps, backbuffer
clipping, bitmap migration between displays, and conversion to memory after
the last display is destroyed. It runs with both fixed and programmable
pipelines. It injects renderer scale metadata and resizes a test window's
physical drawable to check 125% and 200% backbuffer reads, writes, and copies;
125% drawable checks are skipped when fractional scaling is unavailable.
It does not change the desktop's output configuration. These are controlled
regressions, not a replacement for testing native output moves/hotplug on
multiple compositors.

The fractional-protocol-disabled build should also compile and run these
tests. HiDPI backbuffer-to-texture copies currently use a correctness-first
software fallback, because `glCopyTexSubImage2D` cannot resample a physical
buffer into logical-sized texture pixels.
