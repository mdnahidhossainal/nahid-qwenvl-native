# Stage 5Q — Chat Template + Greedy Generation Probe

This patch attempts to improve Stage 5P gibberish by:

- using a compact Qwen-style chat template,
- using `<__media__>` marker once,
- using image chunk `n_pos` for embedding decode instead of image token count,
- using greedy generation instead of random/top-p sampling,
- keeping all successful Stage 5O/5P load/tokenize/encode/decode steps.

Expected app result markers:

- ANALYZE_STAGE_5Q_CHAT_TEMPLATE_GREEDY_PROBE
- IMAGE_DECODE_POSITIONS_USED=20 ...
- STAGE5P_CHUNK_EVAL_OK ✅
- GENERATION_MODE=GREEDY_CHAT_TEMPLATE
- TEXT_OUTPUT=...
- STAGE5Q_CHAT_TEMPLATE_GREEDY_PROBE_OK ✅
