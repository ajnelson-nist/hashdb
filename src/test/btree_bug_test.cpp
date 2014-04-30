// Test to manifest the use_count bug, 1/15/2014

#include "file_modes.h"
#include "map_manager.hpp"
#include <iostream>
#include <cstdio>
#include "directory_helper.hpp"
#include <boost/detail/lightweight_main.hpp>

static const char temp_dir[] = "temp_dir";

typedef map_manager_t<uint64_t> map_t;

void test_rw() {

  size_t size __attribute__((unused));
  std::pair<map_manager_t<uint64_t>::map_iterator_t, bool> pair_it_bool;

  // clean up from any previous run
  rm_hashdb_dir(temp_dir);
  make_dir_if_not_there(temp_dir);

  // create new map
  map_t map(temp_dir, RW_NEW);

  // check count
  size = map.size();

  // change entry invalid
  pair_it_bool = map.change(6000006, 60);

  // check count stayed same
  size = map.size();
}

void test_ro() {
  // open and reclose Read Only
  map_t map(temp_dir, READ_ONLY);
}

int cpp_main(int argc, char* argv[]) {

  test_rw();
  test_ro();

  // done
  return 0;
}

