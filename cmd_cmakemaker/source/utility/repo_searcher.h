#pragma once
#include <experimental/filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

class RepoSearcher {
 public:
  RepoSearcher();

  struct filemap {
    std::map<std::string, std::vector<std::string>> fmap;
  };

  struct library {
    std::string name;
    std::string find_command;
    std::vector<std::string> components;
    std::vector<std::string> includes;
    std::vector<std::string> libs;
    std::vector<std::string> dlls;
	std::vector<std::string> debug_dlls;
    std::string debug_suffix;
  };

  struct directory {
    std::string dir_name;
    std::string precomp_h;
    std::string precomp_cc;
    std::map<std::string, filemap> files;
    std::set<std::string> moc_files;
    std::set<std::string> libraries;
    std::set<std::string> dependencies;
    std::set<std::string> dependent;
    std::set<std::string> include_dirs;
    std::map<std::string, int> include_files;
    std::vector<std::string> directories;
  };

  directory SearchPath(std::string path);
  directory SearchPathSubdirs(std::string path);
  void FindDependencies(
      std::map<std::string, RepoSearcher::directory>& targets,
      std::map<std::string, RepoSearcher::library>& libraries);

 private:
  void ProcessFile(std::string& file, RepoSearcher::directory& target,
                   std::map<std::string, RepoSearcher::library>& libraries,
                   std::set<std::string>& include_files,
                   std::set<std::string>& all_headers, bool add_header);
  void CollectEntry(
      const std::experimental::filesystem::v1::directory_entry& entry,
      RepoSearcher::directory& dir, std::string& dir_name, bool subdir);

  int order_ = 0;
};
