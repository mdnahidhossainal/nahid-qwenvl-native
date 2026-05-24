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
    out << "MTMD INIT-FROM-FILE PROBE — Stage 5O\n";
    out << "This stage loads main GGUF, then calls only mtmd_context_params_default + mtmd_init_from_file.\n";
    out << "No screenshot bitmap, no mtmd_encode, no image inference is called.\n\n";

    out << "File checks:\n";
    out << "MAIN_EXISTS=" << (file_exists(g_main_model_path) ? "true" : "false") << "\n";
    out << "MAIN_SIZE=" << file_size(g_main_model_path) << "\n";
    out << "MMPROJ_EXISTS=" << (file_exists(g_mmproj_path) ? "true" : "false") << "\n";
    out << "MMPROJ_SIZE=" << file_size(g_mmproj_path) << "\n\n";

    if (!file_exists(g_main_model_path)) {
        out << "STAGE5O_STOP: main GGUF not found ❌\n";
        return out.str();
    }
    if (!file_exists(g_mmproj_path)) {
        out << "STAGE5O_STOP: mmproj GGUF not found ❌\n";
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
        out << "\nSTAGE5O_STOP: required function pointer missing ❌\n";
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

    out << "STAGE5O_MMPROJ_INIT_PROBE_OK ✅\n";
    out << "NEXT_SAFE_STAGE: load screenshot bitmap with mtmd_helper_bitmap_init_from_file, then tokenize/evaluate chunks.\n";
    return out.str();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativePing(JNIEnv *env, jobject /*thiz*/) {
    LOGI("nativePing Stage 5O called");
    return make_jstring(env, "PONG_STAGE_5O: image chunk encode probe is available.");
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
    out << "INIT_STAGE_5O_IMAGE_CHUNK_ENCODE_PROBE\n";
    out << "MAIN=" << g_main_model_path << "\n";
    out << "MMPROJ=" << g_mmproj_path << "\n\n";
    out << mtmd_init_from_file_probe();
    out << "\nNOTE: Stage 5O does NOT run screenshot understanding yet. It only verifies mmproj projector init through libmtmd.\n";

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
    out << "ANALYZE_STAGE_5O_IMAGE_CHUNK_ENCODE_PROBE\n";
    out << "This stage loads the saved screenshot bitmap, creates mtmd input chunks from prompt + bitmap,\n";
    out << "and prints chunk types/counts. No mtmd_encode, no llama_decode, no real final answer generation is called.\n\n";
    out << "IMAGE=" << image << "\n";
    out << "IMAGE_EXISTS=" << (file_exists(image) ? "true" : "false") << "\n";
    out << "IMAGE_SIZE=" << file_size(image) << "\n";
    out << "MODEL_INITIALIZED_PATHS=" << (g_initialized ? "true" : "false") << "\n";
    out << "PROMPT_PREVIEW=" << p.substr(0, 220) << "\n\n";

    if (!file_exists(image)) {
        out << "STAGE5O_STOP: screenshot image file not found ❌\n";
        std::string result = out.str();
        LOGI("%s", result.c_str());
        return make_jstring(env, result);
    }

    void *ggml = safe_dlopen("libggml.so", out);
    void *ggml_base = safe_dlopen("libggml-base.so", out);
    void *ggml_cpu = safe_dlopen("libggml-cpu.so", out);
    void *llama_h = safe_dlopen("libllama.so", out);
    void *mtmd_h = safe_dlopen("libmtmd.so", out);
    (void)ggml; (void)ggml_base; (void)ggml_cpu;

    out << "\nModel + bitmap + tokenize function pointers (Stage 5O):\n";
    auto p_llama_backend_init = load_fn<decltype(&llama_backend_init)>(llama_h, "llama_backend_init", out);
    auto p_llama_backend_free = load_fn<decltype(&llama_backend_free)>(llama_h, "llama_backend_free", out);
    auto p_llama_model_default_params = load_fn<decltype(&llama_model_default_params)>(llama_h, "llama_model_default_params", out);
    auto p_llama_model_load_from_file = load_fn<decltype(&llama_model_load_from_file)>(llama_h, "llama_model_load_from_file", out);
    auto p_llama_model_free = load_fn<decltype(&llama_model_free)>(llama_h, "llama_model_free", out);
    auto p_mtmd_context_params_default = load_fn<decltype(&mtmd_context_params_default)>(mtmd_h, "mtmd_context_params_default", out);
    auto p_mtmd_init_from_file = load_fn<decltype(&mtmd_init_from_file)>(mtmd_h, "mtmd_init_from_file", out);
    auto p_mtmd_free = load_fn<decltype(&mtmd_free)>(mtmd_h, "mtmd_free", out);

    using mtmd_helper_bitmap_init_from_file_fn = mtmd_bitmap * (*)(mtmd_context *, const char *);
    using mtmd_bitmap_free_fn = void (*)(mtmd_bitmap *);
    using mtmd_bitmap_get_u32_fn = uint32_t (*)(const mtmd_bitmap *);
    using mtmd_bitmap_get_size_fn = size_t (*)(const mtmd_bitmap *);
    using mtmd_default_marker_fn = const char * (*)();
    using mtmd_input_chunks_init_fn = mtmd_input_chunks * (*)();
    using mtmd_input_chunks_size_fn = size_t (*)(const mtmd_input_chunks *);
    using mtmd_input_chunks_get_fn = const mtmd_input_chunk * (*)(const mtmd_input_chunks *, size_t);
    using mtmd_input_chunks_free_fn = void (*)(mtmd_input_chunks *);
    using mtmd_tokenize_fn = int32_t (*)(mtmd_context *, mtmd_input_chunks *, const mtmd_input_text *, const mtmd_bitmap **, size_t);
    using mtmd_encode_chunk_fn = int32_t (*)(mtmd_context *, const mtmd_input_chunk *);
    using mtmd_get_output_embd_fn = float * (*)(mtmd_context *);
    using mtmd_input_chunk_get_type_fn = enum mtmd_input_chunk_type (*)(const mtmd_input_chunk *);
    using mtmd_input_chunk_get_n_tokens_fn = size_t (*)(const mtmd_input_chunk *);
    using mtmd_input_chunk_get_n_pos_fn = llama_pos (*)(const mtmd_input_chunk *);
    using mtmd_input_chunk_get_id_fn = const char * (*)(const mtmd_input_chunk *);

    auto p_mtmd_helper_bitmap_init_from_file = load_fn<mtmd_helper_bitmap_init_from_file_fn>(mtmd_h, "mtmd_helper_bitmap_init_from_file", out);
    auto p_mtmd_bitmap_free = load_fn<mtmd_bitmap_free_fn>(mtmd_h, "mtmd_bitmap_free", out);
    auto p_mtmd_bitmap_get_nx = load_fn<mtmd_bitmap_get_u32_fn>(mtmd_h, "mtmd_bitmap_get_nx", out, false);
    auto p_mtmd_bitmap_get_ny = load_fn<mtmd_bitmap_get_u32_fn>(mtmd_h, "mtmd_bitmap_get_ny", out, false);
    auto p_mtmd_bitmap_get_n_bytes = load_fn<mtmd_bitmap_get_size_fn>(mtmd_h, "mtmd_bitmap_get_n_bytes", out, false);
    auto p_mtmd_default_marker = load_fn<mtmd_default_marker_fn>(mtmd_h, "mtmd_default_marker", out, false);
    auto p_mtmd_input_chunks_init = load_fn<mtmd_input_chunks_init_fn>(mtmd_h, "mtmd_input_chunks_init", out);
    auto p_mtmd_input_chunks_size = load_fn<mtmd_input_chunks_size_fn>(mtmd_h, "mtmd_input_chunks_size", out);
    auto p_mtmd_input_chunks_get = load_fn<mtmd_input_chunks_get_fn>(mtmd_h, "mtmd_input_chunks_get", out);
    auto p_mtmd_input_chunks_free = load_fn<mtmd_input_chunks_free_fn>(mtmd_h, "mtmd_input_chunks_free", out);
    auto p_mtmd_tokenize = load_fn<mtmd_tokenize_fn>(mtmd_h, "mtmd_tokenize", out);
    auto p_mtmd_encode_chunk = load_fn<mtmd_encode_chunk_fn>(mtmd_h, "mtmd_encode_chunk", out);
    auto p_mtmd_get_output_embd = load_fn<mtmd_get_output_embd_fn>(mtmd_h, "mtmd_get_output_embd", out, false);
    auto p_mtmd_input_chunk_get_type = load_fn<mtmd_input_chunk_get_type_fn>(mtmd_h, "mtmd_input_chunk_get_type", out, false);
    auto p_mtmd_input_chunk_get_n_tokens = load_fn<mtmd_input_chunk_get_n_tokens_fn>(mtmd_h, "mtmd_input_chunk_get_n_tokens", out, false);
    auto p_mtmd_input_chunk_get_n_pos = load_fn<mtmd_input_chunk_get_n_pos_fn>(mtmd_h, "mtmd_input_chunk_get_n_pos", out, false);
    auto p_mtmd_input_chunk_get_id = load_fn<mtmd_input_chunk_get_id_fn>(mtmd_h, "mtmd_input_chunk_get_id", out, false);

    if (!p_llama_backend_init || !p_llama_backend_free || !p_llama_model_default_params ||
        !p_llama_model_load_from_file || !p_llama_model_free ||
        !p_mtmd_context_params_default || !p_mtmd_init_from_file || !p_mtmd_free ||
        !p_mtmd_helper_bitmap_init_from_file || !p_mtmd_bitmap_free ||
        !p_mtmd_input_chunks_init || !p_mtmd_input_chunks_size || !p_mtmd_input_chunks_get ||
        !p_mtmd_input_chunks_free || !p_mtmd_tokenize || !p_mtmd_encode_chunk || !p_mtmd_input_chunk_get_type) {
        out << "\nSTAGE5O_STOP: required function pointer missing ❌\n";
        std::string result = out.str();
        LOGI("%s", result.c_str());
        return make_jstring(env, result);
    }

    out << "\nCalling llama_backend_init + model load + mtmd_init_from_file...\n";
    p_llama_backend_init();
    llama_model_params model_params = p_llama_model_default_params();
    model_params.n_gpu_layers = 0;
    llama_model *model = p_llama_model_load_from_file(g_main_model_path.c_str(), model_params);
    if (!model) {
        out << "MODEL_LOAD_FAILED ❌\n";
        p_llama_backend_free();
        std::string result = out.str();
        LOGI("%s", result.c_str());
        return make_jstring(env, result);
    }
    out << "MODEL_LOAD_OK ✅\n";

    mtmd_context_params mtmd_params = p_mtmd_context_params_default();
    mtmd_context *mctx = p_mtmd_init_from_file(g_mmproj_path.c_str(), model, mtmd_params);
    if (!mctx) {
        out << "MTMD_INIT_FROM_FILE_FAILED ❌\n";
        p_llama_model_free(model);
        p_llama_backend_free();
        std::string result = out.str();
        LOGI("%s", result.c_str());
        return make_jstring(env, result);
    }
    out << "MTMD_INIT_FROM_FILE_OK ✅\n";

    out << "\nCalling mtmd_helper_bitmap_init_from_file(mctx, saved_screenshot)...\n";
    mtmd_bitmap *bitmap = p_mtmd_helper_bitmap_init_from_file(mctx, image.c_str());
    out << "BITMAP_PTR=" << bitmap << "\n";
    if (!bitmap) {
        out << "BITMAP_LOAD_FAILED ❌\n";
        p_mtmd_free(mctx);
        p_llama_model_free(model);
        p_llama_backend_free();
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

    const char *marker_c = p_mtmd_default_marker ? p_mtmd_default_marker() : nullptr;
    std::string marker = (marker_c && marker_c[0]) ? std::string(marker_c) : std::string("<__media__>");
    out << "MTMD_DEFAULT_MARKER=" << marker << "\n";

    std::string tokenize_prompt;
    tokenize_prompt += "You are Nahid AI Offline Vision Judge.\n";
    tokenize_prompt += "Screenshot: ";
    tokenize_prompt += marker;
    tokenize_prompt += "\nUser instruction: ";
    tokenize_prompt += p;
    tokenize_prompt += "\nReturn compact Bengali summary plus JSON with visible_items and confidence.";
    out << "TOKENIZE_PROMPT_PREVIEW=" << tokenize_prompt.substr(0, 260) << "\n";

    mtmd_input_chunks *chunks = p_mtmd_input_chunks_init();
    if (!chunks) {
        out << "INPUT_CHUNKS_INIT_FAILED ❌\n";
        p_mtmd_bitmap_free(bitmap);
        p_mtmd_free(mctx);
        p_llama_model_free(model);
        p_llama_backend_free();
        std::string result = out.str();
        LOGI("%s", result.c_str());
        return make_jstring(env, result);
    }
    out << "INPUT_CHUNKS_INIT_OK ✅\n";

    mtmd_input_text input_text{};
    input_text.text = tokenize_prompt.c_str();
    input_text.add_special = true;
    input_text.parse_special = true;
    const mtmd_bitmap *bitmaps[1] = { bitmap };

    out << "Calling mtmd_tokenize(ctx, chunks, text_with_marker, 1 bitmap)...\n";
    int32_t tok_rc = p_mtmd_tokenize(mctx, chunks, &input_text, bitmaps, 1);
    out << "MTMD_TOKENIZE_RC=" << tok_rc << "\n";
    if (tok_rc != 0) {
        out << "MTMD_TOKENIZE_FAILED ❌\n";
        out << "Return meanings from mtmd.h: 1=bitmap/marker count mismatch, 2=image preprocessing error.\n";
        p_mtmd_input_chunks_free(chunks);
        p_mtmd_bitmap_free(bitmap);
        p_mtmd_free(mctx);
        p_llama_model_free(model);
        p_llama_backend_free();
        std::string result = out.str();
        LOGI("%s", result.c_str());
        return make_jstring(env, result);
    }

    out << "MTMD_TOKENIZE_OK ✅\n";
    size_t n_chunks = p_mtmd_input_chunks_size(chunks);
    out << "INPUT_CHUNKS_SIZE=" << n_chunks << "\n";
    for (size_t i = 0; i < n_chunks && i < 12; ++i) {
        const mtmd_input_chunk *chunk = p_mtmd_input_chunks_get(chunks, i);
        out << "CHUNK[" << i << "]=";
        if (!chunk) {
            out << "NULL\n";
            continue;
        }
        if (p_mtmd_input_chunk_get_type) {
            enum mtmd_input_chunk_type t = p_mtmd_input_chunk_get_type(chunk);
            out << "type=" << static_cast<int>(t);
            if (t == MTMD_INPUT_CHUNK_TYPE_TEXT) out << "(TEXT)";
            else if (t == MTMD_INPUT_CHUNK_TYPE_IMAGE) out << "(IMAGE)";
            else if (t == MTMD_INPUT_CHUNK_TYPE_AUDIO) out << "(AUDIO)";
            else out << "(UNKNOWN)";
        } else {
            out << "type=?";
        }
        if (p_mtmd_input_chunk_get_n_tokens) {
            out << ", tokens=" << p_mtmd_input_chunk_get_n_tokens(chunk);
        }
        if (p_mtmd_input_chunk_get_n_pos) {
            out << ", n_pos=" << p_mtmd_input_chunk_get_n_pos(chunk);
        }
        if (p_mtmd_input_chunk_get_id) {
            const char *id = p_mtmd_input_chunk_get_id(chunk);
            if (id) out << ", id=" << id;
        }
        out << "\n";
    }

    out << "\nImage chunk encode probe:\n";
    size_t image_chunks = 0;
    size_t encode_ok = 0;
    for (size_t i = 0; i < n_chunks; ++i) {
        const mtmd_input_chunk *chunk = p_mtmd_input_chunks_get(chunks, i);
        if (!chunk) continue;
        enum mtmd_input_chunk_type t = p_mtmd_input_chunk_get_type(chunk);
        if (t != MTMD_INPUT_CHUNK_TYPE_IMAGE) continue;
        image_chunks++;
        out << "Calling mtmd_encode_chunk on IMAGE chunk[" << i << "]...\n";
        int32_t enc_rc = p_mtmd_encode_chunk(mctx, chunk);
        out << "MTMD_ENCODE_CHUNK_RC=" << enc_rc << "\n";
        if (enc_rc == 0) {
            encode_ok++;
            out << "MTMD_ENCODE_CHUNK_OK ✅\n";
            if (p_mtmd_get_output_embd) {
                float *embd = p_mtmd_get_output_embd(mctx);
                out << "MTMD_OUTPUT_EMBD_PTR=" << embd << "\n";
                if (embd) out << "MTMD_OUTPUT_EMBD_AVAILABLE ✅\n";
                else out << "MTMD_OUTPUT_EMBD_NULL ⚠️\n";
            }
        } else {
            out << "MTMD_ENCODE_CHUNK_FAILED ❌\n";
        }
        // Encode only the first image chunk in this probe to reduce memory/thermal risk.
        break;
    }
    out << "IMAGE_CHUNKS_FOUND=" << image_chunks << "\n";
    out << "IMAGE_CHUNKS_ENCODE_OK=" << encode_ok << "\n";
    if (image_chunks > 0 && encode_ok > 0) {
        out << "STAGE5O_IMAGE_CHUNK_ENCODE_PROBE_OK ✅\n";
    } else if (image_chunks == 0) {
        out << "STAGE5O_NO_IMAGE_CHUNK_FOUND ❌\n";
    } else {
        out << "STAGE5O_IMAGE_CHUNK_ENCODE_PROBE_FAILED ❌\n";
    }

    out << "Freeing chunks, bitmap, mtmd context, and model immediately to keep RAM safe...\n";
    p_mtmd_input_chunks_free(chunks);
    p_mtmd_bitmap_free(bitmap);
    p_mtmd_free(mctx);
    p_llama_model_free(model);
    p_llama_backend_free();

    out << "NEXT_SAFE_STAGE: connect encoded image embeddings to llama decode and generate first screenshot answer.\n";

    std::string result = out.str();
    LOGI("%s", result.c_str());
    return make_jstring(env, result);
}

extern "C" JNIEXPORT void JNICALL
Java_com_nahidai_assistant_screen_QwenVlNativeBridge_nativeRelease(JNIEnv * /*env*/, jobject /*thiz*/) {
    LOGI("nativeRelease Stage 5O called");
    g_main_model_path.clear();
    g_mmproj_path.clear();
    g_initialized = false;
}
