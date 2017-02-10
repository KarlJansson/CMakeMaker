#include "cmake_writer.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include "settings_parser.h"

CmakeWriter::CmakeWriter(RepoSearcher searcher) : searcher_(searcher) {
  SettingsParser parse;
  parse.ParseSettings("./cmakemaker", libraries_);
}

void CmakeWriter::WriteCmakeFiles() {
  main_dir_ = searcher_.SearchPath("./");
  WriteMain(main_dir_);
  for (auto& dir : main_dir_.directories)
    targets_[dir] = searcher_.SearchPathSubdirs(dir);
  searcher_.FindDependencies(targets_, libraries_);

  for (auto& dir : main_dir_.directories) WriteSubdir(dir, targets_[dir]);
}

void CmakeWriter::WriteMain(RepoSearcher::directory& dir) {
  std::ofstream open("./CMakeLists.txt");

  open << "########### CMakeMaker autogenerated file ###########" << std::endl;
  open << "cmake_minimum_required(VERSION 3.0)" << std::endl
       << "project(cmakemaker_solution C CXX)" << std::endl
       << std::endl
       << std::endl;

  open
      << "macro(add_msvc_precompiled_header PrecompiledHeader "
         "PrecompiledSource SourcesVar)\n"
      << "  if(MSVC)\n"
      << "    get_filename_component(PrecompiledBasename ${PrecompiledHeader} "
         "NAME_WE)\n"
      << "    set(PrecompiledBinary \"$(IntDir)/${PrecompiledBasename}.pch\")\n"
      << "    set(Sources ${${SourcesVar}})\n"
      << "    set_source_files_properties(${Sources}\n"
      << "      PROPERTIES COMPILE_FLAGS \"/Yu\\\"${PrecompiledHeader}\\\" "
         "/FI\\\"${PrecompiledHeader}\\\" /Fp\\\"${PrecompiledBinary}\\\"\"\n"
      << "      OBJECT_DEPENDS \"${PrecompiledBinary}\")\n"
      << "    set_source_files_properties(${PrecompiledSource}\n"
      << "      PROPERTIES COMPILE_FLAGS \"/Yc\\\"${PrecompiledHeader}\\\" "
         "/FI\\\"${PrecompiledHeader}\\\" /Fp\\\"${PrecompiledBinary}\\\"\"\n"
      << "      OBJECT_OUTPUTS \"${PrecompiledBinary}\")\n"
      << "    list(APPEND ${SourcesVar} ${PrecompiledSource})\n"
      << "  endif(MSVC)\n"
      << "endmacro(add_msvc_precompiled_header)\n\n";

  open << "set (LIBRARY_OUTPUT_PATH Build_Output/libs CACHE PATH \"Lib path\")"
       << std::endl
       << "set(EXECUTABLE_OUTPUT_PATH Build_Output/bin CACHE PATH \"Exe "
          "path\")"
       << std::endl
       << "mark_as_advanced(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)"
       << std::endl
       << "add_definitions(-DUNICODE -D_UNICODE)" << std::endl
       << std::endl
       << "if (WIN32)" << std::endl
       << "  add_definitions(-DWindowsBuild)" << std::endl
       << "  set(CMAKE_CXX_FLAGS_RELEASE  \"${CMAKE_CXX_FLAGS_RELEASE} -MP\")"
       << std::endl
       << "else(WIN32)" << std::endl
       << "  add_definitions(-DUnixBuild)" << std::endl
       << "  set(CMAKE_CXX_FLAGS  \"${CMAKE_CXX_FLAGS} -fPIC\")" << std::endl
       << "endif(WIN32)" << std::endl
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
  proj_name = dir_name.substr(dir_name.find_first_of('_') + 1, dir_name.size());
  std::vector<std::string> precomp_file_bundles, proj_names;
  proj_names.push_back(proj_name);

  std::vector<std::string> configs = {"Release", "Debug", "MinSizeRel",
                                      "RelWithDebInfo"};

  open << "########### CMakeMaker autogenerated file ###########" << std::endl;
  open << "cmake_minimum_required(VERSION 3.0)" << std::endl << std::endl;

  bool cuda_compile = dir.files["cu"].fmap.empty() ? false : true;
  std::vector<std::string> extensions = {"cc", "cpp", "h", "cu", "cuh", "qml"};

  if (cuda_compile) open << "find_package(CUDA)" << std::endl;
  for (auto& lib : dir.libraries) {
    auto& lib_info = libraries_[lib];
    if (!lib_info.find_command.empty()) {
      open << "find_package(" << libraries_[lib].find_command;
      if (!lib_info.components.empty()) {
        open << " COMPONENTS" << std::endl;
        for (auto& mod : lib_info.components) open << "  " << mod << std::endl;
      }
      open << ")\n";
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

  if (!dir.moc_files.empty()) {
    open << "qt5_wrap_cpp(moc_files\n";
    for (auto& file : dir.moc_files) open << "  " + file + "\n";
    open << ")\n\n";

    if (!dir.precomp_h.empty() && !dir.precomp_cc.empty())
      precomp_file_bundles.push_back("moc_files");
  }

  if (!dir.precomp_h.empty() && !dir.precomp_cc.empty())
    precomp_file_bundles.push_back("cpp_files");

  for (auto& s : precomp_file_bundles)
    open << "add_msvc_precompiled_header(\"" +
                dir.precomp_h.substr(dir.precomp_h.find_last_of('/') + 1,
                                     dir.precomp_h.size()) +
                "\" "
                "\"" +
                dir.precomp_cc + "\" " + s + ")\n";

  if (dir_name.find("lib_") != std::string::npos &&
      dir_name.find("slib_") == std::string::npos &&
      dir_name.find("dlib_") == std::string::npos)
    proj_names.push_back("static_" + proj_name);

  int proj_count = 0;
  for (auto& p_name : proj_names) {
    if (dir_name.find("runnable_") != std::string::npos ||
        dir_name.find("cmd_") != std::string::npos ||
        dir_name.find("app_") != std::string::npos) {
      open << (cuda_compile ? "cuda_add_executable(" : "add_executable(");
      open << p_name;
      if (dir_name.find("app_") != std::string::npos) open << " WIN32";
    } else if (dir_name.find("lib_") != std::string::npos ||
               dir_name.find("slib_") != std::string::npos ||
               dir_name.find("dlib_") != std::string::npos) {
      open << (cuda_compile ? "cuda_add_library(" : "add_library(");
      open << p_name;
      if (dir_name.find("slib_") != std::string::npos)
        open << " STATIC";
      else if (dir_name.find("dlib_") != std::string::npos)
        open << " SHARED";
      else if (proj_count++ == 0)
        open << " SHARED";
      else
        open << " STATIC";
    }
    open << " ${cpp_files}";
    if (!dir.moc_files.empty()) open << " ${moc_files}";
    if (dir.files.find("qrc") != dir.files.end()) open << " ${qt_resources}";
    open << ")" << std::endl;
  }
  open << std::endl;

  for (auto& p_name : proj_names) {
    open << "include_directories(" << p_name << std::endl;
    for (auto& d : dir.directories)
      if (!d.empty()) open << "  " << d << std::endl;
    for (auto& d : dir.include_dirs)
      if (!d.empty()) open << "  " << d << std::endl;
    for (auto& lib : dir.libraries)
      if (!libraries_[lib].include_dir.empty())
        open << "  " << libraries_[lib].include_dir << std::endl;
    if (cuda_compile)
      open << "  "
           << "${CUDA_INCLUDE_DIRS}" << std::endl;
    open << ")" << std::endl << std::endl;

    open << "target_link_libraries(" << p_name << std::endl;
    for (auto& dep : dir.dependencies)
      open << "  " << dep.substr(dep.find_first_of('_') + 1, dep.size())
           << std::endl;
    for (auto& lib : dir.libraries)
      open << "  " << libraries_[lib].lib_dir << std::endl;
    if (cuda_compile)
      open << "  "
           << "${CUDA_LIBRARIES}" << std::endl;
    open << ")" << std::endl << std::endl;

    if (p_name.find("static_") == std::string::npos && !dir.dependent.empty()) {
      open << "add_custom_command(TARGET " + p_name + " POST_BUILD\n";
      for (auto& dep : dir.dependent) {
        open << "  COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different\n";
        open << "    \"";
        for (auto& conf : configs)
          open << "$<$<CONFIG:" + conf +
                      ">:${CMAKE_CURRENT_BINARY_DIR}/Build_Output/libs/" +
                      conf + "/" + p_name + ".dll>";
        open << "\"\n    \"";
        for (auto& conf : configs)
          open << "$<$<CONFIG:" + conf + ">:${CMAKE_CURRENT_BINARY_DIR}/../" +
                      dep + "/Build_Output/bin/" + conf + "/" + p_name +
                      ".dll>";
        open << "\"\n";
      }
      open << ")\n\n";
    }

    if (dir_name.find("runnable_") != std::string::npos ||
        dir_name.find("cmd_") != std::string::npos ||
        dir_name.find("app_") != std::string::npos) {
      std::set<std::string> libs;
      for (auto& dep : dir.dependencies)
        for (auto& lib : targets_["./" + dep].libraries)
          if (!libraries_[lib].dll_file.empty()) libs.insert(lib);

      for (auto& lib : dir.libraries)
        if (!libraries_[lib].dll_file.empty()) libs.insert(lib);

      if (!libs.empty())
        open << "add_custom_command(TARGET " + p_name + " POST_BUILD\n";
      for (auto& lib : libs) {
        open << "  COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different\n";
        open << "    \"";
        for (auto& conf : configs)
          open << "$<$<CONFIG:" + conf + ">:" + libraries_[lib].dll_file +
                      (conf.compare("Debug") == 0 ? libraries_[lib].debug_suffix
                                                  : "") +
                      ".dll>";
        open << "\"\n    \"";
        for (auto& conf : configs)
          open << "$<$<CONFIG:" + conf +
                      ">:${CMAKE_CURRENT_BINARY_DIR}/Build_Output/bin/" + conf +
                      "/" +
                      libraries_[lib].dll_file.substr(
                          libraries_[lib].dll_file.find_last_of('/') + 1,
                          libraries_[lib].dll_file.size()) +
                      (conf.compare("Debug") == 0 ? libraries_[lib].debug_suffix
                                                  : "") +
                      ".dll>";
        open << "\"\n";
      }
      if (!libs.empty()) open << ")\n\n";
    }
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
      for (size_t i = 0; i < dir_names.size(); ++i)
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
