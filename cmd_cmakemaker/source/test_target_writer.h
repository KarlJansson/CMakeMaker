#pragma once
#include "repo_searcher.h"

class TestTargetWriter {
 public:
  TestTargetWriter() = default;
  ~TestTargetWriter() = default;

  void WriteTestTraget(std::string dir_name,
                       std::map<std::string, RepoSearcher::directory> &targets,
                       std::map<std::string, RepoSearcher::library> &libraries);

 protected:
 private:
};
