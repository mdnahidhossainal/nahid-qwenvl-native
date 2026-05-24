# Nahid Qwen-VL Native Build Repo - Stage 5E-D

This stage fixes Stage 5E-C by not building the default `all` target.
It builds only llama.cpp library targets (`ggml`, `llama`) and avoids app/executable targets such as `llama-app` / `bin/llama` that caused `build-info.h` failures.

Expected artifact:
- `arm64-v8a/libnahid_qwenvl.so`
- llama.cpp shared libraries such as `libllama.so`, `libggml*.so` if the library-only build succeeds.

This is still a shared-library packaging/probe stage, not final Qwen2.5-VL inference.
