#include <jni.h>
#include <string>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <dlfcn.h>
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

static std::string dl_probe(const char * lib, const std::vector<const char *> & symbols) {
    std::ostringstream out;
    void * h = dlopen(lib, RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        out << "LOAD_FAIL: " << lib << " :: " << (dlerror() ? dlerror() : "unknown") << "\n";
        return out.str();
    }
    out << "LOAD_OK: " << lib << "\n";
    for (const char * sym : symbols) {
        dlerror();
        void * p = dlsym(h, sym);
        out << sym << ": " << (p ? "FOUND" : "NOT_FOUND") << "\n";
    }
    dlclose(h);
    return out.str();
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
    out << "PONG_STAGE_5H_MULTIMODAL_LIBRARY_PROBE" << "\n";
    out << "Purpose: verify mtmd/llava/clip multimodal shared library packaging before real image inference.";
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
    out << "INIT_STAGE_5H_MULTIMODAL_LIBRARY_PROBE" << "\n";
    out << "MAIN=" << main_path << "\n";
    out << "MAIN_EXISTS=" << (file_exists(main_path) ? "true" : "false") << "\n";
    out << "MAIN_SIZE=" << file_size(main_path) << "\n";
    out << "MMPROJ=" << mmproj_path << "\n";
    out << "MMPROJ_EXISTS=" << (file_exists(mmproj_path) ? "true" : "false") << "\n";
    out << "MMPROJ_SIZE=" << file_size(mmproj_path) << "\n\n";

    out << "Core runtime probe:\n";
    out << dl_probe("libllama.so", {"llama_backend_init", "llama_model_load_from_file", "llama_init_from_model"});
    out << "\nMultimodal runtime probe:\n";
    out << dl_probe("libmtmd.so", {"mtmd_context_params_default", "mtmd_init_from_file", "mtmd_free", "mtmd_encode"});
    out << dl_probe("libllava.so", {"llava_validate_embed_size", "llava_image_embed_make_with_filename", "llava_image_embed_free"});
    out << dl_probe("libclip.so", {"clip_model_load", "clip_free", "clip_image_load_from_file"});

    out << "\nNow checking whether main GGUF still loads...\n";
    if (!file_exists(main_path)) {
        out << "MODEL_LOAD_SKIPPED: main GGUF not found\n";
        return to_jstring(env, out.str());
    }

    llama_backend_init();
    llama_model_params params = llama_model_default_params();
    params.n_gpu_layers = 0;
    llama_model * model = llama_model_load_from_file(main_path.c_str(), params);
    if (model == nullptr) {
        out << "MODEL_LOAD_FAILED ❌\n";
        llama_backend_free();
        return to_jstring(env, out.str());
    }
    out << "MODEL_LOAD_OK ✅\n";
    llama_model_free(model);
    llama_backend_free();

    out << "\nNOTE: Stage 5H only proves multimodal helper library availability/loadability. Real screenshot understanding is next.";
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
    const std::string model_path = "/storage/emulated/0/Android/data/com.nahidai.assistant/files/models/qwen2.5-vl-3b-ui-grounding.q4_k_s.gguf";

    std::ostringstream out;
    out << "ANALYZE_STAGE_5H_MULTIMODAL_LIBRARY_PROBE" << "\n";
    out << "IMAGE=" << image_path << "\n";
    out << "IMAGE_EXISTS=" << (file_exists(image_path) ? "true" : "false") << "\n";
    out << "IMAGE_SIZE=" << file_size(image_path) << "\n";
    out << "MODEL=" << model_path << "\n";
    out << "MODEL_EXISTS=" << (file_exists(model_path) ? "true" : "false") << "\n";
    out << "PROMPT_PREVIEW=" << prompt_text.substr(0, 180) << "\n\n";

    out << "Multimodal library runtime check:\n";
    out << dl_probe("libmtmd.so", {"mtmd_context_params_default", "mtmd_init_from_file", "mtmd_encode"});
    out << dl_probe("libllava.so", {"llava_validate_embed_size", "llava_image_embed_make_with_filename"});
    out << dl_probe("libclip.so", {"clip_model_load", "clip_image_load_from_file"});

    out << "\nTEXT_GENERATION_SANITY_CHECK:\n";
    if (!file_exists(model_path)) {
        out << "SKIPPED: model file not found\n";
        return to_jstring(env, out.str());
    }

    llama_backend_init();
    llama_model_params mparams = llama_model_default_params();
    mparams.n_gpu_layers = 0;
    llama_model * model = llama_model_load_from_file(model_path.c_str(), mparams);
    if (model == nullptr) {
        out << "MODEL_LOAD_FAILED_IN_ANALYZE ❌\n";
        llama_backend_free();
        return to_jstring(env, out.str());
    }

    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = 384;
    cparams.n_batch = 96;
    cparams.n_threads = 4;
    cparams.n_threads_batch = 4;
    llama_context * ctx = llama_init_from_model(model, cparams);
    if (ctx == nullptr) {
        out << "CONTEXT_CREATE_FAILED_IN_ANALYZE ❌\n";
        llama_model_free(model);
        llama_backend_free();
        return to_jstring(env, out.str());
    }

    const llama_vocab * vocab = llama_model_get_vocab(model);
    const std::string text_prompt = "<|im_start|>user\nSay OK in one word.<|im_end|>\n<|im_start|>assistant\n";
    std::vector<llama_token> tokens(text_prompt.size() + 16);
    int n_tokens = llama_tokenize(vocab, text_prompt.c_str(), (int) text_prompt.size(), tokens.data(), (int) tokens.size(), true, true);
    if (n_tokens < 0) {
        tokens.resize((size_t)(-n_tokens));
        n_tokens = llama_tokenize(vocab, text_prompt.c_str(), (int) text_prompt.size(), tokens.data(), (int) tokens.size(), true, true);
    }
    if (n_tokens <= 0) {
        out << "TOKENIZE_FAILED ❌\n";
        llama_free(ctx);
        llama_model_free(model);
        llama_backend_free();
        return to_jstring(env, out.str());
    }
    tokens.resize((size_t)n_tokens);
    llama_batch batch = llama_batch_get_one(tokens.data(), n_tokens);
    if (llama_decode(ctx, batch) != 0) {
        out << "PROMPT_DECODE_FAILED ❌\n";
        llama_free(ctx);
        llama_model_free(model);
        llama_backend_free();
        return to_jstring(env, out.str());
    }

    llama_sampler * smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.1f));
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(5678));
    std::string generated;
    int generated_count = 0;
    for (int i = 0; i < 8; ++i) {
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

    out << "TEXT_GENERATION_OK ✅\n";
    out << "GENERATED_TOKENS=" << generated_count << "\n";
    out << "TEXT_OUTPUT=" << generated << "\n\n";
    out << "NOTE: Stage 5H is not real image inference yet. If libmtmd.so LOAD_OK appears, Stage 5I can attach image+mmproj.";
    return to_jstring(env, out.str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeRelease(JNIEnv * /*env*/, jobject /*thiz*/) {
    // Probe stage: no persistent model retained.
}
