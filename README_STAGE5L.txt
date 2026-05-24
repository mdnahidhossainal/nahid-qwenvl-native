# Stage 5L — Safe Screenshot Bitmap Load Probe

This patch keeps the Stage 5K mmproj init probe in nativeInit and adds a safe nativeAnalyze bitmap load test.

Expected app result:

- INIT_STAGE_5L_BITMAP_LOAD_PROBE
- MODEL_LOAD_OK ✅
- MTMD_INIT_FROM_FILE_OK ✅
- STAGE5L_MMPROJ_INIT_PROBE_OK ✅
- ANALYZE_STAGE_5L_BITMAP_LOAD_PROBE
- BITMAP_LOAD_OK ✅
- STAGE5L_BITMAP_LOAD_PROBE_OK ✅

This still does not run real image inference. It only proves libmtmd can decode the saved screenshot file.
