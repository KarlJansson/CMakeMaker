#include "precomp.h"

#include "repo_searcher.h"

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

void RepoSearcher::FindDependencies(
    std::map<std::string, RepoSearcher::directory>& targets,
    std::map<std::string, RepoSearcher::library>& libraries) {
  bool add_header;
  std::set<std::string> all_headers;
  std::vector<std::set<std::string>> include_files;
  for (auto& target : targets) {
    include_files.push_back(std::set<std::string>());
    for (auto& map : target.second.files) {
      add_header = map.first.compare("h") == 0 || map.first.compare("hpp") == 0;

      for (auto& pair : map.second.fmap)
        for (auto& file : pair.second)
          ProcessFile(file, target.second, libraries, include_files.back(),
                      all_headers, add_header);
    }
  }

  for (auto& target : targets)
    for (auto& file : all_headers) target.second.include_files.erase(file);

  for (auto& target : targets) {
    int i = 0;
    for (auto& target_insert : targets) {
      if (target.second.dir_name.compare(target_insert.second.dir_name) == 0) {
        ++i;
        continue;
      }
      for (auto& map : target.second.files) {
        for (auto& fmap : map.second.fmap) {
          for (auto& file : fmap.second) {
            if (include_files[i].find(
                    file.substr(file.find_last_of('/') + 1, file.size())) !=
                include_files[i].end()) {
              if (target.second.dir_name.find("runnable_") ==
                      std::string::npos &&
                  target.second.dir_name.find("cmd_") == std::string::npos &&
                  target.second.dir_name.find("app_") == std::string::npos) {
                target_insert.second.dependencies.insert(
                    target.second.dir_name.substr(
                        target.second.dir_name.find_last_of('/') + 1,
                        target.second.dir_name.size()));
                if (target.second.dir_name.find("slib_") == std::string::npos)
                  target.second.dependent.insert(
                      target_insert.second.dir_name.substr(
                          target_insert.second.dir_name.find_last_of('/') + 1,
                          target_insert.second.dir_name.size()));
              }

              auto incdir = file.substr(0, file.find_last_of('/'));
              incdir =
                  "." + target.second.dir_name +
                  incdir.substr(incdir.find_first_of('.') + 1, incdir.size());
              target_insert.second.include_dirs.insert(incdir);
            }
          }
        }
      }
      ++i;
    }
  }
}

void RepoSearcher::ProcessFile(
    std::string& file, RepoSearcher::directory& target,
    std::map<std::string, RepoSearcher::library>& libraries,
    std::set<std::string>& include_files, std::set<std::string>& all_headers,
    bool add_header) {
  std::string q_obj_string = "Q_";
  q_obj_string += "OBJECT";
  std::string include_encap;
  std::ifstream open(target.dir_name +
                     file.substr(file.find_first_of('.'), file.size()));

  while (!open.fail() && !open.eof()) {
    std::string line;
    std::getline(open, line);

    if (line.find("#include") != std::string::npos) {
      for (auto& lib : libraries)
        if (line.find(lib.first) != std::string::npos)
          target.libraries.insert(lib.first);

      std::string stripped_name;
      if (line.find('<') != std::string::npos) {
        stripped_name =
            line.substr(line.find_first_of('<'),
                        line.find_last_of('>') - line.find_first_of('<') + 1);
        include_encap = "<>";
      } else if (line.find('"') != std::string::npos) {
        stripped_name =
            line.substr(line.find_first_of('"'),
                        line.find_last_of('"') - line.find_first_of('"') + 1);
        include_encap = "\"\"";
      } else
        continue;

      if (!stripped_name.empty() && stripped_name.size() > 1 &&
          stripped_name[0] == include_encap[0] &&
          stripped_name.back() == include_encap[1] &&
          stripped_name.find("#include") == std::string::npos) {
        if (stripped_name.find("precomp") == std::string::npos) {
          include_files.insert(
              stripped_name.substr(1, stripped_name.size() - 2));
          if (line[0] == '#') target.include_files[stripped_name] = order_++;
        }
      }
    }

    if (line.find(q_obj_string) != std::string::npos)
      target.moc_files.insert(file);
  }
  open.close();

  if (add_header)
    all_headers.insert(
        "\"" + file.substr(file.find_last_of('/') + 1, file.size()) + "\"");
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
        path.find("cmd_") != std::string::npos ||
        path.find("app_") != std::string::npos ||
        path.find("test_") != std::string::npos ||
        path.find("benchmark_") != std::string::npos ||
        path.find("slib_") != std::string::npos ||
        path.find("dlib_") != std::string::npos ||
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