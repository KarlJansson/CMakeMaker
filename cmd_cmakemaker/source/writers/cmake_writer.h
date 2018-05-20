#pragma once
#include "common_writer.h"
#include "repo_searcher.h"

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
  std::map<std::string, std::string> settings_;
};
