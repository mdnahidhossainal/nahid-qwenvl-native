#include <jni.h>
#include <string>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <dlfcn.h>
#include <android/log.h>
#include "llama.h"
#include "mtmd.h"

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
    return !path.empty() && stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
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

template <typename Fn>
static Fn load_fn(void *handle, const char *symbol, std::ostringstream &out, bool required = true) {
    if (!handle) {
        out << symbol << ": SKIPPED_HANDLE_NULL\n";
        return nullptr;
    }
    dlerror();
    void *ptr = dlsym(handle, symbol);
    const char *err = dlerror();
    if (ptr && !err) {
        out << symbol << ": FOUND\n";
        return reinterpret_cast<Fn>(ptr);
    }
    out << symbol << ": NOT_FOUND";
    if (err) out << " :: " << err;
    if (required) out << "  ❌";
    out << "\n";
    return nullptr;
}

static std::string mtmd_init_from_file_probe() {
    std::ostringstream out;
    out << "MTMD INIT-FROM-FILE PROBE — Stage 5L-B\n";
    out << "This stage loads main GGUF, then calls only mtmd_context_params_default + mtmd_init_from_file.\n";
    out << "No screenshot bitmap, no mtmd_encode, no image inference is called.\n\n";

    out << "File checks:\n";
    out << "MAIN_EXISTS=" << (file_exists(g_main_model_path) ? "true" : "false") << "\n";
    out << "MAIN_SIZE=" << file_size(g_main_model_path) << "\n";
    out << "MMPROJ_EXISTS=" << (file_exists(g_mmproj_path) ? "true" : "false") << "\n";
    out << "MMPROJ_SIZE=" << file_size(g_mmproj_path) << "\n\n";

    if (!file_exists(g_main_model_path)) {
        out << "STAGE5L_STOP: main GGUF not found ❌\n";
        return out.str();
    }
    if (!file_exists(g_mmproj_path)) {
        out << "STAGE5L_STOP: mmproj GGUF not found ❌\n";
        return out.str();
    }

    void *ggml = safe_dlopen("libggml.so", out);
    void *ggml_base = safe_dlopen("libggml-base.so", out);
    void *ggml_cpu = safe_dlopen("libggml-cpu.so", out);
    void *llama_h = safe_dlopen("libllama.so", out);
    void *mtmd_h = safe_dlopen("libmtmd.so", out);
    (void)ggml; (void)ggml_base; (void)ggml_cpu;

    out << "\nRequired function pointers:\n";
    auto p_llama_backend_init = load_fn<decltype(&llama_backend_init)>(llama_h, "llama_backend_init", out);
    auto p_llama_backend_free = load_fn<decltype(&llama_backend_free)>(llama_h, "llama_backend_free", out);
    auto p_llama_model_default_params = load_fn<decltype(&llama_model_default_params)>(llama_h, "llama_model_default_params", out);
    auto p_llama_model_load_from_file = load_fn<decltype(&llama_model_load_from_file)>(llama_h, "llama_model_load_from_file", out);
    auto p_llama_model_free = load_fn<decltype(&llama_model_free)>(llama_h, "llama_model_free", out);

    auto p_mtmd_context_params_default = load_fn<decltype(&mtmd_context_params_default)>(mtmd_h, "mtmd_context_params_default", out);
    auto p_mtmd_init_from_file = load_fn<decltype(&mtmd_init_from_file)>(mtmd_h, "mtmd_init_from_file", out);
    auto p_mtmd_free = load_fn<decltype(&mtmd_free)>(mtmd_h, "mtmd_free", out);
    auto p_mtmd_support_vision = load_fn<decltype(&mtmd_support_vision)>(mtmd_h, "mtmd_support_vision", out, false);

    if (!p_llama_backend_init || !p_llama_backend_free || !p_llama_model_default_params ||
        !p_llama_model_load_from_file || !p_llama_model_free ||
        !p_mtmd_context_params_default || !p_mtmd_init_from_file || !p_mtmd_free) {
        out << "\nSTAGE5L_STOP: required function pointer missing ❌\n";
        return out.str();
    }

    out << "\nCalling llama_backend_init...\n";
    p_llama_backend_init();

    llama_model_params model_params = p_llama_model_default_params();
    model_params.n_gpu_layers = 0;

    out << "Calling llama_model_load_from_file...\n";
    llama_model *model = p_llama_model_load_from_file(g_main_model_path.c_str(), model_params);
    if (!model) {
        out << "MODEL_LOAD_FAILED ❌\n";
        p_llama_backend_free();
        return out.str();
    }
    out << "MODEL_LOAD_OK ✅\n";

    out << "Calling mtmd_context_params_default...\n";
    mtmd_context_params mtmd_params = p_mtmd_context_params_default();

    out << "Calling mtmd_init_from_file with mmproj + loaded llama_model...\n";
    mtmd_context *mctx = p_mtmd_init_from_file(g_mmproj_path.c_str(), model, mtmd_params);
    if (!mctx) {
        out << "MTMD_INIT_FROM_FILE_FAILED ❌\n";
        out << "Meaning: libmtmd is callable, but projector init failed for this mmproj/model combo or runtime config.\n";
        p_llama_model_free(model);
        p_llama_backend_free();
        return out.str();
    }

    out << "MTMD_INIT_FROM_FILE_OK ✅\n";
    if (p_mtmd_support_vision) {
        bool vision = p_mtmd_support_vision(mctx);
        out << "MTMD_SUPPORT_VISION=" << (vision ? "true ✅" : "false ⚠️") << "\n";
    }

    out << "Freeing mtmd context and llama model immediately to keep RAM safe...\n";
    p_mtmd_free(mctx);
    p_llama_model_free(model);
    p_llama_backend_free();

    out << "STAGE5L_MMPROJ_INIT_PROBE_OK ✅\n";
    out << "NEXT_SAFE_STAGE: load screenshot bitmap with mtmd_helper_bitmap_init_from_file, then tokenize/evaluate chunks.\n";
    return out.str();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativePing(JNIEnv *env, jobject /*thiz*/) {
    LOGI("nativePing Stage 5L-B called");
    return make_jstring(env, "PONG_STAGE_5L_B: bitmap load compile-fix probe is available.");
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
    out << "INIT_STAGE_5L_B_BITMAP_LOAD_COMPILE_FIX\n";
    out << "MAIN=" << g_main_model_path << "\n";
    out << "MMPROJ=" << g_mmproj_path << "\n\n";
    out << mtmd_init_from_file_probe();
    out << "\nNOTE: Stage 5L-B does NOT run screenshot understanding yet. It only verifies mmproj projector init through libmtmd.\n";

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
    out << "ANALYZE_STAGE_5L_B_BITMAP_LOAD_COMPILE_FIX\n";
    out << "This stage loads the saved screenshot file using mtmd_helper_bitmap_init_from_file.\n";
    out << "No mtmd_encode, no token evaluation, no real screenshot understanding is called.\n\n";
    out << "IMAGE=" << image << "\n";
    out << "IMAGE_EXISTS=" << (file_exists(image) ? "true" : "false") << "\n";
    out << "IMAGE_SIZE=" << file_size(image) << "\n";
    out << "MODEL_INITIALIZED_PATHS=" << (g_initialized ? "true" : "false") << "\n";
    out << "PROMPT_PREVIEW=" << p.substr(0, 220) << "\n\n";

    if (!file_exists(image)) {
        out << "STAGE5L_STOP: screenshot image file not found ❌\n";
        std::string result = out.str();
        LOGI("%s", result.c_str());
        return make_jstring(env, result);
    }

    void *ggml = safe_dlopen("libggml.so", out);
    void *ggml_base = safe_dlopen("libggml-base.so", out);
    void *ggml_cpu = safe_dlopen("libggml-cpu.so", out);
    void *llama_h = safe_dlopen("libllama.so", out);
    void *mtmd_h = safe_dlopen("libmtmd.so", out);
    (void)ggml; (void)ggml_base; (void)ggml_cpu; (void)llama_h;

    out << "\nBitmap function pointers (manual ABI-safe typedefs):\n";
    // The current llama.cpp mtmd API returns an int status and writes the bitmap through an out pointer.
    // We avoid decltype(&mtmd_helper_bitmap_init_from_file) here because some header revisions differ,
    // which caused the Stage 5L wrapper compile error in GitHub Actions.
    using mtmd_helper_bitmap_init_from_file_fn = int32_t (*)(mtmd_bitmap **, const char *);
    using mtmd_bitmap_free_fn = void (*)(mtmd_bitmap *);
    using mtmd_bitmap_get_u32_fn = uint32_t (*)(const mtmd_bitmap *);
    using mtmd_bitmap_get_size_fn = size_t (*)(const mtmd_bitmap *);

    auto p_mtmd_helper_bitmap_init_from_file = load_fn<mtmd_helper_bitmap_init_from_file_fn>(mtmd_h, "mtmd_helper_bitmap_init_from_file", out);
    auto p_mtmd_bitmap_free = load_fn<mtmd_bitmap_free_fn>(mtmd_h, "mtmd_bitmap_free", out);
    auto p_mtmd_bitmap_get_nx = load_fn<mtmd_bitmap_get_u32_fn>(mtmd_h, "mtmd_bitmap_get_nx", out, false);
    auto p_mtmd_bitmap_get_ny = load_fn<mtmd_bitmap_get_u32_fn>(mtmd_h, "mtmd_bitmap_get_ny", out, false);
    auto p_mtmd_bitmap_get_n_bytes = load_fn<mtmd_bitmap_get_size_fn>(mtmd_h, "mtmd_bitmap_get_n_bytes", out, false);

    if (!p_mtmd_helper_bitmap_init_from_file || !p_mtmd_bitmap_free) {
        out << "\nSTAGE5L_B_STOP: required bitmap function pointer missing ❌\n";
        std::string result = out.str();
        LOGI("%s", result.c_str());
        return make_jstring(env, result);
    }

    out << "\nCalling mtmd_helper_bitmap_init_from_file(&bitmap, saved_screenshot)...\n";
    mtmd_bitmap *bitmap = nullptr;
    int32_t bitmap_rc = p_mtmd_helper_bitmap_init_from_file(&bitmap, image.c_str());
    out << "BITMAP_LOAD_RC=" << bitmap_rc << "\n";
    if (bitmap_rc != 0 || !bitmap) {
        out << "BITMAP_LOAD_FAILED ❌\n";
        out << "Meaning: libmtmd is callable, but it could not decode this screenshot file or returned null.\n";
        std::string result = out.str();
        LOGI("%s", result.c_str());
        return make_jstring(env, result);
    }

    out << "BITMAP_LOAD_OK ✅\n";
    if (p_mtmd_bitmap_get_nx && p_mtmd_bitmap_get_ny) {
        out << "BITMAP_WIDTH=" << p_mtmd_bitmap_get_nx(bitmap) << "\n";
        out << "BITMAP_HEIGHT=" << p_mtmd_bitmap_get_ny(bitmap) << "\n";
    }
    if (p_mtmd_bitmap_get_n_bytes) {
        out << "BITMAP_BYTES=" << p_mtmd_bitmap_get_n_bytes(bitmap) << "\n";
    }

    out << "Freeing bitmap immediately to keep RAM safe...\n";
    p_mtmd_bitmap_free(bitmap);

    out << "STAGE5L_B_BITMAP_LOAD_PROBE_OK ✅\n";
    out << "NEXT_SAFE_STAGE: create mtmd input chunks from prompt + bitmap, without generation yet.\n";

    std::string result = out.str();
    LOGI("%s", result.c_str());
    return make_jstring(env, result);
}

extern "C" JNIEXPORT void JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeRelease(JNIEnv * /*env*/, jobject /*thiz*/) {
    LOGI("nativeRelease Stage 5L-B called");
    g_main_model_path.clear();
    g_mmproj_path.clear();
    g_initialized = false;
}
