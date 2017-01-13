#pragma once
#include <map>
#include <string>
#include "repo_searcher.h"

class SettingsParser {
 public:
  void ParseSettings(std::string path,
                     std::map<std::string, RepoSearcher::library>& libraries);
};