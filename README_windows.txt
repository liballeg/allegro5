Windows-specific notes
======================

DPI Awareness
-------------

By default, apps created with Allegro are marked as DPI aware, and are not
scaled by the OS. This is mostly transparent on your end, the only complication
comes when the DPI changes on the fly (e.g. your app's window gets moved
between displays with different DPIs):

- If the A5O_DISPLAY was created with the A5O_RESIZABLE flag it will
  send an A5O_DISPLAY_RESIZE event. This will have the effect of your app's
  window remaining visually the same, while the display size in pixels will
  increase or decrease. This is the recommended situation.

- If the A5O_DISPLAY was not created with the A5O_RESIZABLE flag, then
  the display size in pixels will remain the same, but the app's window will
  appear to grow or shrink.

If you for some reason want to opt out of DPI-awareness, utilize the
application manifests to specify that your app is not DPI-aware.

Runtime DLL Loading
-------------------

For some of its features, Allegro will look for certain DLLs on the
system it runs on, but will not explicitly depend them at build time.
These are as follows:

- msftedit.dll or riched20.dll or riched32.dll - For the native dialog addon.

- d3dx9_${version}.dll - Shader support for the Direct3D backend.

- xinput1_${version}.dll - XInput-based joysticks.
