#pragma once

#include <functional>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

void save_dialog(
    const std::function<void(std::ostream&)>& callback,
    const std::filesystem::path& default_path = "",
    const std::vector<std::pair<std::string, std::string>>& filters = {});

void load_dialog(
    const std::function<void(std::istream&)>& callback,
    const std::filesystem::path& default_path = "",
    const std::vector<std::pair<std::string, std::string>>& filters = {});

void save_stream(
    const std::function<void(std::ostream&)>& callback,
    const std::filesystem::path& file_path);

void load_stream(
    const std::function<void(std::istream&)>& callback,
    const std::filesystem::path& file_path);
    