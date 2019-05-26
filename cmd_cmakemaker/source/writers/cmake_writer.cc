#include "precomp.h"

#include "benchmarktarget_writer.h"
#include "cmake_writer.h"
#include "settings_parser.h"
#include "subdir_writer.h"
#include "testtarget_writer.h"

CmakeWriter::CmakeWriter(RepoSearcher searcher) : searcher_(searcher) {
  SettingsParser parse;
  std::string cmake_config_path;
#ifdef WindowsBuild
  if (std::experimental::filesystem::exists("./.cmakemaker_win"))
    cmake_config_path = "./.cmakemaker_win";
#else
  if (std::experimental::filesystem::exists("./.cmakemaker_unix"))
    cmake_config_path = "./.cmakemaker_unix";
#endif
  if (cmake_config_path.empty())
    parse.ParseSettings("./.cmakemaker", libraries_, settings_);
  else
    parse.ParseSettings(cmake_config_path, libraries_, settings_);
}

void CmakeWriter::WriteCmakeFiles() {
  main_dir_ = searcher_.SearchPath("./");

  for (auto& dir : main_dir_.directories)
    if (dir.find("benchmark_") == std::string::npos &&
        dir.find("test_") == std::string::npos)
      targets_[dir] = searcher_.SearchPathSubdirs(dir);
  searcher_.FindDependencies(targets_, libraries_);
  WriteMain(main_dir_);

  // Write non test and benchmark targets
  auto subdir_writer = std::make_unique<SubdirWriter>();
  for (auto& dir : main_dir_.directories)
    if (dir.find("benchmark_") == std::string::npos &&
        dir.find("test_") == std::string::npos)
      subdir_writer->WriteSubdir(dir, targets_[dir], targets_, libraries_);

  // Write benchmark targets
  auto benchmarktarget_writer = std::make_unique<BenchmarktargetWriter>();
  for (auto& dir : main_dir_.directories)
    if (dir.find("benchmark_") != std::string::npos)
      benchmarktarget_writer->WriteBenchmarkTarget(dir, targets_, libraries_);

  // Write test targets
  auto testtarget_writer = std::make_unique<TesttargetWriter>();
  for (auto& dir : main_dir_.directories)
    if (dir.find("test_") != std::string::npos)
      testtarget_writer->WriteTestTarget(dir, targets_, libraries_);
}

