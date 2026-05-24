# Stage 5J-B — Build-target fix for handle-based mtmd symbol probe

This patch fixes the GitHub Actions failure where the workflow accidentally built the llama.cpp app/executable target (`bin/llama`) and failed during linking.

Change:
- Do NOT run `cmake --build ...` with the default `all` target.
- Build only shared-library targets: `ggml`, `ggml-base`, `ggml-cpu`, `llama`, `mtmd`.
- Keep the Stage 5J JNI code unchanged: it only does safe `dlopen("libmtmd.so")` + handle-based `dlsym(...)` checks.

Expected app result after installing artifact SO files:
- INIT_STAGE_5J_HANDLE_BASED_MTMD_SYMBOL_PROBE
- DL_OPEN_OK: libmtmd.so
- mtmd_context_params_default: FOUND
- mtmd_init_from_file: FOUND
- MTMD_HANDLE_SYMBOL_PROBE_OK ✅
