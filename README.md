# Nahid QwenVL Native Build Repo — Stage 5E-C

Purpose:
- Fix Stage 5E-B failure by disabling llama.cpp apps/tools/examples/tests.
- Build only Android arm64-v8a shared libraries from llama.cpp.
- Build the Nahid JNI wrapper `libnahid_qwenvl.so`.
- Package all produced `.so` files into one artifact.

This stage is still a shared-library/backend probe, not final Qwen-VL inference.

Expected artifact:
`libnahid_qwenvl_llamacpp_arm64_v8a_stage5e_c.zip`

Expected inside:
- `arm64-v8a/libnahid_qwenvl.so`
- one or more llama.cpp / ggml `.so` files, such as `libllama.so` and `libggml*.so`

If this succeeds, the next stage will connect real model load and real screenshot + prompt inference.
