# Stage 5S — Concise Bengali Screen Answer Probe

This update keeps the Stage 5R working vision pipeline, but uses a shorter prompt and an assistant answer prefix to improve answer quality. It still prints debug logs, but the useful final answer is shown in `USER_FINAL_ANSWER_ONLY=`.

Expected app result:

```text
ANALYZE_STAGE_5S_CONCISE_SCREEN_ANSWER_PROBE
IMAGE_EMBD_DECODE_OK ✅
GENERATION_MODE=GREEDY_CONCISE_BENGALI_SCREEN_ANSWER
USER_FINAL_ANSWER_ONLY=স্ক্রিনে দেখা যাচ্ছে ...
STAGE5S_CONCISE_SCREEN_ANSWER_PROBE_OK ✅
```
