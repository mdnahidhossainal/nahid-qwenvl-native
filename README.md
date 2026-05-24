# Nahid QwenVL Native Build Repo — Stage 5E-B

Purpose:
- Build Android arm64-v8a JNI wrapper `libnahid_qwenvl.so`.
- Clone and build `llama.cpp` shared libraries for Android arm64-v8a in GitHub Actions.
- Package wrapper + llama.cpp / ggml `.so` files in one artifact.

This stage is a **shared-library/backend probe**, not final Qwen-VL inference.

Expected artifact:
`libnahid_qwenvl_llamacpp_arm64_v8a.zip`

Expected inside:
`arm64-v8a/libnahid_qwenvl.so`
plus one or more llama/ggml libraries, such as:
`arm64-v8a/libllama.so`
`arm64-v8a/libggml.so`

After copying all `.so` files into:
`NahidAI/app/src/main/jniLibs/arm64-v8a/`

The app test should show:
- Native library loaded
- INIT_OK
- libllama.so LOAD_OK, if packaging and runtime loading are correct

Next stage:
- Real model load.
- Real screenshot + prompt inference.
