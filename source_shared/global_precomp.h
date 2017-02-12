#ifdef WindowsBuild
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WIN32_WINDOWS 0x0601
#define DLLExport __declspec(dllexport)
#define TestExport __declspec(dllexport)
#include <windows.h>
#endif

#ifdef UnixBuild
#define DLLExport __attribute__((visibility("default")))
#define TestExport __attribute__((visibility("default")))
#endif

#include <memory>
#include <string>
#include <vector>
#include <array>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <random>
#include <iterator>
#include <locale>
#include <atomic>
#include <mutex>
#include <future>
#include <condition_variable>
#include <numeric>
