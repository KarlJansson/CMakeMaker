#pragma once
#include "repo_searcher.h"

class CommonWriter {
 public:
  CommonWriter() = default;
  ~CommonWriter() = default;

  static void UpdateIfDifferent(std::string file_path, std::string &expected);
  static void AddMakeDirCommand(std::string dir_path, std::string base_dir,
                                bool config_inc, std::string &expected);
  static void AddCopyCommand(std::string file_path, std::string debug_suffix,
                             std::string ext, std::string source_base,
                             std::string dest_base, bool config_inc,
                             std::string &expected);
  static void WriteSourceGroups(RepoSearcher::directory &dir,
                                std::string &expected);

  static const std::vector<std::string> extensions_;
  static const std::vector<std::string> configs_;
  static const std::string cmake_header_;

 protected:
 private:
};
