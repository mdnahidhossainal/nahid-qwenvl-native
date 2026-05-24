# Nahid QwenVL Stage 5J — Handle-based mtmd symbol probe

This patch is for the **native build repo**:

`/sdcard/Download/nahid_qwenvl_native_build_repo`

Goal:
- keep real image/mmproj inference disabled
- safely open `libmtmd.so` with `dlopen("libmtmd.so")`
- search symbols with `dlsym(handle, "...")`
- confirm whether Android runtime can find the API symbols that appeared in `mtmd_symbols_nm.txt`

Expected app result:
- `INIT_STAGE_5J_HANDLE_BASED_MTMD_SYMBOL_PROBE`
- `DL_OPEN_OK: libmtmd.so`
- `mtmd_context_params_default: FOUND`
- `mtmd_init_from_file: FOUND`
- `mtmd_helper_bitmap_init_from_file: FOUND`
- `MTMD_HANDLE_SYMBOL_PROBE_OK ✅`

If this passes, the next stage will safely try `mtmd_context_params_default + mtmd_init_from_file`.
