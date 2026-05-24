Stage 5L-C Bitmap Pointer API Fix

Fixes Stage 5L-B bitmap probe.
Stage 5L-B interpreted mtmd_helper_bitmap_init_from_file as int32_t (*)(mtmd_bitmap **, const char *), but the Android libmtmd export behaves like mtmd_bitmap * (*)(mtmd_context *, const char *).

This stage:
- loads llama model
- initializes mtmd with mmproj
- calls mtmd_helper_bitmap_init_from_file(mctx, imagePath)
- prints bitmap width/height/bytes if successful
- does not run mtmd_encode or real vision inference yet
