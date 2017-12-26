#pragma once
#include "repo_searcher.h"

class BenchmarktargetWriter {
 public:
  BenchmarktargetWriter() = default;
  ~BenchmarktargetWriter() = default;

  void WriteBenchmarkTarget(
      std::string dir_name,
      std::map<std::string, RepoSearcher::directory> &targets,
      std::map<std::string, RepoSearcher::library> &libraries);

 protected:
 private:
  bool MainUpdateNeeded(
      std::string dir_name,
      std::map<std::string, RepoSearcher::directory> &targets);
};
