# Nahid QwenVL Native Build Repo — Stage 5H-Lite

Stage 5H-Lite is a safer multimodal probe after the full Stage 5H crashed on-device.

It keeps the known-stable Stage 5G text-generation path, and only checks multimodal (`mtmd`) symbols using `dlsym(RTLD_DEFAULT, ...)`.

It does **not** call mtmd init, mmproj loading, image encoding, or screenshot inference yet.

Expected app output:
- INIT_STAGE_5H_LITE_SAFE_MULTIMODAL_SYMBOL_PROBE
- MODEL_LOAD_OK ✅
- CONTEXT_CREATE_OK ✅
- TEXT_GENERATION_OK ✅
- MTMD_SYMBOL_PROBE_SAFE ✅
