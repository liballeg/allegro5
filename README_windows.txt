Windows-specific notes
======================

DPI Awareness
-------------

By default, apps created with Allegro are marked as DPI aware, and are not
scaled by the OS. This is mostly transparent on your end, the only complication
comes when the DPI changes on the fly (e.g. your app's window gets moved
between displays with different DPIs):

- If the ALLEGRO_DISPLAY was created with the ALLEGRO_RESIZABLE flag it will
  send an ALLEGRO_DISPLAY_RESIZE event. This will have the effect of your app's
  window remaining visually the same, while the display size in pixels will
  increase or decrease. This is the recommended situation.

- If the ALLEGRO_DISPLAY was not created with the ALLEGRO_RESIZABLE flag, then
  the display size in pixels will remain the same, but the app's window will
  appear to grow or shrink.

If you for some reason want to opt out of DPI-awareness, utilize the
application manifests to specify that your app is not DPI-aware.
