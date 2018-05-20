#include "precomp.h"

#include "settings_parser.h"

void SettingsParser::ParseSettings(
    std::string path, std::map<std::string, RepoSearcher::library>& libraries,
    std::map<std::string, std::string>& other_settings) {
  std::ifstream open(path);
  if (!open.fail()) {
    RepoSearcher::library tmp;
    std::string line;
    while (!open.eof() && !open.fail()) {
      std::getline(open, line);

      if (line.find(':') != std::string::npos) {
        std::string left = line.substr(0, line.find_first_of(':'));
        std::string right =
            line.substr(line.find_first_of(':') + 1, line.size());

        std::string* str_ptr = nullptr;
        if (left.find("name") != std::string::npos)
          str_ptr = &tmp.name;
        else if (left.find("find_command") != std::string::npos)
          str_ptr = &tmp.find_command;
        else if (left.find("include_dir") != std::string::npos) {
          tmp.includes.push_back("");
          str_ptr = &tmp.includes.back();
        } else if (left.find("lib_dir") != std::string::npos) {
          tmp.libs.push_back("");
          str_ptr = &tmp.libs.back();
        } else if (left.find("dll_file") != std::string::npos ||
                   left.find("dll_release") != std::string::npos) {
          tmp.dlls.push_back("");
          str_ptr = &tmp.dlls.back();
        } else if (left.find("dll_debug") != std::string::npos) {
          tmp.debug_dlls.push_back("");
          str_ptr = &tmp.debug_dlls.back();
        } else if (left.find("debug_suffix") != std::string::npos)
          str_ptr = &tmp.debug_suffix;
        else
          other_settings[left] = right;

        if (str_ptr)
          *str_ptr = right.substr(
              right.find_first_of('\"') + 1,
              right.find_last_of('\"') - right.find_first_of('\"') - 1);

        if (left.find("components") != std::string::npos) {
          std::string comp = "";
          right = right.substr(right.find_first_of('\"') + 1, right.size());
          while (right.find('\"') != std::string::npos) {
            comp = right.substr(0, right.find_first_of('\"'));
            tmp.components.push_back(comp);
            right = right.substr(right.find_first_of('\"') + 1, right.size());
            right = right.substr(right.find_first_of('\"') + 1, right.size());
          }
        }
      }

      if (line.compare("{") == 0) tmp = RepoSearcher::library();
      if (line.compare("}") == 0) libraries[tmp.name] = tmp;
    }
  }
  open.close();
}
