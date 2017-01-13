#pragma once
#include "repo_searcher.h"

class CmakeWriter {
 public:
  CmakeWriter(RepoSearcher searcher);

  void WriteCmakeFiles();

 private:
  void WriteMain(RepoSearcher::directory& dir);
  void WriteSubdir(std::string dir_name, RepoSearcher::directory& dir);
  void WriteSourceGroups(std::ofstream& open, RepoSearcher::directory& dir,
                         std::vector<std::string>& extensions);

  RepoSearcher searcher_;
  std::map<std::string, RepoSearcher::library> libraries_;
};