void CmakeWriter::WriteMain(RepoSearcher::directory& dir) {
  std::string cpp_version = "c++14";
  auto it = settings_.find("cpp_version");
  if (it != settings_.end()) cpp_version = it->second;

  std::string expected =
      CommonWriter::cmake_header_ +
      "find_program(CCACHE_PROGRAM ccache)\n"
      "if(CCACHE_PROGRAM)\n"
      "  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "
      "\"${CCACHE_PROGRAM}\")\n"
      "endif()\n\n"

      "project(cmakemaker_solution CXX)\n\n"

      "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n"
      "macro(add_msvc_precompiled_header PrecompiledHeader \n"
      "PrecompiledSource SourcesVar)\n"
      "  if(MSVC)\n"
      "    get_filename_component(PrecompiledBasename ${PrecompiledHeader} "
      "NAME_WE)\n"
      "    set(PrecompiledBinary \"$(IntDir)/${PrecompiledBasename}.pch\")\n"
      "    set(Sources ${${SourcesVar}})\n"
      "    set_source_files_properties(${Sources}\n"
      "      PROPERTIES COMPILE_FLAGS \"/Yu\\\"${PrecompiledHeader}\\\" "
      "/FI\\\"${PrecompiledHeader}\\\" /Fp\\\"${PrecompiledBinary}\\\"\"\n"
      "      OBJECT_DEPENDS \"${PrecompiledBinary}\")\n"
      "    set_source_files_properties(${PrecompiledSource}\n"
      "      PROPERTIES COMPILE_FLAGS \"/Yc\\\"${PrecompiledHeader}\\\" "
      "/FI\\\"${PrecompiledHeader}\\\" /Fp\\\"${PrecompiledBinary}\\\"\"\n"
      "      OBJECT_OUTPUTS \"${PrecompiledBinary}\")\n"
      "    list(APPEND ${SourcesVar} ${PrecompiledSource})\n"
      "  endif(MSVC)\n"
      "endmacro(add_msvc_precompiled_header)\n\n"

      "set(LIBRARY_OUTPUT_PATH Build_Output/libs CACHE PATH \"Lib path\")\n"
      "set(EXECUTABLE_OUTPUT_PATH Build_Output/bin CACHE PATH \"Exe path\")\n"
      "mark_as_advanced(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)\n"

      "add_definitions(-DUNICODE -D_UNICODE)\n\n"

      "if (WIN32)\n"
      "  add_definitions(-DWindowsBuild)\n"
      "  set(CMAKE_CXX_FLAGS  \"${CMAKE_CXX_FLAGS} -MP\")\n"

      "else(WIN32)\n"
      "  add_definitions(-DUnixBuild)\n"
      "  set(CMAKE_CXX_FLAGS  \"${CMAKE_CXX_FLAGS} -std=" +
      cpp_version +
      "\")\n"
      "  set(CMAKE_CXX_FLAGS_DEBUG  \"${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG\")\n"
      "endif(WIN32)\n\n"

      "find_program(CMAKEMAKER_CMD cmakemaker)\n"
      "if(NOT CMAKEMAKER_CMD)\n"
      "  add_custom_target(\n"
      "    ALL_PRE_BUILD\n"
      "    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}\n"
      "  )\n"
      "else()\n"
      "  add_custom_target(\n"
      "    ALL_PRE_BUILD\n"
      "    COMMAND ${CMAKEMAKER_CMD}\n"
      "    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}\n"
      "  )\n"
      "endif()\n\n";

  std::vector<std::string> copy_types = {"qml"};
  for (auto target : targets_) {
    for (auto& type : copy_types) {
      if (!target.second.files[type].fmap.empty()) {
        expected += "add_custom_command(TARGET ALL_PRE_BUILD PRE_BUILD\n";
        for (auto& path : target.second.files[type].fmap) {
          CommonWriter::AddMakeDirCommand(
              path.first,
              "${CMAKE_CURRENT_BINARY_DIR}" +
                  target.first.substr(1, target.first.size()) +
                  "/Build_Output/bin/",
              true, expected);
          CommonWriter::AddMakeDirCommand(
              path.first,
              "${CMAKE_CURRENT_BINARY_DIR}" +
                  target.first.substr(1, target.first.size()),
              false, expected);
          for (auto& file : path.second) {
            CommonWriter::AddCopyCommand(
                file.substr(1, file.size()), "", "",
                "${CMAKE_CURRENT_SOURCE_DIR}" +
                    target.first.substr(1, target.first.size()),
                "${CMAKE_CURRENT_BINARY_DIR}" +
                    target.first.substr(1, target.first.size()) +
                    "/Build_Output/bin/",
                true, expected);
            CommonWriter::AddCopyCommand(
                file.substr(1, file.size()), "", "",
                "${CMAKE_CURRENT_SOURCE_DIR}" +
                    target.first.substr(1, target.first.size()),
                "${CMAKE_CURRENT_BINARY_DIR}" +
                    target.first.substr(1, target.first.size()),
                false, expected);
          }
        }
        expected += ")\n\n";
      }
    }
  }

  std::vector<std::string> test_targets, benchmark_targets, exe_targets,
      lib_tragets;
  for (auto& subdir : dir.directories) {
    if (subdir.find("test_") != std::string::npos)
      test_targets.push_back(subdir);
    else if (subdir.find("benchmark_") != std::string::npos)
      benchmark_targets.push_back(subdir);
    else if (subdir.find("lib_") != std::string::npos ||
             subdir.find("slib_") != std::string::npos ||
             subdir.find("dlib_") != std::string::npos)
      lib_tragets.push_back(subdir);
    else if (subdir.find("runnable_") != std::string::npos ||
             subdir.find("cmd_") != std::string::npos ||
             subdir.find("app_") != std::string::npos)
      exe_targets.push_back(subdir);
  }

  for (auto& subdir : exe_targets)
    expected += "add_subdirectory(" + subdir + ")\n";
  for (auto& subdir : lib_tragets)
    expected += "add_subdirectory(" + subdir + ")\n";
  for (auto& subdir : test_targets)
    expected += "add_subdirectory(" + subdir + ")\n";
  for (auto& subdir : benchmark_targets)
    expected += "add_subdirectory(" + subdir + ")\n";

  std::string default_target = "";
  if (!exe_targets.empty())
    default_target = exe_targets[0];
  else if (!test_targets.empty())
    default_target = test_targets[0];

  if (!default_target.empty()) {
    expected +=
        "if (MSVC)\n"
        "  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY "
        "VS_STARTUP_PROJECT " +
        default_target.substr(default_target.find_first_of('_') + 1,
                              default_target.size()) +
        ")\n"
        "endif(MSVC)\n";
  }

  expected +=
      "add_custom_command(TARGET ALL_PRE_BUILD PRE_BUILD\n"
      "  COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different "
      "\"${CMAKE_BINARY_DIR}/compile_commands.json\" "
      "\"${CMAKE_SOURCE_DIR}/compile_commands.json\""
      "\n)";

  CommonWriter::UpdateIfDifferent("./CMakeLists.txt", expected);
}
