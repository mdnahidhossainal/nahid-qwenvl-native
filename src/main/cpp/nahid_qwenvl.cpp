#include <jni.h>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <android/log.h>
#include "llama.h"

#define LOG_TAG "NahidQwenVL"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static bool file_exists(const std::string & path) {
    struct stat st{};
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

static long long file_size(const std::string & path) {
    struct stat st{};
    if (stat(path.c_str(), &st) != 0) return -1;
    return static_cast<long long>(st.st_size);
}

static std::string jstr(JNIEnv * env, jstring s) {
    if (!s) return "";
    const char * c = env->GetStringUTFChars(s, nullptr);
    std::string out = c ? c : "";
    if (c) env->ReleaseStringUTFChars(s, c);
    return out;
}

static jstring to_jstring(JNIEnv * env, const std::string & s) {
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativePing(JNIEnv * env, jobject /*thiz*/) {
    std::ostringstream out;
    out << "PONG_STAGE_5F_MODEL_LOAD_PROBE" << "\n";
    out << "llama.cpp linked: yes" << "\n";
    out << "Purpose: verify real GGUF model load path before multimodal image inference.";
    return to_jstring(env, out.str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeInit(
        JNIEnv * env,
        jobject /*thiz*/,
        jstring mainModelPath,
        jstring mmprojPath) {

    const std::string main_path = jstr(env, mainModelPath);
    const std::string mmproj_path = jstr(env, mmprojPath);

    std::ostringstream out;
    out << "INIT_STAGE_5F_MODEL_LOAD_PROBE" << "\n";
    out << "MAIN=" << main_path << "\n";
    out << "MAIN_EXISTS=" << (file_exists(main_path) ? "true" : "false") << "\n";
    out << "MAIN_SIZE=" << file_size(main_path) << "\n";
    out << "MMPROJ=" << mmproj_path << "\n";
    out << "MMPROJ_EXISTS=" << (file_exists(mmproj_path) ? "true" : "false") << "\n";
    out << "MMPROJ_SIZE=" << file_size(mmproj_path) << "\n\n";

    if (!file_exists(main_path)) {
        out << "MODEL_LOAD_SKIPPED: main GGUF file not found";
        return to_jstring(env, out.str());
    }

    if (!file_exists(mmproj_path)) {
        out << "WARNING: mmproj file not found. Text model-load probe can continue, but image inference will need mmproj." << "\n";
    }

    out << "Calling llama_backend_init..." << "\n";
    llama_backend_init();

    llama_model_params params = llama_model_default_params();
    params.n_gpu_layers = 0; // CPU-safe Android probe

    out << "Calling llama_model_load_from_file..." << "\n";
    llama_model * model = llama_model_load_from_file(main_path.c_str(), params);
    if (model == nullptr) {
        out << "MODEL_LOAD_FAILED ❌" << "\n";
        out << "Meaning: libllama is loaded, but this GGUF could not be opened as a llama.cpp model on Android." << "\n";
        out << "Next fix may need a newer llama.cpp revision or a different GGUF export." << "\n";
        llama_backend_free();
        return to_jstring(env, out.str());
    }

    out << "MODEL_LOAD_OK ✅" << "\n";
    out << "The main GGUF can be loaded by llama.cpp on Android." << "\n";
    out << "This stage intentionally frees the model immediately to avoid keeping RAM busy." << "\n";
    out << "NEXT: Stage 5G will keep a context and add prompt generation; Stage 5H will attach image/mmproj." << "\n";

    llama_model_free(model);
    llama_backend_free();
    return to_jstring(env, out.str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeAnalyze(
        JNIEnv * env,
        jobject /*thiz*/,
        jstring imagePath,
        jstring prompt) {
    const std::string image_path = jstr(env, imagePath);
    const std::string prompt_text = jstr(env, prompt);

    std::ostringstream out;
    out << "ANALYZE_STAGE_5F_NOT_REAL_IMAGE_INFERENCE_YET" << "\n";
    out << "IMAGE=" << image_path << "\n";
    out << "IMAGE_EXISTS=" << (file_exists(image_path) ? "true" : "false") << "\n";
    out << "IMAGE_SIZE=" << file_size(image_path) << "\n";
    out << "PROMPT_PREVIEW=" << prompt_text.substr(0, 220) << "\n\n";
    out << "Stage 5F result: native bridge + llama.cpp model-load path is being tested in nativeInit." << "\n";
    out << "Real screenshot understanding is not enabled in this .so yet.";
    return to_jstring(env, out.str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeRelease(JNIEnv * /*env*/, jobject /*thiz*/) {
    // Stage 5F does not keep a persistent model/context yet.
}
