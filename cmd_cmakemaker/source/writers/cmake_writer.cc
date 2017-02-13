#include "precomp.h"

#include "cmake_writer.h"
#include "settings_parser.h"
#include "subdir_writer.h"
#include "test_target_writer.h"

CmakeWriter::CmakeWriter(RepoSearcher searcher) : searcher_(searcher) {
  SettingsParser parse;
  parse.ParseSettings("./cmakemaker", libraries_);
}

void CmakeWriter::WriteCmakeFiles() {
  main_dir_ = searcher_.SearchPath("./");
  WriteMain(main_dir_);
  for (auto& dir : main_dir_.directories)
    if (dir.find("test_") == std::string::npos)
      targets_[dir] = searcher_.SearchPathSubdirs(dir);
  searcher_.FindDependencies(targets_, libraries_);

  // Write non test targets
  auto subdir_writer = std::make_unique<SubdirWriter>();
  for (auto& dir : main_dir_.directories)
    if (dir.find("test_") == std::string::npos)
      subdir_writer->WriteSubdir(dir, targets_[dir], targets_, libraries_);

  // Write test targets
  auto test_target_writer = std::make_unique<TestTargetWriter>();
  for (auto& dir : main_dir_.directories)
    if (dir.find("test_") != std::string::npos)
      test_target_writer->WriteTestTarget(dir, targets_, libraries_);
}

void CmakeWriter::WriteMain(RepoSearcher::directory& dir) {
  std::string expected =
      CommonWriter::cmake_header_ +
      "project(cmakemaker_solution C CXX)\n\n"

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
      "  set(CMAKE_CXX_FLAGS_RELEASE  \"${CMAKE_CXX_FLAGS_RELEASE} -MP\")\n"

      "else(WIN32)\n"
      "  add_definitions(-DUnixBuild)\n"
      "  set(CMAKE_CXX_FLAGS  \"${CMAKE_CXX_FLAGS} -fPIC\")\n"
      "endif(WIN32)\n\n";

  expected +=
      "add_custom_target(\n"
      "  ALL_PRE_BUILD\n"
      "  COMMAND cmakemaker.exe\n"
      "  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}\n"
      ")\n\n";

  std::vector<std::string> test_targets, exe_targets, lib_tragets;
  for (auto& subdir : dir.directories) {
    if (subdir.find("test_") != std::string::npos)
      test_targets.push_back(subdir);
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

  CommonWriter::UpdateIfDifferent("./CMakeLists.txt", expected);
}
