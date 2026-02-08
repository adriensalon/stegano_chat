#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif
#if defined(_WIN32)
#include <nfd.h>
#endif

namespace {

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

EM_JS(void, _download_file, (const char* path, const char* filename), {
    const p = UTF8ToString(path);
    const name = UTF8ToString(filename);

    // Read from Emscripten FS (make sure you compiled with -sFORCE_FILESYSTEM=1)
    const data = FS.readFile(p); // Uint8Array

    const blob = new Blob([data], { type: 'application/octet-stream' });
    const url = URL.createObjectURL(blob);

    const a = document.createElement('a');
    a.href = url;
    a.download = name || p.split('/').pop(); // fallback to path basename
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);

    URL.revokeObjectURL(url);
});
#endif

#if defined(_WIN32)
static std::string _build_nfd_filter_string(const std::vector<std::pair<std::string, std::string>>& filters)
{
    // We ignore the human-readable name (first) and only use pattern (second).
    // Pattern is expected like "*.png;*.jpg" etc.
    std::string out;
    bool first = true;

    for (const auto& f : filters) {
        const std::string& pattern = f.second;
        std::size_t start = 0;

        while (start < pattern.size()) {
            std::size_t end = pattern.find(';', start);
            if (end == std::string::npos)
                end = pattern.size();

            std::string token = pattern.substr(start, end - start);

            // trim spaces
            auto trim_front = token.find_first_not_of(" \t");
            if (trim_front != std::string::npos)
                token.erase(0, trim_front);
            auto trim_back = token.find_last_not_of(" \t");
            if (trim_back != std::string::npos)
                token.erase(trim_back + 1);

            // remove "*." or "." prefix
            if (token.rfind("*.", 0) == 0)
                token.erase(0, 2);
            else if (!token.empty() && token[0] == '.')
                token.erase(0, 1);

            if (!token.empty()) {
                if (!first)
                    out += ",";
                out += token;
                first = false;
            }

            start = end + 1;
        }
    }

    return out;
}
#endif

}

bool import_file(
    std::filesystem::path& file_path,
    const std::filesystem::path& default_path,
    const std::vector<std::pair<std::string, std::string>>& filters)
{
#if defined(_WIN32)
    std::string filterList = _build_nfd_filter_string(filters);
    const nfdchar_t* filter_cstr = filterList.empty() ? nullptr : filterList.c_str();

    nfdchar_t* out_path = nullptr;

    nfdresult_t result = NFD_OpenDialog(
        filter_cstr,
        default_path.empty() ? nullptr : default_path.string().c_str(),
        &out_path);

    if (result == NFD_OKAY) {
        file_path = std::filesystem::path(out_path);
        free(out_path); // or NFD_FreePath if your NFD version provides it
        return true;
    }
    // NFD_CANCEL or NFD_ERROR → false
#endif
    return false;
}

bool export_file(
    std::filesystem::path& file_path,
    const std::filesystem::path& default_path,
    const std::vector<std::pair<std::string, std::string>>& filters)
{
#if defined(__EMSCRIPTEN__)
    std::string fs_path = file_path.string();
    std::string fname;
    if (!default_path.empty()) {
        fname = default_path.filename().string();
    } else {
        fname = file_path.filename().string();
    }
    _download_file(fs_path.c_str(), fname.c_str());
    return true;
#endif

#if defined(_WIN32)
    std::string filterList = _build_nfd_filter_string(filters);
    const nfdchar_t* filter_cstr = filterList.empty() ? nullptr : filterList.c_str();

    nfdchar_t* out_path = nullptr;

    nfdresult_t result = NFD_SaveDialog(
        filter_cstr,
        default_path.empty() ? nullptr : default_path.string().c_str(),
        &out_path);

    if (result == NFD_OKAY) {
        file_path = std::filesystem::path(out_path);
        free(out_path); // or NFD_FreePath
        return true;
    }
    return false;
#endif
}
