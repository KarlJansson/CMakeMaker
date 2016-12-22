#include "settings_parser.h"
#include <fstream>

void SettingsParser::ParseSettings(
    std::string path, std::map<std::string, RepoSearcher::library>& libraries) {
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
        else if (left.find("include_dir") != std::string::npos)
          str_ptr = &tmp.include_dir;
        else if (left.find("lib_dir") != std::string::npos)
          str_ptr = &tmp.lib_dir;

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
