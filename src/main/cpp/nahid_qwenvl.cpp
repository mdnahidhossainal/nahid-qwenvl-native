#include <jni.h>
#include <string>
#include <sstream>
#include <dlfcn.h>
#include <android/log.h>
#include <sys/stat.h>

#define LOG_TAG "NahidQwenVL"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static bool file_exists(const char* path) {
    struct stat st{};
    return path && stat(path, &st) == 0;
}

static long file_size(const char* path) {
    struct stat st{};
    if (!path || stat(path, &st) != 0) return -1;
    return (long) st.st_size;
}

static std::string load_lib(const char* name) {
    void* h = dlopen(name, RTLD_NOW | RTLD_GLOBAL);
    if (!h) {
        std::string e = "LOAD_FAIL: ";
        e += name;
        e += " -> ";
        e += dlerror() ? dlerror() : "unknown";
        return e;
    }
    return std::string("LOAD_OK: ") + name;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativePing(JNIEnv* env, jobject) {
    std::ostringstream out;
    out << "PONG: Stage 5I wrapper loaded\n";
    out << load_lib("libggml.so") << "\n";
    out << load_lib("libggml-base.so") << "\n";
    out << load_lib("libggml-cpu.so") << "\n";
    out << load_lib("libllama.so") << "\n";
    out << load_lib("libmtmd.so") << "\n";
    return env->NewStringUTF(out.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeInit(
        JNIEnv* env, jobject, jstring mainPath, jstring mmprojPath) {
    const char* main_c = env->GetStringUTFChars(mainPath, nullptr);
    const char* mmproj_c = env->GetStringUTFChars(mmprojPath, nullptr);

    std::ostringstream out;
    out << "INIT_STAGE_5I_MTMD_SYMBOL_DUMP_BUILD\n";
    out << "MAIN=" << main_c << "\n";
    out << "MAIN_EXISTS=" << (file_exists(main_c) ? "true" : "false") << "\n";
    out << "MAIN_SIZE=" << file_size(main_c) << "\n";
    out << "MMPROJ=" << mmproj_c << "\n";
    out << "MMPROJ_EXISTS=" << (file_exists(mmproj_c) ? "true" : "false") << "\n";
    out << "MMPROJ_SIZE=" << file_size(mmproj_c) << "\n\n";
    out << load_lib("libggml.so") << "\n";
    out << load_lib("libggml-base.so") << "\n";
    out << load_lib("libggml-cpu.so") << "\n";
    out << load_lib("libllama.so") << "\n";
    out << load_lib("libmtmd.so") << "\n";
    out << "This stage is for GitHub artifact symbol dump, not real image inference.\n";

    env->ReleaseStringUTFChars(mainPath, main_c);
    env->ReleaseStringUTFChars(mmprojPath, mmproj_c);
    return env->NewStringUTF(out.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeAnalyze(
        JNIEnv* env, jobject, jstring imagePath, jstring prompt) {
    const char* image_c = env->GetStringUTFChars(imagePath, nullptr);
    const char* prompt_c = env->GetStringUTFChars(prompt, nullptr);

    std::ostringstream out;
    out << "ANALYZE_STAGE_5I_SYMBOL_DUMP_PLACEHOLDER\n";
    out << "IMAGE=" << image_c << "\n";
    out << "IMAGE_EXISTS=" << (file_exists(image_c) ? "true" : "false") << "\n";
    out << "IMAGE_SIZE=" << file_size(image_c) << "\n";
    out << "PROMPT_PREVIEW=";
    std::string p = prompt_c ? prompt_c : "";
    out << p.substr(0, 180) << "\n\n";
    out << "MTMD_SYMBOLS_NOT_CHECKED_IN_APP.\n";
    out << "Download the GitHub Actions artifact and open mtmd_symbols_nm.txt / mtmd_symbols_readelf.txt.\n";

    env->ReleaseStringUTFChars(imagePath, image_c);
    env->ReleaseStringUTFChars(prompt, prompt_c);
    return env->NewStringUTF(out.str().c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeRelease(JNIEnv*, jobject) {
}
