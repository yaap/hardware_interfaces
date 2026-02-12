/*
 * Copyright 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bluetooth_hal/util/files.h"

#include <assert.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "android-base/logging.h"
#include "android-base/unique_fd.h"
#include "bluetooth_hal/util/logging.h"

namespace bluetooth_hal::util {
namespace {

using ::android::base::unique_fd;

// device node for Battery percentage.
constexpr char kBtteryPercentageNode[] = "/sys/class/power_supply/battery/capacity";

void HandleError(std::string_view temp_path, FILE** fp) {
    // This indicates there is a write issue.  Unlink as partial data is not
    // acceptable.
    unlink(std::string(temp_path).c_str());
    if (*fp) {
        fclose(*fp);
        *fp = nullptr;
    }
}

}  // namespace

bool GetFsDebugDump(int fd, std::string_view debugfs) {
    std::stringstream ss;
    std::ifstream file;

    ss << "=============================================" << std::endl;
    ss << "Debugfs:" << debugfs << std::endl;
    ss << "=============================================" << std::endl;
    file.open(std::string(debugfs));
    if (file.is_open()) {
        ss << file.rdbuf() << std::endl;
    } else {
        ss << "Fail to read debugfs: " << debugfs << std::endl;
    }
    ss << std::endl;
    write(fd, ss.str().c_str(), ss.str().length());
    return true;
}

bool GetBatteryPercentage(std::string& batt_level) {
    unique_fd batt_ctl_fd(open(kBtteryPercentageNode, O_CREAT | O_RDONLY, S_IRGRP));
    if (!batt_ctl_fd.ok()) {
        LOG(ERROR) << __func__ << ": Unable to open Bttery Percentage device node ("
                   << kBtteryPercentageNode << "): " << strerror(errno) << " (" << errno << ").";
        return false;
    }
    ssize_t length;
    char buffer[4] = {};
    length = TEMP_FAILURE_RETRY(read(batt_ctl_fd.get(), &buffer, sizeof(buffer) - 1));

    if (length < 1) {
        return false;
    }
    if (buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    }
    batt_level.assign(buffer);

    return true;
}

std::string GetLastLogPath(std::string_view log_file_path) {
    return std::string(log_file_path).append(".last");
}

void CreateLogFile(std::string_view log_file_path, std::ofstream& log_file_stream) {
    LOG(INFO) << __func__ << ": log_file_path: " << log_file_path << ".";
    std::string last_file_path = GetLastLogPath(log_file_path);

    if (FileExists(log_file_path)) {
        // Change the file's permissions to OWNER Read/Write, GROUP Read, OTHER Read
        if (chmod(last_file_path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
            LOG(ERROR) << __func__ << ": Unable to change file permissions " << last_file_path
                       << ".";
        }
        if (!RenameFile(log_file_path, last_file_path)) {
            LOG(ERROR) << __func__ << ": Unable to rename existing snoop log from \""
                       << log_file_path << "\" to \"" << last_file_path << "\".";
        }
    } else {
        LOG(INFO) << __func__ << ": Previous log file \"" << log_file_path
                  << "\" does not exist, skip renaming.";
    }

    // do not use std::ios::app as we want override the existing file
    log_file_stream.open(std::string(log_file_path), std::ios::out);

    // Change the file's permissions to OWNER Read/Write, GROUP Read, OTHER Read
    if (chmod(std::string(log_file_path).c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
        LOG(ERROR) << __func__ << ": Unable to change file permissions " << log_file_path << ".";
    }
    if (!log_file_stream.good()) {
        LOG(ERROR) << __func__ << ": Unable to open log at \"" << log_file_path << "\", error: \""
                   << strerror(errno) << "\".";
    }
    log_file_stream << Logger::GetFileFormatTimestamp() << std::endl;
}

void CloseLogFileStream(std::ofstream& log_file_stream) {
    if (log_file_stream.is_open()) {
        log_file_stream.flush();
        log_file_stream.close();
    }
}

bool FileExists(std::string_view path) {
    std::ifstream input(std::string(path), std::ios::binary | std::ios::ate);
    return input.good();
}

bool RenameFile(std::string_view from, std::string_view to) {
    if (std::rename(std::string(from).c_str(), std::string(to).c_str()) != 0) {
        LOG(ERROR) << __func__ << ": Unable to rename file from '" << from << "' to '" << to
                   << "', error: " << strerror(errno) << ".";
        return false;
    }
    return true;
}

std::optional<std::string> ReadSmallFile(std::string_view path) {
    std::ifstream input(std::string(path), std::ios::binary | std::ios::ate);
    if (!input) {
        LOG(WARNING) << __func__ << ": Failed to open file '" << path
                     << "', error: " << strerror(errno) << ".";
        return std::nullopt;
    }
    int file_size = input.tellg();
    if (file_size < 0) {
        LOG(WARNING) << __func__ << ": Failed to get file size for '" << path
                     << "', error: " << strerror(errno) << ".";
        return std::nullopt;
    }
    std::string result(file_size, '\0');
    if (!input.seekg(0)) {
        LOG(WARNING) << __func__ << ": Failed to go back to the beginning of file '" << path
                     << "', error: " << strerror(errno) << ".";
        return std::nullopt;
    }
    if (!input.read(result.data(), result.size())) {
        LOG(WARNING) << __func__ << ": Failed to read file '" << path
                     << "', error: " << strerror(errno) << ".";
        return std::nullopt;
    }
    input.close();
    return result;
}

bool WriteToFile(std::string_view path, std::string_view data) {
    // TBD: ASSERT(!path.empty());
    // Steps to ensure content of data gets to disk:
    //
    // 1) Open and write to temp file (e.g. bt_config.conf.new).
    // 2) Flush the stream buffer to the temp file.
    // 3) Sync the temp file to disk with fsync().
    // 4) Rename temp file to actual config file (e.g. bt_config.conf).
    //    This ensures atomic update.
    // 5) Sync directory that has the conf file with fsync().
    //    This ensures directory entries are up-to-date.
    //
    // We are using traditional C type file methods because C++ std::filesystem
    // and std::ofstream do not support:
    // - Operation on directories
    // - fsync() to ensure content is written to disk

    // Build temp config file based on config file (e.g. bt_config.conf.new).
    const std::string temp_path = std::string(path) + ".new";

    // Extract directory from file path (e.g. /data/misc/bluedroid).
    // libc++fs is not supported in APEX yet and hence cannot use
    // std::filesystem::path::parent_path
    std::string directory_path;
    {
        // Make a temporary variable as inputs to dirname() will be modified and
        // return value points to input char array temp_path_for_dir must not be
        // destroyed until results from dirname is appended to directory_path
        std::string temp_path_for_dir(path);
        directory_path.append(dirname(temp_path_for_dir.data()));
    }
    if (directory_path.empty()) {
        LOG(ERROR) << __func__ << ": Error extracting directory from '" << path
                   << "', error: " << strerror(errno) << ".";
        return false;
    }

    unique_fd dir_fd(open(directory_path.c_str(), O_RDONLY | O_DIRECTORY));
    if (!dir_fd.ok()) {
        LOG(ERROR) << __func__ << ": Unable to open dir '" << directory_path
                   << "', error: " << strerror(errno) << ".";
        return false;
    }

    FILE* fp = std::fopen(temp_path.c_str(), "wt");
    if (!fp) {
        LOG(ERROR) << __func__ << ": Unable to write to file '" << temp_path
                   << "', error: " << strerror(errno) << ".";
        HandleError(temp_path, &fp);
        return false;
    }

    if (std::fprintf(fp, "%s", std::string(data).c_str()) < 0) {
        LOG(ERROR) << __func__ << ": Unable to write to file '" << temp_path
                   << "', error: " << strerror(errno) << ".";
        HandleError(temp_path, &fp);
        return false;
    }

    // Flush the stream buffer to the temp file.
    if (std::fflush(fp) != 0) {
        LOG(ERROR) << __func__ << ": Unable to write flush buffer to file '" << temp_path
                   << "', error: " << strerror(errno) << ".";
        HandleError(temp_path, &fp);
        return false;
    }

    // Sync written temp file out to disk. fsync() is blocking until data makes it
    // to disk.
    if (fsync(fileno(fp)) != 0) {
        LOG(WARNING) << __func__ << ": Unable to fsync file '" << temp_path
                     << "', error: " << strerror(errno) << ".";
        // Allow fsync to fail and continue
    }

    if (std::fclose(fp) != 0) {
        LOG(ERROR) << __func__ << ": Unable to close file '" << temp_path
                   << "', error: " << strerror(errno) << ".";
        HandleError(temp_path, &fp);
        return false;
    }
    fp = nullptr;

    // Change the file's permissions to Read/Write by User and Group
    if (chmod(temp_path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) != 0) {
        LOG(ERROR) << __func__ << ": Unable to change file permissions '" << temp_path
                   << "', error: " << strerror(errno) << ".";
        HandleError(temp_path, &fp);
        return false;
    }

    // Rename written temp file to the actual config file.
    if (std::rename(temp_path.c_str(), std::string(path).c_str()) != 0) {
        LOG(ERROR) << __func__ << ": Unable to commit file from '" << temp_path << "' to '" << path
                   << "', error: " << strerror(errno) << ".";
        HandleError(temp_path, &fp);
        return false;
    }

    // This should ensure the directory is updated as well.
    if (fsync(dir_fd.get()) != 0) {
        LOG(WARNING) << __func__ << ": Unable to fsync dir '" << directory_path
                     << "', error: " << strerror(errno) << ".";
    }

    return true;
}

bool RemoveFile(std::string_view path) {
    if (remove(std::string(path).c_str()) != 0) {
        LOG(ERROR) << __func__ << ": Unable to remove file '" << path
                   << "', error: " << strerror(errno) << ".";
        return false;
    }
    return true;
}

std::optional<std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>>
FileCreatedTime(std::string_view path) {
    struct stat file_info;
    if (stat(std::string(path).c_str(), &file_info) != 0) {
        LOG(ERROR) << __func__ << ": Unable to read '" << path
                   << "' file metadata, error: " << strerror(errno) << ".";
        return std::nullopt;
    }

    timespec created_ts = file_info.st_ctim;
    std::chrono::duration d =
            std::chrono::seconds{created_ts.tv_sec} + std::chrono::nanoseconds{created_ts.tv_nsec};

    return std::chrono::time_point<std::chrono::system_clock>(
            duration_cast<std::chrono::system_clock::duration>(d));
}

void DeleteOldestFiles(std::string_view directory, std::optional<std::string_view> file_prefix,
                       size_t files_to_keep) {
    LOG(INFO) << __func__ << " (directory: " << directory
              << ", file_prefix: " << (file_prefix.has_value() ? file_prefix.value() : "")
              << ", files_to_keep: " << files_to_keep << ")";
    std::vector<std::filesystem::directory_entry> files;

    // Collect all regular files in the directory
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string filename = entry.path().filename().string();
        if (!file_prefix.has_value() || filename.starts_with(file_prefix.value())) {
            files.emplace_back(entry);
        }
    }

    // Sort files by their last write time
    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
        return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
    });

    // Delete files, starting at starting_point
    for (size_t i = files_to_keep; i < files.size(); ++i) {
        std::filesystem::remove(files[i]);
        LOG(INFO) << "Deleted: " << files[i].path().c_str();
    }
}

}  // namespace bluetooth_hal::util
