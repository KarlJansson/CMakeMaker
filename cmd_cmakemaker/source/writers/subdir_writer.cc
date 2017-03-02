#include "precomp.h"

#include "common_writer.h"
#include "subdir_writer.h"

void SubdirWriter::WriteSubdir(
    std::string dir_name, RepoSearcher::directory& dir,
    std::map<std::string, RepoSearcher::directory>& targets,
    std::map<std::string, RepoSearcher::library>& libraries) {
  std::vector<std::string> precomp_file_bundles, proj_names;
  std::string proj_name =
      dir_name.substr(dir_name.find_last_of('/') + 1, dir_name.size());
  proj_name = dir_name.substr(dir_name.find_first_of('_') + 1, dir_name.size());
  proj_names.push_back(proj_name);

  std::string expected = CommonWriter::cmake_header_ + "\n";
  bool cuda_compile = dir.files["cu"].fmap.empty() ? false : true;

  if (cuda_compile) expected += "find_package(CUDA)\n";
  for (auto& lib : dir.libraries) {
    auto& lib_info = libraries[lib];
    if (!lib_info.find_command.empty()) {
      expected += "find_package(" + libraries[lib].find_command;
      if (!lib_info.components.empty()) {
        expected += " COMPONENTS\n";
        for (auto& mod : lib_info.components) expected += "  " + mod + "\n";
      }
      expected += ")\n";
    }
  }

  expected += "\n\n";

  if (cuda_compile) {
    expected +=
        "set(CUDA_ATTACH_VS_BUILD_RULE_TO_CUDA_FILE OFF)\n"
        "set(CUDA_64_BIT_DEVICE_CODE ON)\n"
        "if(CUDA_FOUND)\n"
        "  add_definitions(-DCuda_Found)\n"
        "else(CUDA_FOUND)\n"
        "  message(\"Could not find the Cuda toolkit, the Cuda algorithms "
        "will not be built.\")\n"
        "endif(CUDA_FOUND)\n\n";
  }

  expected += "set(cpp_files\n";
  for (auto& ext : CommonWriter::extensions_)
    for (auto& file : dir.files[ext].fmap)
      for (auto& pair : file.second) expected += "  " + pair + "\n";
  expected += ")\n\n";

  CommonWriter::WriteSourceGroups(dir, expected);

  if (!dir.moc_files.empty()) {
    expected += "qt5_wrap_cpp(moc_files\n";
    for (auto& file : dir.moc_files) expected += "  " + file + "\n";
    expected += ")\n\n";

    if (!dir.precomp_h.empty() && !dir.precomp_cc.empty())
      precomp_file_bundles.push_back("moc_files");
  }

  if (!dir.precomp_h.empty() && !dir.precomp_cc.empty())
    precomp_file_bundles.push_back("cpp_files");

  for (auto& s : precomp_file_bundles)
    expected += "add_msvc_precompiled_header(\"" +
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
      expected +=
          (cuda_compile ? "cuda_add_executable(" : "add_executable(") + p_name;
      if (dir_name.find("app_") != std::string::npos) expected += " WIN32";
    } else if (dir_name.find("lib_") != std::string::npos ||
               dir_name.find("slib_") != std::string::npos ||
               dir_name.find("dlib_") != std::string::npos) {
      expected +=
          (cuda_compile ? "cuda_add_library(" : "add_library(") + p_name;
      if (dir_name.find("slib_") != std::string::npos)
        expected += " STATIC";
      else if (dir_name.find("dlib_") != std::string::npos)
        expected += " SHARED";
      else if (proj_count++ == 0)
        expected += " SHARED";
      else
        expected += " STATIC";
    }
    expected += " ${cpp_files}";
    if (!dir.moc_files.empty()) expected += " ${moc_files}";
    if (dir.files.find("qrc") != dir.files.end())
      expected += " ${qt_resources}";
    expected += ")\n";
  }
  expected += "\n";

  for (auto& p_name : proj_names) {
    std::set<std::string> include_dirs;
    if (std::experimental::filesystem::exists("./source_shared"))
      include_dirs.insert("../source_shared");

    for (auto& d : dir.directories)
      if (!d.empty()) include_dirs.insert(d);
    for (auto& d : dir.include_dirs)
      if (!d.empty()) include_dirs.insert(d);
    for (auto& lib : dir.libraries)
      for (auto& inc : libraries[lib].includes) include_dirs.insert(inc);

    expected += "include_directories(" + p_name + "\n";
    for (auto& inc_dir : include_dirs) expected += "  " + inc_dir + "\n";
    if (cuda_compile) expected += "  ${CUDA_INCLUDE_DIRS}\n";
    expected += ")\n\n";

    expected += "target_link_libraries(" + p_name + "\n";
    for (auto& dep : dir.dependencies)
      expected +=
          "  " + dep.substr(dep.find_first_of('_') + 1, dep.size()) + "\n";
    for (auto& lib : dir.libraries)
      for (auto& l : libraries[lib].libs) expected += "  " + l + "\n";
    if (cuda_compile) expected += "  ${CUDA_LIBRARIES}\n";
    expected += ")\n\n";

    if (p_name.find("static_") == std::string::npos && !dir.dependent.empty()) {
      expected += "add_custom_command(TARGET " + p_name + " POST_BUILD\n";
      for (auto& dep : dir.dependent) {
        expected +=
            "  COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different\n"
            "    \"";
        for (auto& conf : CommonWriter::configs_)
          expected += "$<$<CONFIG:" + conf +
                      ">:${CMAKE_CURRENT_BINARY_DIR}/Build_Output/libs/" +
                      conf + "/" + p_name + ".dll>";
        expected += "\"\n    \"";
        for (auto& conf : CommonWriter::configs_)
          expected += "$<$<CONFIG:" + conf +
                      ">:${CMAKE_CURRENT_BINARY_DIR}/../" + dep +
                      "/Build_Output/bin/" + conf + "/" + p_name + ".dll>";
        expected += "\"\n";
      }
      expected += ")\n\n";
    }

    if (dir_name.find("runnable_") != std::string::npos ||
        dir_name.find("cmd_") != std::string::npos ||
        dir_name.find("app_") != std::string::npos) {
      std::set<std::string> libs;
      for (auto& dep : dir.dependencies)
        for (auto& lib : targets["./" + dep].libraries)
          if (!libraries[lib].dlls.empty()) libs.insert(lib);

      for (auto& lib : dir.libraries)
        if (!libraries[lib].dlls.empty()) libs.insert(lib);

      if (!libs.empty())
        expected += "add_custom_command(TARGET " + p_name + " POST_BUILD\n";
      for (auto& lib : libs) {
        for (int i = 0; i < libraries[lib].dlls.size(); ++i) {
          auto& dll = libraries[lib].dlls[i];
          auto& debug_dll = libraries[lib].debug_dlls.size() > i
                                ? libraries[lib].debug_dlls[i]
                                : dll;

          expected +=
              "  COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different\n"
              "    \"";

          for (auto& conf : CommonWriter::configs_)
            expected += "$<$<CONFIG:" + conf + ">:" +
                        (conf.compare("Debug") == 0
                             ? debug_dll + libraries[lib].debug_suffix
                             : dll) +
                        ".dll>";
          expected += "\"\n    \"";
          for (auto& conf : CommonWriter::configs_)
            expected +=
                "$<$<CONFIG:" + conf +
                ">:${CMAKE_CURRENT_BINARY_DIR}/Build_Output/bin/" + conf + "/" +
                (conf.compare("Debug") == 0
                     ? debug_dll.substr(debug_dll.find_last_of('/') + 1,
                                        debug_dll.size()) +
                           libraries[lib].debug_suffix
                     : dll.substr(dll.find_last_of('/') + 1, dll.size())) +
                ".dll>";
          expected += "\"\n";
        }
      }
      if (!libs.empty()) expected += ")\n\n";
    }
  }

  expected += "add_dependencies(" + proj_name + " ALL_PRE_BUILD)";

  CommonWriter::UpdateIfDifferent(dir_name + "/CMakeLists.txt", expected);
}
