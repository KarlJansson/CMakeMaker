#define DLLExport

#include <gtest/gtest.h>

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
#include <complex>
#include <numeric>

#include "test_target_writer.h"
#include "test_test_target_writer.h"

int main(int argc, char** args) {
  ::testing::InitGoogleTest(&argc, args);
  return RUN_ALL_TESTS();
}