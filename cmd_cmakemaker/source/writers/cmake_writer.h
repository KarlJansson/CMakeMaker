#pragma once
#include "repo_searcher.h"
#include "common_writer.h"

class CmakeWriter {
 public:
  CmakeWriter(RepoSearcher searcher);

  void WriteCmakeFiles();

 private:
  void WriteMain(RepoSearcher::directory& dir);

  RepoSearcher searcher_;
  RepoSearcher::directory main_dir_;
  std::map<std::string, RepoSearcher::directory> targets_;
  std::map<std::string, RepoSearcher::library> libraries_;
};