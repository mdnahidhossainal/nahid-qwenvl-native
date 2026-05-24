Stage 5K — mtmd_init_from_file probe

Purpose:
- Keep Stage 5J safe handle-based symbol path.
- Load main GGUF using llama.cpp.
- Call mtmd_context_params_default.
- Call mtmd_init_from_file(mmproj, model, params).
- Free mtmd context/model immediately.
- No screenshot/image inference yet.

Expected app output:
INIT_STAGE_5K_MTMD_INIT_FROM_FILE_PROBE
MODEL_LOAD_OK ✅
MTMD_INIT_FROM_FILE_OK ✅
STAGE5K_MMPROJ_INIT_PROBE_OK ✅
