#include <fstream>
#include <sstream>
#include <utility>

#if defined(__ANDROID__)
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <jni.h>
extern JavaVM* _jni_jvm;
extern jobject _jni_activity;
#endif

#if defined(__EMSCRIPTEN__)
#include <atomic>
#include <emscripten.h>
#endif

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
#include <nfd.h>
#endif

#include <core/dialog.hpp>

namespace {

#if defined(__EMSCRIPTEN__)

// clang-format off
EM_ASYNC_JS(char*, _upload_file, (const char* types_json), {
    const _accept = UTF8ToString(types_json);
    return await new Promise(resolve => {
        const _element = document.createElement('input');
        _element.type = 'file';
        _element.accept = _accept;
        let _is_dialog_open = false;
        let _is_done = false;
        let _timeout = null;
        function _finish_with_path(path)
        {
            const _length = lengthBytesUTF8(path) + 1;
            const _cstr = _malloc(_length);
            stringToUTF8(path, _cstr, _length);
            resolve(_cstr);
        }
        _element.addEventListener('change', async () => {
            if (_is_done) {
                return;
            }
            _is_done = true;
            clearTimeout(_timeout);
            if (!_element.files.length) {
                resolve(0);
                return;
            }
            const _file = _element.files[0];
            const _buffer = new Uint8Array(await _file.arrayBuffer());
            const _path = '/tmp/' + _file.name;
            FS.mkdirTree('/tmp');
            FS.writeFile(_path, _buffer);
            _finish_with_path(_path); });
        _element.addEventListener('click', () => { _is_dialog_open = true; });
        window.addEventListener('focus', () => {
            if (!_is_dialog_open || _is_done) {
                return;
            }
            clearTimeout(_timeout);
            _timeout = setTimeout(() => {
                if (_is_done) {
                    return;
                }
                _is_done = true;
                if (!_element.files.length) {
                    resolve(0);
                }
            }, 100); });
        _element.click();
    });
});
// clang-format on

#endif

[[nodiscard]] static std::string _build_filters(const std::vector<std::pair<std::string, std::string>>& filters)
{
    std::string _filters;

#if defined(__ANDROID__)
    for (const std::pair<const std::string, std::string>& _filter : filters) {
        if (!_filters.empty()) {
            _filters += ",";
        }
        const std::string& _extension = _filter.second;
        if (_extension == ".png") {
            _filters += "image/png";
        } else if (_extension == ".jpg" || _extension == ".jpeg") {
            _filters += "image/jpeg";
        } else if (_extension == ".txt") {
            _filters += "text/plain";
        } else {
            _filters += "*/*";
        }
    }
#endif

#if defined(__EMSCRIPTEN__)
    for (const std::pair<const std::string, std::string>& _filter : filters) {
        if (!_filters.empty()) {
            _filters += ",";
        }
        std::string _pattern = _filter.second;
        if (_pattern.rfind("*.", 0) == 0) {
            _pattern = _pattern.substr(1);
        } else if (!_pattern.empty() && _pattern[0] != '.') {
            _pattern = "." + _pattern;
        }
        _filters += _pattern;
    }
#endif

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    bool _is_first = true;
    for (const std::pair<const std::string, std::string>& _filter : filters) {
        const std::string& _pattern = _filter.second;
        std::size_t _start = 0;
        while (_start < _pattern.size()) {
            std::size_t _end = _pattern.find(';', _start);
            if (_end == std::string::npos) {
                _end = _pattern.size();
            }
            std::string _token = _pattern.substr(_start, _end - _start);
            const std::size_t _trim_front = _token.find_first_not_of(" \t");
            if (_trim_front != std::string::npos) {
                _token.erase(0, _trim_front);
            }
            const std::size_t trim_back = _token.find_last_not_of(" \t");
            if (trim_back != std::string::npos) {
                _token.erase(trim_back + 1);
            }
            if (_token.rfind("*.", 0) == 0) {
                _token.erase(0, 2);
            } else if (!_token.empty() && _token[0] == '.') {
                _token.erase(0, 1);
            }
            if (!_token.empty()) {
                if (!_is_first) {
                    _filters += ",";
                }
                _filters += _token;
                _is_first = false;
            }
            _start = _end + 1;
        }
    }
#endif

    return _filters;
}

[[nodiscard]] inline std::filesystem::path _root_path()
{
#if defined(__ANDROID__)
    JNIEnv* _jni_env = nullptr;
    _jni_jvm->AttachCurrentThread(&_jni_env, nullptr);
    jclass _jni_activity_class = _jni_env->GetObjectClass(_jni_activity);
    jmethodID _jni_get_files_dir_method = _jni_env->GetMethodID(_jni_activity_class, "getFilesDir", "()Ljava/io/File;");
    jobject _jni_file_object = _jni_env->CallObjectMethod(_jni_activity, _jni_get_files_dir_method);
    jclass _jni_file_class = _jni_env->GetObjectClass(_jni_file_object);
    jmethodID _jni_get_absolute_path_method = _jni_env->GetMethodID(_jni_file_class, "getAbsolutePath", "()Ljava/lang/String;");
    jstring _jni_rootpath = (jstring)_jni_env->CallObjectMethod(_jni_file_object, _jni_get_absolute_path_method);
    const char* _rootpath_cstr = _jni_env->GetStringUTFChars(_jni_rootpath, nullptr);
    std::filesystem::path _rootpath(_rootpath_cstr);
    _jni_env->ReleaseStringUTFChars(_jni_rootpath, _rootpath_cstr);
    _jni_env->DeleteLocalRef(_jni_rootpath);
    _jni_env->DeleteLocalRef(_jni_file_class);
    _jni_env->DeleteLocalRef(_jni_file_object);
    _jni_env->DeleteLocalRef(_jni_activity_class);
    return _rootpath;
#endif

#if defined(__EMSCRIPTEN__)
    return "/data";
#endif

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    return std::filesystem::current_path() / "data";
#endif
}

[[nodiscard]] inline std::filesystem::path _resolve_path(const std::filesystem::path& relative)
{
    return _root_path() / relative;
}

}

