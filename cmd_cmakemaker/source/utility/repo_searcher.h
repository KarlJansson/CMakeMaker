#pragma once
#include <experimental\filesystem>
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
    std::string include_dir;
    std::string lib_dir;
    std::string dll_file;
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
    std::vector<std::string> directories;
  };

  directory SearchPath(std::string path);
  directory SearchPathSubdirs(std::string path);
  void FindDependencies(
      std::map<std::string, RepoSearcher::directory>& targets,
      std::map<std::string, RepoSearcher::library>& libraries);

 private:
  void CollectEntry(
      const std::experimental::filesystem::v1::directory_entry& entry,
      RepoSearcher::directory& dir, std::string& dir_name, bool subdir);
};