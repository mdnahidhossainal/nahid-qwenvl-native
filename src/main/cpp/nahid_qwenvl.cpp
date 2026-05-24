#include <jni.h>
#include <string>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <dlfcn.h>
#include <android/log.h>

#define LOG_TAG "NahidQwenVL"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static std::string g_main_model_path;
static std::string g_mmproj_path;
static bool g_initialized = false;

static std::string jstring_to_std(JNIEnv *env, jstring value) {
    if (value == nullptr) return std::string();
    const char *chars = env->GetStringUTFChars(value, nullptr);
    std::string out = chars ? chars : "";
    if (chars) env->ReleaseStringUTFChars(value, chars);
    return out;
}

static jstring make_jstring(JNIEnv *env, const std::string &value) {
    return env->NewStringUTF(value.c_str());
}

static bool file_exists(const std::string &path) {
    struct stat st{};
    return !path.empty() && stat(path.c_str(), &st) == 0;
}

static long long file_size(const std::string &path) {
    struct stat st{};
    if (path.empty() || stat(path.c_str(), &st) != 0) return -1;
    return static_cast<long long>(st.st_size);
}

static std::string safe_dlerror() {
    const char *err = dlerror();
    return err ? std::string(err) : std::string("unknown");
}

static void *safe_dlopen(const char *name, std::ostringstream &out) {
    dlerror();
    void *h = dlopen(name, RTLD_NOW | RTLD_LOCAL);
    if (h) {
        out << "DL_OPEN_OK: " << name << "\n";
    } else {
        out << "DL_OPEN_FAIL: " << name << " :: " << safe_dlerror() << "\n";
    }
    return h;
}

static bool check_symbol(void *handle, const char *symbol, std::ostringstream &out) {
    if (!handle) {
        out << symbol << ": SKIPPED_HANDLE_NULL\n";
        return false;
    }
    dlerror();
    void *ptr = dlsym(handle, symbol);
    const char *err = dlerror();
    if (ptr && !err) {
        out << symbol << ": FOUND\n";
        return true;
    }
    out << symbol << ": NOT_FOUND";
    if (err) out << " :: " << err;
    out << "\n";
    return false;
}

