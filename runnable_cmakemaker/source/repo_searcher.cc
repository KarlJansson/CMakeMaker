#include "repo_searcher.h"
#include <fstream>
#include <sstream>

RepoSearcher::RepoSearcher() {}

RepoSearcher::directory RepoSearcher::SearchPath(std::string path) {
  directory result;
  result.dir_name = path;
  for (auto& p : std::experimental::filesystem::directory_iterator(path))
    CollectEntry(p, result, path, false);
  return result;
}

RepoSearcher::directory RepoSearcher::SearchPathSubdirs(std::string path) {
  directory result;
  result.dir_name = path;
  for (auto& p :
       std::experimental::filesystem::recursive_directory_iterator(path))
    CollectEntry(p, result, path, true);
  return result;
}

void RepoSearcher::CollectEntry(
    const std::experimental::filesystem::v1::directory_entry& entry,
    RepoSearcher::directory& dir, std::string& dir_name, bool subdir) {
  std::stringstream path_stream;
  path_stream << entry;
  std::string path = path_stream.str();
  std::replace(path.begin(), path.end(), '\\', '/');

  if (dir_name.compare("./") != 0) {
    std::string::size_type i = path.find(
        dir_name.substr(dir_name.find_first_of('/'), dir_name.size()));
    if (i != std::string::npos) path.erase(i, dir_name.length() - 1);
  }

  if (std::experimental::filesystem::is_directory(entry)) {
    if (path.find("runnable_") != std::string::npos ||
        path.find("lib_") != std::string::npos || subdir)
      dir.directories.emplace_back(path);
  } else if (std::experimental::filesystem::is_regular_file(entry)) {
    std::string ext = path.substr(path.find_last_of('.') + 1, path.size());
    dir.files[ext].fmap[path.substr(0, path.find_last_of('/'))].emplace_back(
        path);
    if (path.find("precomp.h") != std::string::npos)
      dir.precomp_h = path;
    else if (path.find("precomp.") != std::string::npos)
      dir.precomp_cc = path;
  }
}

void RepoSearcher::FindDependencies(
    std::vector<RepoSearcher::directory>& targets,
	std::map<std::string, RepoSearcher::library>& libraries) {
  std::vector<std::set<std::string>> include_files;
  for (auto& target : targets) {
    include_files.push_back(std::set<std::string>());
    for (auto& map : target.files) {
      for (auto& file : map.second.fmap) {
        for (auto& pair : file.second) {
          std::ifstream open(target.dir_name +
                             pair.substr(pair.find_first_of('.'), pair.size()));
          while (!open.fail() && !open.eof()) {
            std::string line = "";
            std::getline(open, line);
            if (line.find("#include") != std::string::npos) {
              for (auto& lib : libraries)
                if (line.find(lib.first) != std::string::npos)
                  target.libraries.insert(lib.first);
              std::string stripped_name;
              if (line.find('<') != std::string::npos)
                stripped_name = line.substr(
                    line.find_first_of('<') + 1,
                    line.find_last_of('>') - line.find_first_of('<') - 1);
              else
                stripped_name = line.substr(
                    line.find_first_of('"') + 1,
                    line.find_last_of('"') - line.find_first_of('"') - 1);
              include_files.back().insert(stripped_name);
            }
          }
          open.close();
        }
      }
    }
  }

  for (auto& target : targets) {
    int i = 0;
    for (auto& target_insert : targets) {
      if (target.dir_name.compare(target_insert.dir_name) == 0) {
        ++i;
        continue;
      }
      for (auto& map : target.files) {
        for (auto& fmap : map.second.fmap) {
          for (auto& file : fmap.second) {
            if (include_files[i].find(
                    file.substr(file.find_last_of('/') + 1, file.size())) !=
                include_files[i].end()) {
              target_insert.dependencies.insert(
                  target.dir_name.substr(target.dir_name.find_last_of('/') + 1,
                                         target.dir_name.size()));

              auto incdir = file.substr(0, file.find_last_of('/'));
              incdir =
                  "." + target.dir_name +
                  incdir.substr(incdir.find_first_of('.') + 1, incdir.size());
              target_insert.include_dirs.insert(incdir);
            }
          }
        }
      }
      ++i;
    }
  }
}
