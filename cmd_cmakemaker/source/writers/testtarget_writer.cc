#include "precomp.h"

#include "common_writer.h"
#include "testtarget_writer.h"

void TesttargetWriter::WriteTestTarget(
    std::string dir_name,
    std::map<std::string, RepoSearcher::directory>& targets,
    std::map<std::string, RepoSearcher::library>& libraries) {
  std::string proj_name =
      dir_name.substr(dir_name.find_last_of('/') + 1, dir_name.size());
  proj_name = dir_name.substr(dir_name.find_first_of('_') + 1, dir_name.size());
  std::set<std::string> include_dirs;
  std::vector<std::string> ext = {"cc", "cpp"};

  std::set<std::string> all_includes;
  for (auto& target : targets)
    for (auto& file : target.second.include_files) all_includes.insert(file);

  std::string precomp_includes = "";

  if (std::experimental::filesystem::exists("./source_shared")) {
    include_dirs.insert("../source_shared");
    for (auto& p :
         std::experimental::filesystem::directory_iterator("./source_shared")) {
      if (std::experimental::filesystem::is_regular_file(p)) {
        std::stringstream path_stream;
        path_stream << p;
        std::string path = path_stream.str();
        std::replace(path.begin(), path.end(), '\\', '/');

        precomp_includes +=
            "#include \"" + path.substr(path.find_last_of('/') + 1) + "\"\n";
      }
    }
  }

  precomp_includes += "\n";

  for (auto& include : all_includes)
    precomp_includes += "#include " + include + "\n";

  precomp_includes += "\n";

  std::string expected_precomp_h =
      "#include <gtest/gt"
      "est.h>\n\n" +
      precomp_includes;
  CommonWriter::UpdateIfDifferent(dir_name + "/precomp.h", expected_precomp_h);

  std::string expected_precomp_cc = "#include \"precomp.h\"\n\n";
  CommonWriter::UpdateIfDifferent(dir_name + "/precomp.cc",
                                  expected_precomp_cc);

  std::string expected_test_main = "#include \"precomp.h\"\n\n";
  for (auto& dir : targets)
    for (auto& obj_dir : dir.second.files["h"].fmap)
      for (auto& obj : obj_dir.second)
        if (obj.find("test_") != std::string::npos)
          expected_test_main +=
              "#include \"" +
              obj.substr(obj.find_last_of('/') + 1, obj.size()) + "\"\n";

  expected_test_main +=
      "\nint main(int argc, char** args) {\n"
      "  ::testing::InitGoogleTest(&argc, args);\n"
      "  return RUN_ALL_TESTS();\n"
      "}";

  CommonWriter::UpdateIfDifferent(dir_name + "/test_main.cc",
                                  expected_test_main);

  std::string expected = CommonWriter::cmake_header_ + "\n";
  std::map<std::string, std::set<std::string>> all_finds;
  all_finds["GTest"] = {};

  for (auto& dir : targets) {
    for (auto& lib : dir.second.libraries) {
      auto& lib_info = libraries[lib];

      if (!lib_info.find_command.empty()) {
        auto& ref = all_finds[lib_info.find_command];
        for (auto& comp : lib_info.components) ref.insert(comp);
      }
    }
  }

  for (auto& find : all_finds) {
    expected += "find_package(" + find.first;
    if (!find.second.empty()) {
      expected += " COMPONENTS\n";
      for (auto& mod : find.second) expected += "  " + mod + "\n";
    }
    expected += ")\n";
  }

  expected +=
      "\nset(cpp_files\n"
      "  test_main.cc\n"
      "  precomp.h\n"
      "  precomp.cc\n"
      ")\n\n"

      "add_msvc_precompiled_header(\"precomp.h\" \"precomp.cc\" cpp_files)\n"
      "add_executable(" +
      proj_name + " ${cpp_files})\n\n";

  for (auto& dir : targets) {
    for (auto& d : dir.second.directories)
      if (!d.empty())
        include_dirs.insert("." + dir.first + d.substr(1, d.size()));
    for (auto& d : dir.second.include_dirs)
      if (!d.empty()) include_dirs.insert(d);
    for (auto& lib : dir.second.libraries)
      if (!libraries[lib].include_dir.empty())
        include_dirs.insert(libraries[lib].include_dir);
  }
  include_dirs.insert("${GTEST_INCLUDE_DIRS}");

  expected += "include_directories(" + proj_name + "\n";
  for (auto& include : include_dirs) expected += "  " + include + "\n";
  expected += ")\n\n";

  std::set<std::string> link_libraries;
  for (auto& dir : targets) {
    std::string dir_n =
        dir.first.substr(dir.first.find_last_of('/') + 1, dir.first.size());

    for (auto& lib : dir.second.libraries)
      link_libraries.insert(libraries[lib].lib_dir);
    for (auto& ext : ext) {
      for (auto& obj_dir : dir.second.files[ext].fmap) {
        for (auto& obj : obj_dir.second) {
          if (obj.find("main.") != std::string::npos) continue;
          std::string obj_file_str =
              "${CMAKE_CURRENT_BINARY_DIR}/../" + dir_n + "/" +
              dir_n.substr(dir_n.find_first_of('_') + 1, dir_n.size()) +
              ".dir/";
          for (auto& conf : CommonWriter::configs_)
            obj_file_str += "$<$<CONFIG:" + conf + ">:" + conf + ">";
          obj_file_str +=
              "/" +
              obj.substr(obj.find_last_of('/') + 1,
                         obj.find_last_of('.') - obj.find_last_of('/')) +
              "obj";

          link_libraries.insert(obj_file_str);
        }
      }
    }

    for (auto& obj : dir.second.moc_files) {
      if (obj.find("main.") != std::string::npos) continue;
      std::string obj_file_str =
          "${CMAKE_CURRENT_BINARY_DIR}/../" + dir_n + "/" +
          dir_n.substr(dir_n.find_first_of('_') + 1, dir_n.size()) + ".dir/";
      for (auto& conf : CommonWriter::configs_)
        obj_file_str += "$<$<CONFIG:" + conf + ">:" + conf + ">";
      obj_file_str +=
          "/moc_" +
          obj.substr(obj.find_last_of('/') + 1,
                     obj.find_last_of('.') - obj.find_last_of('/')) +
          "obj";

      link_libraries.insert(obj_file_str);
    }
  }
  link_libraries.insert("${GTEST_LIBRARIES}");

  expected += "target_link_libraries(" + proj_name + "\n";
  for (auto& lib : link_libraries) expected += "  " + lib + "\n";
  expected += ")\n\n";

  for (auto& dir : targets) {
    std::set<std::string> libs;
    for (auto& dep : dir.second.dependencies)
      for (auto& lib : targets["./" + dep].libraries)
        if (!libraries[lib].dll_file.empty()) libs.insert(lib);

    for (auto& lib : dir.second.libraries)
      if (!libraries[lib].dll_file.empty()) libs.insert(lib);

    if (!libs.empty())
      expected += "add_custom_command(TARGET " + proj_name + " POST_BUILD\n";
    for (auto& lib : libs) {
      expected +=
          "  COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different\n    \"";
      for (auto& conf : CommonWriter::configs_)
        expected +=
            "$<$<CONFIG:" + conf + ">:" + libraries[lib].dll_file +
            (conf.compare("Debug") == 0 ? libraries[lib].debug_suffix : "") +
            ".dll>";
      expected += "\"\n    \"";
      for (auto& conf : CommonWriter::configs_)
        expected +=
            "$<$<CONFIG:" + conf +
            ">:${CMAKE_CURRENT_BINARY_DIR}/Build_Output/bin/" + conf + "/" +
            libraries[lib].dll_file.substr(
                libraries[lib].dll_file.find_last_of('/') + 1,
                libraries[lib].dll_file.size()) +
            (conf.compare("Debug") == 0 ? libraries[lib].debug_suffix : "") +
            ".dll>";
      expected += "\"\n";
    }
    if (!libs.empty()) expected += ")\n\n";
  }

  expected += "add_dependencies(" + proj_name;
  for (auto& dir : targets) {
    std::string dir_n =
        dir.first.substr(dir.first.find_last_of('/') + 1, dir.first.size());
    dir_n = dir_n.substr(dir_n.find_first_of('_') + 1, dir_n.size());
    expected += " " + dir_n;
  }
  expected += " ALL_PRE_BUILD)";

  CommonWriter::UpdateIfDifferent(dir_name + "/CMakeLists.txt", expected);
}

bool TesttargetWriter::MainUpdateNeeded(
    std::string dir_name,
    std::map<std::string, RepoSearcher::directory>& targets) {
  if (!std::experimental::filesystem::exists(dir_name + "/test_main.cc"))
    return true;

  std::ifstream test_main(dir_name + "/test_main.cc");

  std::set<std::string> test_files, check_set;
  for (auto& dir : targets)
    for (auto& obj_dir : dir.second.files["h"].fmap)
      for (auto& obj : obj_dir.second)
        if (obj.find("test_") != std::string::npos)
          test_files.insert(obj.substr(obj.find_last_of('/') + 1, obj.size()));

  std::string line;
  while (!test_main.eof()) {
    std::getline(test_main, line);

    if (line.find("#include") != std::string::npos) {
      auto& it = test_files.find(
          line.substr(line.find_first_of('\"') + 1,
                      line.find_last_of('\"') - line.find_first_of('\"') - 1));
      if (it != test_files.end()) check_set.insert(*it);
    }
  }

  test_main.close();
  return check_set.size() != test_files.size();
}