static std::string mtmd_handle_probe_report() {
    std::ostringstream out;
    out << "MTMD HANDLE-BASED SYMBOL PROBE — Stage 5J\n";
    out << "This stage uses dlopen(\"libmtmd.so\") handle directly, not RTLD_DEFAULT.\n";
    out << "No mtmd init, no image encode, no mmproj inference call is made.\n\n";

    void *ggml = safe_dlopen("libggml.so", out);
    void *ggml_base = safe_dlopen("libggml-base.so", out);
    void *ggml_cpu = safe_dlopen("libggml-cpu.so", out);
    void *llama = safe_dlopen("libllama.so", out);
    void *mtmd = safe_dlopen("libmtmd.so", out);

    out << "\nlibllama symbol checks:\n";
    int llama_found = 0;
    const char *llama_symbols[] = {
        "llama_backend_init",
        "llama_model_load_from_file",
        "llama_model_free",
        "llama_init_from_model",
        "llama_free",
        "llama_tokenize",
        "llama_decode"
    };
    for (const char *s : llama_symbols) {
        if (check_symbol(llama, s, out)) llama_found++;
    }

    out << "\nlibmtmd required symbol checks:\n";
    int mtmd_required_found = 0;
    const char *mtmd_required[] = {
        "mtmd_context_params_default",
        "mtmd_init_from_file",
        "mtmd_free",
        "mtmd_tokenize",
        "mtmd_encode",
        "mtmd_helper_bitmap_init_from_file",
        "mtmd_helper_eval_chunks",
        "mtmd_bitmap_free",
        "mtmd_input_chunks_init",
        "mtmd_input_chunks_size",
        "mtmd_input_chunks_get",
        "mtmd_input_chunks_free"
    };
    const int mtmd_required_total = sizeof(mtmd_required) / sizeof(mtmd_required[0]);
    for (const char *s : mtmd_required) {
        if (check_symbol(mtmd, s, out)) mtmd_required_found++;
    }

    out << "\nlibmtmd extra useful symbol checks:\n";
    const char *mtmd_extra[] = {
        "mtmd_support_vision",
        "mtmd_support_audio",
        "mtmd_get_output_embd",
        "mtmd_encode_chunk",
        "mtmd_helper_get_n_tokens",
        "mtmd_helper_get_n_pos",
        "mtmd_helper_image_get_decoder_pos",
        "mtmd_helper_decode_image_chunk",
        "mtmd_helper_eval_chunk_single",
        "mtmd_helper_bitmap_init_from_buf",
        "mtmd_bitmap_get_nx",
        "mtmd_bitmap_get_ny",
        "mtmd_bitmap_get_n_bytes"
    };
    for (const char *s : mtmd_extra) {
        check_symbol(mtmd, s, out);
    }

    out << "\nProbe summary:\n";
    out << "llama symbols found: " << llama_found << "/" << (sizeof(llama_symbols) / sizeof(llama_symbols[0])) << "\n";
    out << "mtmd required symbols found: " << mtmd_required_found << "/" << mtmd_required_total << "\n";

    if (mtmd && mtmd_required_found == mtmd_required_total) {
        out << "MTMD_HANDLE_SYMBOL_PROBE_OK ✅\n";
        out << "NEXT_SAFE_STAGE: call mtmd_context_params_default + mtmd_init_from_file only, with crash guard logs.\n";
    } else if (mtmd) {
        out << "MTMD_HANDLE_SYMBOL_PROBE_PARTIAL ⚠️\n";
        out << "libmtmd.so opened, but not all expected API symbols were found through handle dlsym.\n";
    } else {
        out << "MTMD_HANDLE_SYMBOL_PROBE_FAIL ❌\n";
        out << "libmtmd.so could not be opened from Android runtime.\n";
    }

    // Keep handles open. Closing can be risky if dependent libraries are still referenced by the runtime.
    (void)ggml;
    (void)ggml_base;
    (void)ggml_cpu;
    return out.str();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativePing(JNIEnv *env, jobject /*thiz*/) {
    LOGI("nativePing Stage 5J called");
    return make_jstring(env, "PONG_STAGE_5J: libnahid_qwenvl.so loaded. Handle-based mtmd symbol probe is available.");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeInit(
        JNIEnv *env,
        jobject /*thiz*/,
        jstring mainModelPath,
        jstring mmprojPath
) {
    g_main_model_path = jstring_to_std(env, mainModelPath);
    g_mmproj_path = jstring_to_std(env, mmprojPath);
    g_initialized = !g_main_model_path.empty() && !g_mmproj_path.empty();

    std::ostringstream out;
    out << "INIT_STAGE_5J_HANDLE_BASED_MTMD_SYMBOL_PROBE\n";
    out << "MAIN=" << g_main_model_path << "\n";
    out << "MAIN_EXISTS=" << (file_exists(g_main_model_path) ? "true" : "false") << "\n";
    out << "MAIN_SIZE=" << file_size(g_main_model_path) << "\n";
    out << "MMPROJ=" << g_mmproj_path << "\n";
    out << "MMPROJ_EXISTS=" << (file_exists(g_mmproj_path) ? "true" : "false") << "\n";
    out << "MMPROJ_SIZE=" << file_size(g_mmproj_path) << "\n\n";
    out << mtmd_handle_probe_report();
    out << "\nNOTE: Stage 5J does NOT run real screenshot/mmproj inference. It only proves Android runtime can find mtmd API symbols safely.\n";

    std::string result = out.str();
    LOGI("%s", result.c_str());
    return make_jstring(env, result);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeAnalyze(
        JNIEnv *env,
        jobject /*thiz*/,
        jstring imagePath,
        jstring prompt
) {
    std::string image = jstring_to_std(env, imagePath);
    std::string p = jstring_to_std(env, prompt);

    std::ostringstream out;
    out << "ANALYZE_STAGE_5J_HANDLE_SYMBOL_PROBE_PLACEHOLDER\n";
    out << "IMAGE=" << image << "\n";
    out << "IMAGE_EXISTS=" << (file_exists(image) ? "true" : "false") << "\n";
    out << "IMAGE_SIZE=" << file_size(image) << "\n";
    out << "MODEL_INITIALIZED_PATHS=" << (g_initialized ? "true" : "false") << "\n";
    out << "PROMPT_PREVIEW=" << p.substr(0, 220) << "\n\n";
    out << "No real image inference is called in nativeAnalyze for Stage 5J.\n";
    out << "If Init shows MTMD_HANDLE_SYMBOL_PROBE_OK, the next stage can safely try mtmd_init_from_file.\n";

    std::string result = out.str();
    LOGI("%s", result.c_str());
    return make_jstring(env, result);
}

extern "C" JNIEXPORT void JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeRelease(JNIEnv * /*env*/, jobject /*thiz*/) {
    LOGI("nativeRelease Stage 5J called");
    g_main_model_path.clear();
    g_mmproj_path.clear();
    g_initialized = false;
}
