#include "precomp.h"

#include "test_common_writer.h"
#include "test_subdir_writer.h"
#include "test_testtarget_writer.h"

int main(int argc, char** args) {
  ::testing::InitGoogleTest(&argc, args);
  return RUN_ALL_TESTS();
}