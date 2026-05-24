#include <jni.h>
#include <string>
#include <sstream>

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativePing(JNIEnv *env, jobject /* thiz */) {
    std::string msg = "PONG: libnahid_qwenvl.so loaded successfully";
    return env->NewStringUTF(msg.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeInit(
        JNIEnv *env,
        jobject /* thiz */,
        jstring mainModelPath,
        jstring mmprojPath) {
    const char *mainPath = env->GetStringUTFChars(mainModelPath, nullptr);
    const char *projPath = env->GetStringUTFChars(mmprojPath, nullptr);

    std::ostringstream out;
    out << "INIT_OK: native skeleton received model paths\n";
    out << "MAIN=" << (mainPath ? mainPath : "") << "\n";
    out << "MMPROJ=" << (projPath ? projPath : "") << "\n";
    out << "NOTE: real Qwen-VL inference is not compiled into this skeleton yet.";

    env->ReleaseStringUTFChars(mainModelPath, mainPath);
    env->ReleaseStringUTFChars(mmprojPath, projPath);
    std::string msg = out.str();
    return env->NewStringUTF(msg.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeAnalyze(
        JNIEnv *env,
        jobject /* thiz */,
        jstring imagePath,
        jstring prompt) {
    const char *img = env->GetStringUTFChars(imagePath, nullptr);
    const char *pr = env->GetStringUTFChars(prompt, nullptr);

    std::ostringstream out;
    out << "ANALYZE_STUB_OK: native library is callable\n";
    out << "IMAGE=" << (img ? img : "") << "\n";
    out << "PROMPT_PREVIEW=";
    if (pr) {
        std::string p(pr);
        out << p.substr(0, 180);
    }
    out << "\nNOTE: real llama.cpp multimodal inference will be attached in the next stage.";

    env->ReleaseStringUTFChars(imagePath, img);
    env->ReleaseStringUTFChars(prompt, pr);
    std::string msg = out.str();
    return env->NewStringUTF(msg.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeRelease(JNIEnv * /* env */, jobject /* thiz */) {
    // No resources in skeleton build.
}
