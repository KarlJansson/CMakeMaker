#pragma once
#include "repo_searcher.h"

class SubdirWriter {
 public:
  SubdirWriter() = default;
  ~SubdirWriter() = default;

  void WriteSubdir(std::string dir_name, RepoSearcher::directory &dir,
                   std::map<std::string, RepoSearcher::directory> &targets,
                   std::map<std::string, RepoSearcher::library> &libraries);

 protected:
 private:
};
