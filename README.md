# nahid-qwenvl-native

GitHub Actions build package for Nahid AI native backend smoke-test library.

This first stage builds a small ARM64 Android shared library named `libnahid_qwenvl.so`.
It does not run Qwen-VL inference yet. It verifies that the Nahid AI Android app can load a prebuilt native library.

Output artifact:
- `libnahid_qwenvl_arm64_v8a.zip`
  - contains `libnahid_qwenvl.so`
