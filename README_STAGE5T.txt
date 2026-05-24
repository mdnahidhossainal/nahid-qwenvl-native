# Stage 5T — Smart Response Policy Probe

This update keeps the working Stage 5S image -> decode -> generation path, but changes the system prompt policy:

- Default answer: short, to the point, Bengali.
- Main goal: find screen buttons/icons/text/targets for phone control.
- Detailed answer only when the user asks to read/explain/list details.
- Maximum generation loop is larger, but the prompt asks the model to stay within about 300 words and finish naturally, not mid-sentence.
- User-facing answer is shown in `USER_FINAL_ANSWER_ONLY=`.

Expected:

```text
ANALYZE_STAGE_5T_SMART_RESPONSE_POLICY_PROBE
GENERATION_MODE=GREEDY_SMART_SCREEN_RESPONSE_POLICY
MAX_GENERATION_TOKENS=360
USER_FINAL_ANSWER_ONLY=...
STAGE5T_SMART_RESPONSE_POLICY_PROBE_OK ✅
```
