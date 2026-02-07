#pragma once

bool import_file(
    std::filesystem::path& file_path,
    const std::filesystem::path& default_path = "",
    const std::vector<std::pair<std::string, std::string>>& filters = {});

bool export_file(
    std::filesystem::path& file_path,
    const std::filesystem::path& default_path = "",
    const std::vector<std::pair<std::string, std::string>>& filters = {});