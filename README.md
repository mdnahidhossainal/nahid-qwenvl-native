# Nahid QwenVL Native Build Repo — Stage 5H

Stage 5H verifies whether the GitHub-built llama.cpp package includes a multimodal helper library (`libmtmd.so`, or fallback `libllava.so`/`libclip.so`) and whether Android can load it alongside `libllama.so`.

This is still a probe stage: real screenshot/image inference comes after the multimodal library/API is confirmed.
