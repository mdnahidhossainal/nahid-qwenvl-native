# Nahid QwenVL Native Build Repo — Stage 5F

Stage 5F verifies that Android can load the real main GGUF using llama.cpp.
It does not perform real screenshot/image inference yet.

Expected output in the app:
- libllama.so LOAD_OK
- MODEL_LOAD_OK ✅ if qwen2.5-vl-3b-ui-grounding.q4_k_s.gguf can be loaded

If MODEL_LOAD_FAILED appears, the native package is working but the GGUF/runtime pairing needs adjustment.
