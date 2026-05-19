The original 0.0.11 Service Explorer release shipped these binary art
assets next to the source tree:

  dir.ico  - icon used for the top-level virtual folder
  file.ico - icon used for individual service entries
  svc.bmp  - 16x16 bitmap strip for the plugin toolbar

The Salamander 5.0 port renders the toolbar bitmap procedurally and now
loads standard shell folder icons at runtime, so the build no longer
depends on these files. They remain documented here in case you want to
keep custom artwork handy for manual overrides or local experimentation.