void save_dialog(
    const std::function<void(std::ostream&)>& callback,
    const std::filesystem::path& default_path,
    const std::vector<std::pair<std::string, std::string>>& filters)
{
    if (!callback) {
        throw std::runtime_error("nullptr save_dialog callback");
    }
    std::string _filename = default_path.empty() ? "download.bin" : default_path.filename().string();

#if defined(__ANDROID__)
    (void)filters;
    JNIEnv* _jni_env = nullptr;
    _jni_jvm->AttachCurrentThread(&_jni_env, nullptr);
    jclass _jni_activity_class = _jni_env->GetObjectClass(_jni_activity);
    jmethodID _jni_save_file_method = _jni_env->GetMethodID(_jni_activity_class, "saveFileDialog", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring _jni_filename = _jni_env->NewStringUTF(_filename.c_str());
    jstring _jni_filepath = (jstring)_jni_env->CallObjectMethod(_jni_activity, _jni_save_file_method, _jni_filename);
    const char* _filepath_cstr = _jni_env->GetStringUTFChars(_jni_filepath, nullptr);
    std::string _filepath(_filepath_cstr);
    _jni_env->ReleaseStringUTFChars(_jni_filepath, _filepath_cstr);
    _jni_env->DeleteLocalRef(_jni_filename);
    _jni_env->DeleteLocalRef(_jni_filepath);
    _jni_env->DeleteLocalRef(_jni_activity_class);
    if (!_filepath.empty()) {
        std::ofstream _ostream(_filepath, std::ios::binary);
        if (_ostream.good()) {
            callback(_ostream);
        }
    }
#endif

#if defined(__EMSCRIPTEN__)
    (void)filters;
    std::ostringstream _ostream;
    callback(_ostream);
    std::string _data = _ostream.str();
    EM_ASM({
        const _name = UTF8ToString($0);
        const _ptr = $1;
        const _length = $2;
        const _blob = new Blob([HEAPU8.subarray(_ptr, _ptr + _length)], { type: "application/octet-stream" });
        const _element = document.createElement("a");
        _element.href = URL.createObjectURL(_blob);
        _element.download = _name;
        document.body.appendChild(_element);
        _element.click();
        document.body.removeChild(_element);
        URL.revokeObjectURL(_element.href); }, _filename.c_str(), reinterpret_cast<int>(_data.data()), static_cast<int>(_data.size()));

#endif

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    std::string _filters = _build_filters(filters);
    const char* _filters_cstr = _filters.empty() ? nullptr : _filters.c_str();
    nfdchar_t* _filepath_cstr = nullptr;
    if (NFD_SaveDialog(
            _filters_cstr,
            _filename.c_str(),
            &_filepath_cstr)
        != NFD_OKAY) {
        return;
    }
    std::ofstream _ostream(_filepath_cstr, std::ios::binary);
    free(_filepath_cstr);
    if (_ostream.good()) {
        callback(_ostream);
    }
#endif
}

void load_dialog(
    const std::function<void(std::istream&)>& callback,
    const std::filesystem::path& default_path,
    const std::vector<std::pair<std::string, std::string>>& filters)
{
    if (!callback) {
        throw std::runtime_error("nullptr load_dialog callback");
    }
    std::string _filters = _build_filters(filters);

#if defined(__ANDROID__)
    (default_path);
    JNIEnv* _jni_env = nullptr;
    _jni_jvm->AttachCurrentThread(&_jni_env, nullptr);
    jclass _jni_activity_class = _jni_env->GetObjectClass(_jni_activity);
    jmethodID _jni_open_file_method = _jni_env->GetMethodID(_jni_activity_class, "openFileDialog", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring _jni_filters = _jni_env->NewStringUTF(_filters.c_str());
    jstring _jni_filepath = (jstring)_jni_env->CallObjectMethod(_jni_activity, _jni_open_file_method, _jni_filters);
    const char* _filepath_cstr = _jni_env->GetStringUTFChars(_jni_filepath, nullptr);
    std::string _filepath(_filepath_cstr);
    _jni_env->ReleaseStringUTFChars(_jni_filepath, _filepath_cstr);
    _jni_env->DeleteLocalRef(_jni_filters);
    _jni_env->DeleteLocalRef(_jni_filepath);
    _jni_env->DeleteLocalRef(_jni_activity_class);
    if (!_filepath.empty()) {
        std::ifstream _istream(_filepath, std::ios::binary);
        if (_istream.good()) {
            callback(_istream);
        }
    }
#endif

#if defined(__EMSCRIPTEN__)
    (default_path);
    char* _path_cstr = _upload_file(_filters.c_str());
    if (_path_cstr) {
        std::ifstream _istream(_path_cstr, std::ios::binary);
        free(_path_cstr);
        if (_istream.good()) {
            callback(_istream);
        }
        // delete temp file
    }
#endif

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    const nfdchar_t* _filters_cstr = _filters.empty() ? nullptr : _filters.c_str();
    nfdchar_t* _path_cstr = nullptr;
    if (NFD_OpenDialog(
            _filters_cstr,
            default_path.empty() ? nullptr : default_path.string().c_str(),
            &_path_cstr)
        != NFD_OKAY) {
        return;
    }
    std::ifstream _istream(_path_cstr, std::ios::binary);
    free(_path_cstr);
    if (_istream.good()) {
        callback(_istream);
    }
#endif
}

void save_stream(
    const std::function<void(std::ostream&)>& callback,
    const std::filesystem::path& file_path)
{
    const std::filesystem::path _resolved_path = _resolve_path(file_path);
    std::ofstream _stream(_resolved_path, std::ios::binary);
    if (!_stream) {
        throw std::runtime_error("failed to open file for writing " + _resolved_path.string());
    }
    callback(_stream);

#if defined(__EMSCRIPTEN__)
    EM_ASM({
        FS.syncfs(function(_err) {
            if (_err)
                console.error(_err);
        });
    });
#endif
}

void load_stream(
    const std::function<void(std::istream&)>& callback,
    const std::filesystem::path& file_path)
{
    const std::filesystem::path _resolved_path = _resolve_path(file_path);
    if (std::filesystem::exists(_resolved_path)) {
        std::ifstream _stream(_resolved_path, std::ios::binary);
        if (_stream) {
            callback(_stream);
        }
    }
}