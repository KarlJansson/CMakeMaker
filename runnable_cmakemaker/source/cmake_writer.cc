#include "cmake_writer.h"
#include <fstream>
#include <iterator>
#include <sstream>
#include "settings_parser.h"

CmakeWriter::CmakeWriter(RepoSearcher searcher) : searcher_(searcher) {
  SettingsParser parse;
  parse.ParseSettings("./cmakemaker", libraries_);
}

void CmakeWriter::WriteCmakeFiles() {
  auto& main_dir = searcher_.SearchPath("./");
  WriteMain(main_dir);
  std::vector<RepoSearcher::directory> targets;
  for (auto& dir : main_dir.directories)
    targets.push_back(searcher_.SearchPathSubdirs(dir));
  searcher_.FindDependencies(targets, libraries_);

  int i = 0;
  for (auto& dir : main_dir.directories) WriteSubdir(dir, targets[i++]);
}

void CmakeWriter::WriteMain(RepoSearcher::directory& dir) {
  std::ofstream open("./CMakeLists.txt");

  open << "########### CMakeMaker autogenerated file ###########" << std::endl;
  open << "cmake_minimum_required(VERSION 3.0)" << std::endl
       << "project(cmakemaker_solution C CXX)" << std::endl
       << std::endl;

  open << "set (LIBRARY_OUTPUT_PATH Build_Output/libs CACHE PATH \"Lib path\")"
       << std::endl
       << "set(EXECUTABLE_OUTPUT_PATH Build_Output/bin CACHE PATH \"Exe "
          "path\")"
       << std::endl
       << "mark_as_advanced(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)"
       << std::endl
       << "add_definitions(-DUNICODE -D_UNICODE)" << std::endl
       << std::endl;

  for (auto& subdir : dir.directories)
    open << "add_subdirectory(" << subdir << ")" << std::endl;

  open.close();
}

void CmakeWriter::WriteSubdir(std::string dir_name,
                              RepoSearcher::directory& dir) {
  std::ofstream open(dir_name + "/CMakeLists.txt");
  std::string proj_name =
      dir_name.substr(dir_name.find_last_of('/') + 1, dir_name.size());

  open << "########### CMakeMaker autogenerated file ###########" << std::endl;
  open << "cmake_minimum_required(VERSION 3.0)" << std::endl << std::endl;

  bool cuda_compile = dir.files["cu"].fmap.empty() ? false : true;
  std::vector<std::string> extensions = {"cc", "cpp", "h", "cu", "cuh"};

  if (cuda_compile) open << "find_package(CUDA)" << std::endl;
  for (auto& lib : dir.libraries) {
    auto& lib_info = libraries_[lib];
    if (!lib_info.find_command.empty()) {
      open << "find_package(" << libraries_[lib].find_command;
      if (!lib_info.components.empty()) {
        open << " COMPONENTS" << std::endl;
        for (auto& mod : lib_info.components) open << "  " << mod << std::endl;
      }
      open << ")";
    }
  }

  open << std::endl << std::endl;

  if (cuda_compile) {
    open << "set(CUDA_ATTACH_VS_BUILD_RULE_TO_CUDA_FILE OFF)" << std::endl
         << "set(CUDA_64_BIT_DEVICE_CODE ON)" << std::endl
         << "if(CUDA_FOUND)" << std::endl
         << "  add_definitions(-DCuda_Found)" << std::endl
         << "else(CUDA_FOUND)" << std::endl
         << "  message(\"Could not find the Cuda toolkit, the Cuda algorithms "
            "will not be built.\")"
         << std::endl
         << "endif(CUDA_FOUND)";
    open << std::endl << std::endl;
  }

  open << "set(cpp_files" << std::endl;
  for (auto& ext : extensions)
    for (auto& file : dir.files[ext].fmap)
      for (auto& pair : file.second) open << "  " << pair << std::endl;
  open << ")" << std::endl << std::endl;

  WriteSourceGroups(open, dir, extensions);

  if (dir_name.find("runnable_") != std::string::npos) {
    open << (cuda_compile ? "cuda_add_executable(" : "add_executable(");
    open << proj_name << " ${cpp_files})";
  } else if (dir_name.find("lib_") != std::string::npos) {
    open << (cuda_compile ? "cuda_add_library(" : "add_library(");
    open << proj_name << " SHARED ${cpp_files})";
  }
  open << std::endl << std::endl;

  open << "include_directories(" << proj_name << std::endl;
  for (auto& d : dir.directories) open << "  " << d << std::endl;
  for (auto& d : dir.include_dirs) open << "  " << d << std::endl;
  for (auto& lib : dir.libraries)
    open << "  " << libraries_[lib].include_dir << std::endl;
  if (cuda_compile)
    open << "  "
         << "${CUDA_INCLUDE_DIRS}" << std::endl;
  open << ")" << std::endl << std::endl;

  open << "target_link_libraries(" << proj_name << std::endl;
  for (auto& dep : dir.dependencies) open << "  " << dep << std::endl;
  for (auto& lib : dir.libraries)
    open << "  " << libraries_[lib].lib_dir << std::endl;
  if (cuda_compile)
    open << "  "
         << "${CUDA_LIBRARIES}" << std::endl;
  open << ")" << std::endl << std::endl;

  if (!dir.precomp_h.empty() && !dir.precomp_cc.empty()) {
    open << "if (MSVC)" << std::endl
         << "  set_target_properties(" << proj_name
         << " PROPERTIES COMPILE_FLAGS \"/Y"
         << dir.precomp_h.substr(dir.precomp_h.find_last_of('/') + 1,
                                 dir.precomp_h.size())
         << "\")" << std::endl
         << "  set_source_files_properties(" << dir.precomp_cc
         << " PROPERTIES COMPILE_FLAGS \"/Yc"
         << dir.precomp_cc.substr(dir.precomp_cc.find_last_of('/') + 1,
                                  dir.precomp_cc.size())
         << "\")" << std::endl
         << "endif(MSVC)";
  }

  open.close();
}

void CmakeWriter::WriteSourceGroups(std::ofstream& open,
                                    RepoSearcher::directory& dir,
                                    std::vector<std::string>& extensions) {
  for (auto& d : dir.directories) {
    auto group_name = d.substr(d.find_first_of('/') + 1, d.size());

    if (group_name.find('/') != std::string::npos) {
      std::vector<std::string> dir_names;
      while (group_name.find('/') != std::string::npos) {
        dir_names.push_back(
            group_name.substr(0, group_name.find_first_of('/')));
        group_name = group_name.substr(group_name.find_first_of('/') + 1,
                                       group_name.size());
      }
      dir_names.push_back(group_name);

      group_name = "";
      for (int i = 0; i < dir_names.size(); ++i)
        group_name += dir_names[i] + (i != dir_names.size() - 1 ? "\\\\" : "");
    }

    open << "source_group(" << group_name << " FILES" << std::endl;
    for (auto& ext : extensions)
      for (auto& file : dir.files[ext].fmap)
        if (file.first.compare(d) == 0)
          for (auto& pair : file.second) open << "  " << pair << std::endl;
    open << ")" << std::endl << std::endl;
  }
}
