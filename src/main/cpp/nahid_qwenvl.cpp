#include <jni.h>
#include <string>
#include <sstream>
#include <vector>
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

static std::string token_to_piece_safe(const llama_vocab * vocab, llama_token token) {
    std::string piece;
    piece.resize(64);
    int n = llama_token_to_piece(vocab, token, piece.data(), (int) piece.size(), 0, true);
    if (n < 0) {
        piece.resize((size_t)(-n));
        n = llama_token_to_piece(vocab, token, piece.data(), (int) piece.size(), 0, true);
    }
    if (n <= 0) return "";
    piece.resize((size_t)n);
    return piece;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativePing(JNIEnv * env, jobject /*thiz*/) {
    std::ostringstream out;
    out << "PONG_STAGE_5G_TEXT_GENERATION_PROBE" << "\n";
    out << "llama.cpp linked: yes" << "\n";
    out << "Purpose: verify GGUF load + context + token generation before multimodal image inference.";
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
    out << "INIT_STAGE_5G_TEXT_GENERATION_PROBE" << "\n";
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

    llama_backend_init();
    llama_model_params params = llama_model_default_params();
    params.n_gpu_layers = 0;

    llama_model * model = llama_model_load_from_file(main_path.c_str(), params);
    if (model == nullptr) {
        out << "MODEL_LOAD_FAILED ❌" << "\n";
        llama_backend_free();
        return to_jstring(env, out.str());
    }

    out << "MODEL_LOAD_OK ✅" << "\n";

    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = 512;
    cparams.n_batch = 128;
    cparams.n_threads = 4;
    cparams.n_threads_batch = 4;

    llama_context * ctx = llama_init_from_model(model, cparams);
    if (ctx == nullptr) {
        out << "CONTEXT_CREATE_FAILED ❌" << "\n";
        llama_model_free(model);
        llama_backend_free();
        return to_jstring(env, out.str());
    }

    out << "CONTEXT_CREATE_OK ✅" << "\n";
    out << "Stage 5G init probe passed. nativeAnalyze will run a short text generation test." << "\n";
    out << "NOTE: This still does not use the screenshot/mmproj image path.";

    llama_free(ctx);
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

    // The Kotlin bridge passes the same model paths to nativeInit only. Stage 5G needs a safe way to find the model.
    // This is derived from the known app-private model location used in previous stages.
    const std::string model_path = "/storage/emulated/0/Android/data/com.nahidai.assistant/files/models/qwen2.5-vl-3b-ui-grounding.q4_k_s.gguf";

    std::ostringstream out;
    out << "ANALYZE_STAGE_5G_TEXT_GENERATION_PROBE" << "\n";
    out << "IMAGE=" << image_path << "\n";
    out << "IMAGE_EXISTS=" << (file_exists(image_path) ? "true" : "false") << "\n";
    out << "IMAGE_SIZE=" << file_size(image_path) << "\n";
    out << "MODEL=" << model_path << "\n";
    out << "MODEL_EXISTS=" << (file_exists(model_path) ? "true" : "false") << "\n";
    out << "PROMPT_PREVIEW=" << prompt_text.substr(0, 180) << "\n\n";

    if (!file_exists(model_path)) {
        out << "TEXT_GENERATION_SKIPPED: model file not found";
        return to_jstring(env, out.str());
    }

    llama_backend_init();

    llama_model_params mparams = llama_model_default_params();
    mparams.n_gpu_layers = 0;
    llama_model * model = llama_model_load_from_file(model_path.c_str(), mparams);
    if (model == nullptr) {
        out << "MODEL_LOAD_FAILED_IN_ANALYZE ❌" << "\n";
        llama_backend_free();
        return to_jstring(env, out.str());
    }

    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = 512;
    cparams.n_batch = 128;
    cparams.n_threads = 4;
    cparams.n_threads_batch = 4;
    llama_context * ctx = llama_init_from_model(model, cparams);
    if (ctx == nullptr) {
        out << "CONTEXT_CREATE_FAILED_IN_ANALYZE ❌" << "\n";
        llama_model_free(model);
        llama_backend_free();
        return to_jstring(env, out.str());
    }

    const llama_vocab * vocab = llama_model_get_vocab(model);
    const std::string text_prompt = "<|im_start|>user\nবাংলায় এক লাইনে বলো: তুমি কি কাজ করছ?<|im_end|>\n<|im_start|>assistant\n";

    std::vector<llama_token> tokens(text_prompt.size() + 32);
    int n_tokens = llama_tokenize(vocab, text_prompt.c_str(), (int) text_prompt.size(), tokens.data(), (int) tokens.size(), true, true);
    if (n_tokens < 0) {
        tokens.resize((size_t)(-n_tokens));
        n_tokens = llama_tokenize(vocab, text_prompt.c_str(), (int) text_prompt.size(), tokens.data(), (int) tokens.size(), true, true);
    }
    if (n_tokens <= 0) {
        out << "TOKENIZE_FAILED ❌" << "\n";
        llama_free(ctx);
        llama_model_free(model);
        llama_backend_free();
        return to_jstring(env, out.str());
    }
    tokens.resize((size_t)n_tokens);

    llama_batch batch = llama_batch_get_one(tokens.data(), n_tokens);
    if (llama_decode(ctx, batch) != 0) {
        out << "PROMPT_DECODE_FAILED ❌" << "\n";
        llama_free(ctx);
        llama_model_free(model);
        llama_backend_free();
        return to_jstring(env, out.str());
    }

    llama_sampler * smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(smpl, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(smpl, llama_sampler_init_top_p(0.90f, 1));
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.35f));
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(1234));

    std::string generated;
    int generated_count = 0;
    for (int i = 0; i < 32; ++i) {
        llama_token new_token = llama_sampler_sample(smpl, ctx, -1);
        if (llama_vocab_is_eog(vocab, new_token)) break;
        generated += token_to_piece_safe(vocab, new_token);
        llama_batch next = llama_batch_get_one(&new_token, 1);
        if (llama_decode(ctx, next) != 0) break;
        generated_count++;
    }

    llama_sampler_free(smpl);
    llama_free(ctx);
    llama_model_free(model);
    llama_backend_free();

    out << "TEXT_GENERATION_OK ✅" << "\n";
    out << "GENERATED_TOKENS=" << generated_count << "\n";
    out << "TEXT_OUTPUT=" << generated << "\n\n";
    out << "NOTE: Stage 5G proves text generation. Screenshot/mmproj vision is still next stage.";
    return to_jstring(env, out.str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeRelease(JNIEnv * /*env*/, jobject /*thiz*/) {
    // Stage 5G does not keep a persistent model/context yet.
}
