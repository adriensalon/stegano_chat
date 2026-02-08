#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#if defined(__EMSCRIPTEN__)
#include <atomic>
#include <emscripten.h>
#endif

#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
#include <nfd.h>
#endif

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

EM_JS(void, _download_file, (const char* filepath), {
    const _path = UTF8ToString(filepath);
    const _data = FS.readFile(_path);
    const _blob = new Blob([_data], { type: 'application/octet-stream' });
    const _url = URL.createObjectURL(_blob);
    const _element = document.createElement('a');
    _element.href = _url;
    _element.download = _path.split('/').pop();
    document.body.appendChild(_element);
    _element.click();
    document.body.removeChild(_element);
    URL.revokeObjectURL(_url);
});
#endif

[[nodiscard]] static std::string _build_filters(const std::vector<std::pair<std::string, std::string>>& filters)
{
    std::string _filters;

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

#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
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

}

bool import_file(
    std::filesystem::path& file_path,
    const std::filesystem::path& default_path,
    const std::vector<std::pair<std::string, std::string>>& filters)
{
    std::string _filters = _build_filters(filters);

#if defined(__EMSCRIPTEN__)
    char* _path_cstr = _upload_file(_filters.c_str());
    if (!_path_cstr) {
        return false;
    }
    file_path = std::filesystem::path(_path_cstr);
    free(_path_cstr);
    return true;
#endif

#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
    const nfdchar_t* _filters_cstr = _filters.empty() ? nullptr : _filters.c_str();
    nfdchar_t* _path_cstr = nullptr;
    if (NFD_OpenDialog(
            _filters_cstr,
            default_path.empty() ? nullptr : default_path.string().c_str(),
            &_path_cstr)
        != NFD_OKAY) {
        return false;
    }
    file_path = std::filesystem::path(_path_cstr);
    free(_path_cstr);
    return true;
#endif
}

bool export_file(
    std::filesystem::path& file_path,
    const std::filesystem::path& default_path,
    const std::vector<std::pair<std::string, std::string>>& filters)
{
#if defined(__EMSCRIPTEN__)
    if (default_path.empty()) {
        return false;
    }
    file_path = default_path.filename();
    return true;
#endif

#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
    std::string _filters = _build_filters(filters);
    const nfdchar_t* _filters_cstr = _filters.empty() ? nullptr : _filters.c_str();
    nfdchar_t* _path_cstr = nullptr;
    if (NFD_SaveDialog(
            _filters_cstr,
            default_path.empty() ? nullptr : default_path.string().c_str(),
            &_path_cstr)
        != NFD_OKAY) {
        return false;
    }
    file_path = std::filesystem::path(_path_cstr);
    free(_path_cstr);
    return true;
#endif
}
