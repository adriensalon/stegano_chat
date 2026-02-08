#pragma once

bool save_dialog(
    std::filesystem::path& file_path,
    const std::filesystem::path& default_path = "",
    const std::vector<std::pair<std::string, std::string>>& filters = {});

bool load_dialog(
    std::filesystem::path& file_path,
    const std::filesystem::path& default_path = "",
    const std::vector<std::pair<std::string, std::string>>& filters = {});
