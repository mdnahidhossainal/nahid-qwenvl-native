#include <jni.h>
#include <string>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <dlfcn.h>
#include <android/log.h>
#include <cstring>
#include "llama.h"
#include "mtmd.h"

#define LOG_TAG "NahidQwenVL"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

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
    if (h) out << "DL_OPEN_OK: " << name << "\n";
    else out << "DL_OPEN_FAIL: " << name << " :: " << safe_dlerror() << "\n";
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

static std::string token_to_piece_safe(const llama_vocab * vocab, llama_token token) {
    std::string piece;
    piece.resize(96);
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
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativePing(JNIEnv *env, jobject /*thiz*/) {
    return make_jstring(env, "PONG_STAGE_5S: concise answer quality probe available.");
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
    out << "INIT_STAGE_5S_CONCISE_SCREEN_ANSWER_PROBE\n";
    out << "MAIN=" << g_main_model_path << "\n";
    out << "MAIN_EXISTS=" << (file_exists(g_main_model_path) ? "true" : "false") << "\n";
    out << "MAIN_SIZE=" << file_size(g_main_model_path) << "\n";
    out << "MMPROJ=" << g_mmproj_path << "\n";
    out << "MMPROJ_EXISTS=" << (file_exists(g_mmproj_path) ? "true" : "false") << "\n";
    out << "MMPROJ_SIZE=" << file_size(g_mmproj_path) << "\n";
    out << "Stage 5S init only checks paths. Concise screen-answer generation probe runs inside nativeAnalyze.\n";
    return make_jstring(env, out.str());
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
    out << "ANALYZE_STAGE_5S_CONCISE_SCREEN_ANSWER_PROBE\n";
    out << "Goal: evaluate TEXT + IMAGE chunks and generate a more useful Bengali screen summary.\n";
    out << "This attempts to improve Stage 5Q short/weak output by using a clearer screen-analysis prompt.\n\n";
    out << "IMAGE=" << image << "\n";
    out << "IMAGE_EXISTS=" << (file_exists(image) ? "true" : "false") << "\n";
    out << "IMAGE_SIZE=" << file_size(image) << "\n";
    out << "MAIN=" << g_main_model_path << "\n";
    out << "MAIN_EXISTS=" << (file_exists(g_main_model_path) ? "true" : "false") << "\n";
    out << "MMPROJ=" << g_mmproj_path << "\n";
    out << "MMPROJ_EXISTS=" << (file_exists(g_mmproj_path) ? "true" : "false") << "\n";
    out << "PROMPT_PREVIEW=" << p.substr(0, 220) << "\n\n";

    if (!file_exists(image) || !file_exists(g_main_model_path) || !file_exists(g_mmproj_path)) {
        out << "STAGE5P_STOP: required file missing ❌\n";
        return make_jstring(env, out.str());
    }

    void *ggml = safe_dlopen("libggml.so", out);
    void *ggml_base = safe_dlopen("libggml-base.so", out);
    void *ggml_cpu = safe_dlopen("libggml-cpu.so", out);
    void *llama_h = safe_dlopen("libllama.so", out);
    void *mtmd_h = safe_dlopen("libmtmd.so", out);
    (void)ggml; (void)ggml_base; (void)ggml_cpu;

    out << "\nFunction pointers (Stage 5S):\n";
    auto p_llama_backend_init = load_fn<decltype(&llama_backend_init)>(llama_h, "llama_backend_init", out);
    auto p_llama_backend_free = load_fn<decltype(&llama_backend_free)>(llama_h, "llama_backend_free", out);
    auto p_llama_model_default_params = load_fn<decltype(&llama_model_default_params)>(llama_h, "llama_model_default_params", out);
    auto p_llama_model_load_from_file = load_fn<decltype(&llama_model_load_from_file)>(llama_h, "llama_model_load_from_file", out);
    auto p_llama_model_free = load_fn<decltype(&llama_model_free)>(llama_h, "llama_model_free", out);
    auto p_llama_context_default_params = load_fn<decltype(&llama_context_default_params)>(llama_h, "llama_context_default_params", out);
    auto p_llama_init_from_model = load_fn<decltype(&llama_init_from_model)>(llama_h, "llama_init_from_model", out);
    auto p_llama_free = load_fn<decltype(&llama_free)>(llama_h, "llama_free", out);
    auto p_llama_model_get_vocab = load_fn<decltype(&llama_model_get_vocab)>(llama_h, "llama_model_get_vocab", out);
    auto p_llama_model_n_embd = load_fn<decltype(&llama_model_n_embd)>(llama_h, "llama_model_n_embd", out);
    auto p_llama_batch_init = load_fn<decltype(&llama_batch_init)>(llama_h, "llama_batch_init", out);
    auto p_llama_batch_free = load_fn<decltype(&llama_batch_free)>(llama_h, "llama_batch_free", out);
    auto p_llama_decode = load_fn<decltype(&llama_decode)>(llama_h, "llama_decode", out);
    auto p_llama_tokenize = load_fn<decltype(&llama_tokenize)>(llama_h, "llama_tokenize", out, false);

    auto p_mtmd_context_params_default = load_fn<decltype(&mtmd_context_params_default)>(mtmd_h, "mtmd_context_params_default", out);
    auto p_mtmd_init_from_file = load_fn<decltype(&mtmd_init_from_file)>(mtmd_h, "mtmd_init_from_file", out);
    auto p_mtmd_free = load_fn<decltype(&mtmd_free)>(mtmd_h, "mtmd_free", out);
    using mtmd_helper_bitmap_init_from_file_fn = mtmd_bitmap * (*)(mtmd_context *, const char *);
    using mtmd_bitmap_free_fn = void (*)(mtmd_bitmap *);
    using mtmd_default_marker_fn = const char * (*)();
    using mtmd_input_chunks_init_fn = mtmd_input_chunks * (*)();
    using mtmd_input_chunks_size_fn = size_t (*)(const mtmd_input_chunks *);
    using mtmd_input_chunks_get_fn = const mtmd_input_chunk * (*)(const mtmd_input_chunks *, size_t);
    using mtmd_input_chunks_free_fn = void (*)(mtmd_input_chunks *);
    using mtmd_tokenize_fn = int32_t (*)(mtmd_context *, mtmd_input_chunks *, const mtmd_input_text *, const mtmd_bitmap **, size_t);
    using mtmd_encode_chunk_fn = int32_t (*)(mtmd_context *, const mtmd_input_chunk *);
    using mtmd_get_output_embd_fn = float * (*)(mtmd_context *);
    using mtmd_input_chunk_get_type_fn = enum mtmd_input_chunk_type (*)(const mtmd_input_chunk *);
    using mtmd_input_chunk_get_tokens_text_fn = const llama_token * (*)(const mtmd_input_chunk *, size_t *);
    using mtmd_input_chunk_get_n_tokens_fn = size_t (*)(const mtmd_input_chunk *);
    using mtmd_input_chunk_get_n_pos_fn = llama_pos (*)(const mtmd_input_chunk *);

    auto p_mtmd_helper_bitmap_init_from_file = load_fn<mtmd_helper_bitmap_init_from_file_fn>(mtmd_h, "mtmd_helper_bitmap_init_from_file", out);
    auto p_mtmd_bitmap_free = load_fn<mtmd_bitmap_free_fn>(mtmd_h, "mtmd_bitmap_free", out);
    auto p_mtmd_default_marker = load_fn<mtmd_default_marker_fn>(mtmd_h, "mtmd_default_marker", out, false);
    auto p_mtmd_input_chunks_init = load_fn<mtmd_input_chunks_init_fn>(mtmd_h, "mtmd_input_chunks_init", out);
    auto p_mtmd_input_chunks_size = load_fn<mtmd_input_chunks_size_fn>(mtmd_h, "mtmd_input_chunks_size", out);
    auto p_mtmd_input_chunks_get = load_fn<mtmd_input_chunks_get_fn>(mtmd_h, "mtmd_input_chunks_get", out);
    auto p_mtmd_input_chunks_free = load_fn<mtmd_input_chunks_free_fn>(mtmd_h, "mtmd_input_chunks_free", out);
    auto p_mtmd_tokenize = load_fn<mtmd_tokenize_fn>(mtmd_h, "mtmd_tokenize", out);
    auto p_mtmd_encode_chunk = load_fn<mtmd_encode_chunk_fn>(mtmd_h, "mtmd_encode_chunk", out);
    auto p_mtmd_get_output_embd = load_fn<mtmd_get_output_embd_fn>(mtmd_h, "mtmd_get_output_embd", out);
    auto p_mtmd_input_chunk_get_type = load_fn<mtmd_input_chunk_get_type_fn>(mtmd_h, "mtmd_input_chunk_get_type", out);
    auto p_mtmd_input_chunk_get_tokens_text = load_fn<mtmd_input_chunk_get_tokens_text_fn>(mtmd_h, "mtmd_input_chunk_get_tokens_text", out);
    auto p_mtmd_input_chunk_get_n_tokens = load_fn<mtmd_input_chunk_get_n_tokens_fn>(mtmd_h, "mtmd_input_chunk_get_n_tokens", out);
    auto p_mtmd_input_chunk_get_n_pos = load_fn<mtmd_input_chunk_get_n_pos_fn>(mtmd_h, "mtmd_input_chunk_get_n_pos", out, false);

    if (!p_llama_backend_init || !p_llama_backend_free || !p_llama_model_default_params ||
        !p_llama_model_load_from_file || !p_llama_model_free || !p_llama_context_default_params ||
        !p_llama_init_from_model || !p_llama_free || !p_llama_model_get_vocab || !p_llama_model_n_embd ||
        !p_llama_batch_init || !p_llama_batch_free || !p_llama_decode ||
        !p_mtmd_context_params_default || !p_mtmd_init_from_file || !p_mtmd_free ||
        !p_mtmd_helper_bitmap_init_from_file || !p_mtmd_bitmap_free || !p_mtmd_input_chunks_init ||
        !p_mtmd_input_chunks_size || !p_mtmd_input_chunks_get || !p_mtmd_input_chunks_free ||
        !p_mtmd_tokenize || !p_mtmd_encode_chunk || !p_mtmd_get_output_embd ||
        !p_mtmd_input_chunk_get_type || !p_mtmd_input_chunk_get_tokens_text || !p_mtmd_input_chunk_get_n_tokens) {
        out << "\nSTAGE5P_STOP: required function pointer missing ❌\n";
        return make_jstring(env, out.str());
    }

    out << "\nLoading model + context + mtmd...\n";
    p_llama_backend_init();
    llama_model_params model_params = p_llama_model_default_params();
    model_params.n_gpu_layers = 0;
    llama_model *model = p_llama_model_load_from_file(g_main_model_path.c_str(), model_params);
    if (!model) {
        out << "MODEL_LOAD_FAILED ❌\n";
        p_llama_backend_free();
        return make_jstring(env, out.str());
    }
    out << "MODEL_LOAD_OK ✅\n";

    llama_context_params cparams = p_llama_context_default_params();
    cparams.n_ctx = 768;
    cparams.n_batch = 256;
    cparams.n_threads = 4;
    cparams.n_threads_batch = 4;
    llama_context *ctx = p_llama_init_from_model(model, cparams);
    if (!ctx) {
        out << "LLAMA_CONTEXT_CREATE_FAILED ❌\n";
        p_llama_model_free(model);
        p_llama_backend_free();
        return make_jstring(env, out.str());
    }
    out << "LLAMA_CONTEXT_CREATE_OK ✅\n";

    mtmd_context_params mtmd_params = p_mtmd_context_params_default();
    mtmd_context *mctx = p_mtmd_init_from_file(g_mmproj_path.c_str(), model, mtmd_params);
    if (!mctx) {
        out << "MTMD_INIT_FROM_FILE_FAILED ❌\n";
        p_llama_free(ctx);
        p_llama_model_free(model);
        p_llama_backend_free();
        return make_jstring(env, out.str());
    }
    out << "MTMD_INIT_FROM_FILE_OK ✅\n";

    mtmd_bitmap *bitmap = p_mtmd_helper_bitmap_init_from_file(mctx, image.c_str());
    if (!bitmap) {
        out << "BITMAP_LOAD_FAILED ❌\n";
        p_mtmd_free(mctx);
        p_llama_free(ctx);
        p_llama_model_free(model);
        p_llama_backend_free();
        return make_jstring(env, out.str());
    }
    out << "BITMAP_LOAD_OK ✅\n";

    const char *marker_c = p_mtmd_default_marker ? p_mtmd_default_marker() : nullptr;
    std::string marker = (marker_c && marker_c[0]) ? std::string(marker_c) : std::string("<__media__>");

    // Stage 5S: keep the proven image->decode->generate path.
    // Use a much shorter prompt and an assistant prefix to reduce instruction echo and incomplete text.
    std::string user_instruction = "স্ক্রিনে কী দেখা যাচ্ছে? শুধু দৃশ্যমান জিনিস বলো। ১টি ছোট বাংলা বাক্য। নির্দেশনা কপি করবে না।";
    std::string answer_prefix = "স্ক্রিনে দেখা যাচ্ছে ";
    std::string tokenize_prompt;
    tokenize_prompt += "<|im_start|>system\n";
    tokenize_prompt += "You are a phone screenshot analyzer. Look at the image. Answer in Bengali only. Do not repeat instructions. Do not mention uncertainty unless needed.\n";
    tokenize_prompt += "<|im_end|>\n";
    tokenize_prompt += "<|im_start|>user\n";
    tokenize_prompt += marker;
    tokenize_prompt += "\n";
    tokenize_prompt += user_instruction;
    tokenize_prompt += "\n<|im_end|>\n";
    tokenize_prompt += "<|im_start|>assistant\n";
    tokenize_prompt += answer_prefix;

    mtmd_input_chunks *chunks = p_mtmd_input_chunks_init();
    mtmd_input_text input_text{};
    input_text.text = tokenize_prompt.c_str();
    input_text.add_special = true;
    input_text.parse_special = true;
    const mtmd_bitmap *bitmaps[1] = { bitmap };
    int32_t tok_rc = p_mtmd_tokenize(mctx, chunks, &input_text, bitmaps, 1);
    out << "MTMD_TOKENIZE_RC=" << tok_rc << "\n";
    if (tok_rc != 0) {
        out << "MTMD_TOKENIZE_FAILED ❌\n";
        p_mtmd_input_chunks_free(chunks);
        p_mtmd_bitmap_free(bitmap);
        p_mtmd_free(mctx);
        p_llama_free(ctx);
        p_llama_model_free(model);
        p_llama_backend_free();
        return make_jstring(env, out.str());
    }
    out << "MTMD_TOKENIZE_OK ✅\n";

    const llama_vocab *vocab = p_llama_model_get_vocab(model);
    const int n_embd = p_llama_model_n_embd(model);
    out << "LLAMA_N_EMBD=" << n_embd << "\n";

    llama_pos cur_pos = 0;
    bool eval_failed = false;
    size_t n_chunks = p_mtmd_input_chunks_size(chunks);
    out << "INPUT_CHUNKS_SIZE=" << n_chunks << "\n";

    auto decode_text_tokens = [&](const llama_token *tokens, size_t n_tokens, bool want_logits) -> bool {
        if (!tokens || n_tokens == 0) return true;
        llama_batch batch = p_llama_batch_init((int32_t)n_tokens, 0, 1);
        batch.n_tokens = (int32_t)n_tokens;
        for (int32_t i = 0; i < (int32_t)n_tokens; ++i) {
            batch.token[i] = tokens[i];
            batch.pos[i] = cur_pos + i;
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = (want_logits && i == (int32_t)n_tokens - 1) ? 1 : 0;
        }
        int rc = p_llama_decode(ctx, batch);
        p_llama_batch_free(batch);
        if (rc != 0) return false;
        cur_pos += (llama_pos)n_tokens;
        return true;
    };

    auto decode_image_embd = [&](float *embd, size_t n_image_pos, bool want_logits) -> bool {
        // Important: mtmd image chunks can report many image tokens but fewer decoder positions.
        // Stage 5R uses n_pos for image embedding decode.
        if (!embd || n_image_pos == 0 || n_embd <= 0) return false;
        llama_batch batch = p_llama_batch_init((int32_t)n_image_pos, n_embd, 1);
        batch.n_tokens = (int32_t)n_image_pos;
        std::memcpy(batch.embd, embd, n_image_pos * (size_t)n_embd * sizeof(float));
        for (int32_t i = 0; i < (int32_t)n_image_pos; ++i) {
            batch.pos[i] = cur_pos + i;
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = (want_logits && i == (int32_t)n_image_pos - 1) ? 1 : 0;
        }
        int rc = p_llama_decode(ctx, batch);
        p_llama_batch_free(batch);
        if (rc != 0) return false;
        cur_pos += (llama_pos)n_image_pos;
        return true;
    };

    out << "\nEvaluating chunks into llama context...\n";
    for (size_t ci = 0; ci < n_chunks; ++ci) {
        const mtmd_input_chunk *chunk = p_mtmd_input_chunks_get(chunks, ci);
        if (!chunk) continue;
        enum mtmd_input_chunk_type t = p_mtmd_input_chunk_get_type(chunk);
        bool is_last = (ci == n_chunks - 1);
        size_t n_tok = p_mtmd_input_chunk_get_n_tokens(chunk);
        llama_pos n_pos = p_mtmd_input_chunk_get_n_pos ? p_mtmd_input_chunk_get_n_pos(chunk) : (llama_pos)n_tok;
        out << "CHUNK[" << ci << "] type=" << (int)t << ", tokens=" << n_tok << ", n_pos=" << n_pos << "\n";
        if (t == MTMD_INPUT_CHUNK_TYPE_TEXT) {
            size_t text_n = 0;
            const llama_token *text_tokens = p_mtmd_input_chunk_get_tokens_text(chunk, &text_n);
            out << "  TEXT_TOKENS=" << text_n << "\n";
            if (!decode_text_tokens(text_tokens, text_n, is_last)) {
                out << "  TEXT_DECODE_FAILED ❌\n";
                eval_failed = true;
                break;
            }
            out << "  TEXT_DECODE_OK ✅ cur_pos=" << cur_pos << "\n";
        } else if (t == MTMD_INPUT_CHUNK_TYPE_IMAGE) {
            int32_t enc_rc = p_mtmd_encode_chunk(mctx, chunk);
            out << "  MTMD_ENCODE_CHUNK_RC=" << enc_rc << "\n";
            if (enc_rc != 0) {
                out << "  IMAGE_ENCODE_FAILED ❌\n";
                eval_failed = true;
                break;
            }
            float *embd = p_mtmd_get_output_embd(mctx);
            out << "  IMAGE_EMBD_PTR=" << embd << "\n";
            size_t image_decode_positions = n_pos > 0 ? (size_t)n_pos : n_tok;
            out << "  IMAGE_DECODE_POSITIONS_USED=" << image_decode_positions << " (n_pos preferred over tokens)\n";
            if (!decode_image_embd(embd, image_decode_positions, is_last)) {
                out << "  IMAGE_EMBD_DECODE_FAILED ❌\n";
                eval_failed = true;
                break;
            }
            out << "  IMAGE_EMBD_DECODE_OK ✅ cur_pos=" << cur_pos << "\n";
        }
    }

    if (eval_failed) {
        out << "STAGE5P_CHUNK_EVAL_FAILED ❌\n";
        p_mtmd_input_chunks_free(chunks);
        p_mtmd_bitmap_free(bitmap);
        p_mtmd_free(mctx);
        p_llama_free(ctx);
        p_llama_model_free(model);
        p_llama_backend_free();
        return make_jstring(env, out.str());
    }
    out << "STAGE5P_CHUNK_EVAL_OK ✅\n";

    out << "\nGreedy short answer generation...\n";
    llama_sampler *smpl = llama_sampler_init_greedy();

    std::string generated;
    int generated_count = 0;
    for (int i = 0; i < 96; ++i) {
        llama_token new_token = llama_sampler_sample(smpl, ctx, -1);
        if (llama_vocab_is_eog(vocab, new_token)) break;
        std::string piece = token_to_piece_safe(vocab, new_token);
        generated += piece;
        if (generated.find("<|im_end|>") != std::string::npos ||
            generated.find("<|endoftext|>") != std::string::npos ||
            generated.find("\n") != std::string::npos) {
            break;
        }
        if (!decode_text_tokens(&new_token, 1, true)) {
            out << "GEN_TOKEN_DECODE_FAILED_AT=" << i << " ❌\n";
            break;
        }
        generated_count++;
    }
    llama_sampler_free(smpl);

    out << "GENERATION_STARTED ✅\n";
    out << "GENERATION_MODE=GREEDY_CONCISE_BENGALI_SCREEN_ANSWER\n";
    std::string final_answer = answer_prefix + generated;
    out << "GENERATED_TOKENS=" << generated_count << "\n";
    out << "TEXT_OUTPUT=" << final_answer << "\n";
    out << "USER_FINAL_ANSWER_ONLY=" << final_answer << "\n";
    if (generated_count > 0) out << "STAGE5S_CONCISE_SCREEN_ANSWER_PROBE_OK ✅\n";
    else out << "STAGE5S_GENERATED_EMPTY ⚠️\n";

    p_mtmd_input_chunks_free(chunks);
    p_mtmd_bitmap_free(bitmap);
    p_mtmd_free(mctx);
    p_llama_free(ctx);
    p_llama_model_free(model);
    p_llama_backend_free();

    std::string result = out.str();
    LOGI("%s", result.c_str());
    return make_jstring(env, result);
}

extern "C" JNIEXPORT void JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeRelease(JNIEnv * /*env*/, jobject /*thiz*/) {
    LOGI("nativeRelease Stage 5S called");
    g_main_model_path.clear();
    g_mmproj_path.clear();
    g_initialized = false;
}
