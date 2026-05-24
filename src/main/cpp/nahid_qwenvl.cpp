#include <jni.h>
#include <string>
#include <sstream>
#include <android/log.h>
#include <dlfcn.h>
#include <sys/stat.h>

#define LOG_TAG "NahidQwenVL"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static std::string g_main_model_path;
static std::string g_mmproj_path;

static bool file_exists(const std::string& path) {
    struct stat st{};
    return !path.empty() && stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

static std::string jstr_to_utf8(JNIEnv* env, jstring value) {
    if (!value) return "";
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (!chars) return "";
    std::string out(chars);
    env->ReleaseStringUTFChars(value, chars);
    return out;
}

static jstring to_jstring(JNIEnv* env, const std::string& value) {
    return env->NewStringUTF(value.c_str());
}

static std::string probe_shared_lib(const char* lib_name) {
    void* handle = dlopen(lib_name, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        const char* err = dlerror();
        std::ostringstream ss;
        ss << "LOAD_FAIL: " << lib_name << " -> " << (err ? err : "unknown dlopen error");
        return ss.str();
    }

    // We only probe symbols here. Real inference will be connected in the next stage.
    void* backend_init = dlsym(handle, "llama_backend_init");
    void* model_load = dlsym(handle, "llama_model_load_from_file");

    std::ostringstream ss;
    ss << "LOAD_OK: " << lib_name;
    ss << "\nllama_backend_init symbol: " << (backend_init ? "FOUND" : "NOT_FOUND");
    ss << "\nllama_model_load_from_file symbol: " << (model_load ? "FOUND" : "NOT_FOUND");

    dlclose(handle);
    return ss.str();
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativePing(
        JNIEnv* env,
        jobject /* thiz */) {
    std::string result =
            "PONG_OK: libnahid_qwenvl.so loaded and JNI callable.\n"
            "Stage: 5E-B llama.cpp shared-library probe wrapper.\n"
            "Note: this is not final Qwen-VL inference yet.";
    return to_jstring(env, result);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeInit(
        JNIEnv* env,
        jobject /* thiz */,
        jstring mainModelPath,
        jstring mmprojPath) {

    g_main_model_path = jstr_to_utf8(env, mainModelPath);
    g_mmproj_path = jstr_to_utf8(env, mmprojPath);

    std::ostringstream ss;
    ss << "INIT_OK: Stage 5E-B native wrapper initialized\n";
    ss << "MAIN=" << g_main_model_path << "\n";
    ss << "MAIN_EXISTS=" << (file_exists(g_main_model_path) ? "true" : "false") << "\n";
    ss << "MMPROJ=" << g_mmproj_path << "\n";
    ss << "MMPROJ_EXISTS=" << (file_exists(g_mmproj_path) ? "true" : "false") << "\n\n";

    ss << "llama.cpp shared library probe:\n";
    ss << probe_shared_lib("libllama.so") << "\n\n";

    ss << "ggml shared library probe:\n";
    ss << probe_shared_lib("libggml.so") << "\n\n";

    ss << "NOTE: If libllama.so LOAD_OK appears, app packaging can load llama.cpp native backend.\n";
    ss << "NEXT: Stage 5E-C will replace this probe with real model load + image prompt inference.";
    LOGI("%s", ss.str().c_str());
    return to_jstring(env, ss.str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeAnalyze(
        JNIEnv* env,
        jobject /* thiz */,
        jstring imagePath,
        jstring prompt) {

    std::string img = jstr_to_utf8(env, imagePath);
    std::string prm = jstr_to_utf8(env, prompt);

    std::ostringstream ss;
    ss << "ANALYZE_PROBE_OK: JNI -> prebuilt wrapper -> llama.cpp probe path working\n";
    ss << "IMAGE=" << img << "\n";
    ss << "IMAGE_EXISTS=" << (file_exists(img) ? "true" : "false") << "\n";
    ss << "PROMPT_PREVIEW=" << prm.substr(0, 240) << "\n\n";

    ss << "Runtime library check:\n";
    ss << probe_shared_lib("libllama.so") << "\n\n";

    ss << "NOTE: This Stage 5E-B only verifies that llama.cpp shared libs are packaged and loadable.\n";
    ss << "Real Qwen2.5-VL image understanding will be added after this test passes.";
    LOGI("%s", ss.str().c_str());
    return to_jstring(env, ss.str());
}
