// Author:  Bruce Allen
// Created: 2/25/2013
//
// The software provided here is released by the Naval Postgraduate
// School, an agency of the U.S. Department of Navy.  The software
// bears no warranty, either expressed or implied. NPS does not assume
// legal liability nor responsibility for a User's use of the software
// or the results of such use.
//
// Please note that within the United States, copyright protection,
// under Section 105 of the United States Code, Title 17, is not
// available for any work of the United States Government and/or for
// any works created by United States Government employees. User
// acknowledges that this software contains work which was created by
// NPS government employees and is therefore in the public domain and
// not subject to copyright.
//
// Released into the public domain on February 25, 2013 by Bruce Allen.

/**
 * \file
 * Test the LMDB data managers
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include "unit_test.h"
#include "lmdb_hash_data_manager.hpp"
#include "lmdb_hash_manager.hpp"
#include "lmdb_source_data_manager.hpp"
#include "lmdb_source_id_manager.hpp"
#include "lmdb_source_name_manager.hpp"
#include "lmdb_helper.h"
#include "lmdb_changes.hpp"
#include "source_id_offsets.hpp"
#include "../src_libhashdb/hashdb.hpp"
#include "directory_helper.hpp"

typedef std::pair<std::string, std::string> source_name_t;
typedef std::set<source_name_t>             source_names_t;

static const std::string hashdb_dir = "temp_dir_lmdb_managers_test.hdb";
static const std::string binary_0(hashdb::hex_to_bin(
                                  "00000000000000000000000000000000"));
static const std::string binary_1(hashdb::hex_to_bin(
                                  "00000000000000000000000000000001"));
static const std::string binary_2(hashdb::hex_to_bin(
                                  "00000000000000000000000000000002"));

void make_new_hashdb_dir(std::string p_hashdb_dir) {
  // remove any previous hashdb_dir
  rm_hashdb_dir(p_hashdb_dir);

  // create the hashdb directory
  require_no_dir(p_hashdb_dir);
  create_new_dir(p_hashdb_dir);
}

void check_changes(const hahsdb::lmdb_changes_t& changes,
                   const size_t hash_data_source_inserted,
                   const size_t hash_data_offset_inserted,
                   const size_t hash_data_data_changed,
                   const size_t hash_data_duplicate_offset_detected,
                   const size_t hash_data_mismatched_sub_count_detected) {

  // enable for diagnostics
  std::cerr << hash_data_source_inserted
            << "," << hash_data_offset_inserted
            << "," << hash_data_data_changed
            << "," << hash_data_duplicate_offset_detected
            << "," << hash_data_mismatched_sub_count_detected;

  // test each
  TEST_EQ(changes.hash_data_source_inserted, hash_data_source_inserted);
  TEST_EQ(changes.hash_data_offset_inserted, hash_data_offset_inserted);
  TEST_EQ(changes.hash_data_data_changed, hash_data_data_changed);
  TEST_EQ(changes.hash_data_duplicate_offset_detected,
          hash_data_duplicate_offset_detected);
  TEST_EQ(changes.hash_data_mismatched_sub_count_detected,
          hash_data_mismatched_sub_count_detected);
}

// ************************************************************
// test_empty
// ************************************************************
void test_empty() {

  // variables
  float entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 0, 0);

  // attempt to insert an empty key.  A warning is sent to stderr.
  TEST_EQ(manager.insert("", 1.0, "bl", 1, 0, changes), 0);

  // attempt to insert with an invalid file_offset.  warning sent to stderr.
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 513, changes), 0);

  // check that binary_0 is not there
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), false);
  TEST_FLOAT_EQ(entropy, 0.0);
  TEST_EQ(block_label, "");
  TEST_EQ(count, 0);
  TEST_EQ(source_id_offsets.size(), 0);

  // iterator
  TEST_EQ(manager.first_hash(), "");
}

// ************************************************************
// test_max
// ************************************************************
void test_max_3_2() {

  // variables
  float entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 3, 2);

  // fill up lists
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*0, changes), 1);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*1, changes), 2);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*2, changes), 3);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*3, changes), 4);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 2, 512*4, changes), 5);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 2, 512*5, changes), 6);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 2, 512*6, changes), 7);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 2, 512*7, changes), 8);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 2, 512*8, changes), 9);

  // validate changes
  check_changes(2,3,0,0,0);

  // check counts
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  source_id_offsets_t::const_iterator it = source_id_offsets.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 4);
  TEST_EQ(it->file_offsets.size(), 2);
  std::set<uint64_t>::it2 = it->file_offsets.begin();
  TEST_EQ(*it2, 512*0);
  ++it2;
  TEST_EQ(*it2, 512*1);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 5);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*4);
}

void test_max_2_3() {

  // variables
  float entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 2, 3);

  // fill up lists
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*0, changes), 1);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*1, changes), 2);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*2, changes), 3);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*3, changes), 4);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 2, 512*4, changes), 5);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 2, 512*5, changes), 6);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 2, 512*6, changes), 7);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 2, 512*7, changes), 8);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 2, 512*8, changes), 9);

  // validate changes
  check_changes(2,2,0,0,0);

  // check counts
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  source_id_offsets_t::const_iterator it = source_id_offsets.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 4);
  TEST_EQ(it->file_offsets.size(), 2);
  std::set<uint64_t>::it2 = it->file_offsets.begin();
  TEST_EQ(*it2, 512*0);
  ++it2;
  TEST_EQ(*it2, 512*1);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 5);
  TEST_EQ(it->file_offsets.size(), 0);
}

void test_insert_type1() {

  // variables
  float entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 2, 2);

  // insert something
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*1, changes), 1);

//  changes.hash_data_source_inserted
//  changes.hash_data_offset_inserted
//  changes.hash_data_data_changed
//  changes.hash_data_duplicate_offsets_detected
//  changes.hash_data_mismatched_sub_count_detected

  // validate changes
  check_changes(1,1,0,0,0);

  // insert same
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*1, changes), 1);
  check_changes(1,1,0,1,0);

  // insert same, different data
  TEST_EQ(manager.insert(binary_0, 2.0, "bl", 1, 512*1, changes), 1);
  check_changes(1,1,1,2,0);

  // insert second, same data
  TEST_EQ(manager.insert(binary_1, 2.0, "bl", 1, 512*2, changes), 1);
  check_changes(1,2,1,2,0);

  // insert third, different data
  TEST_EQ(manager.insert(binary_1, 1.0, "bl", 1, 512*3, changes), 1);
  check_changes(1,3,2,2,0);

  // merge, same data
  manager.find(binary_0, entropy, block_label, count, source_id_offsets);
  TEST_EQ(manager.merge(binary_0, 1.0, "bl", 1, 3,
                        *(source_id_offsets.begin()), changes), 3);
  check_changes(1,3,2,2,0);

  // merge, different data
  TEST_EQ(manager.merge(binary_0, 2.0, "bl", 1, 3,
                        *(source_id_offsets.begin()), changes), 3);
  check_changes(1,3,3,2,0);

  // merge, different data but mismatched
  source_id_offsets.begin()->sub_count++;
  TEST_EQ(manager.merge(binary_0, 3.0, "bl", 1, 3,
                        *(source_id_offsets.begin()), changes), 3);
  check_changes(1,3,3,2,1);
}

//  changes.hash_data_source_inserted
//  changes.hash_data_offset_inserted
//  changes.hash_data_data_changed
//  changes.hash_data_duplicate_offsets_detected
//  changes.hash_data_mismatched_sub_count_detected

// test Type 2 to Type 3 with insert
void test_type2_to_type3_insert() {

  // variables
  float entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 10, 10);

  // start with type1
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*1, changes), 1);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*1, changes), 1);

  // transition type1 to type2 and 2 type3
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 2, 512*1, changes), 2);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 2, 512*1, changes), 2);

  // add third type3
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 3, 512*1, changes), 3);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 3, 512*1, changes), 3);

  // validate changes
  check_changes(3,3,0,3,0);

  // validate storage
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                       source_id_offsets), true);
  TEST_EQ(count, 3);
  TEST_EQ(source_id_offsets.size(), 3);
}

// test Type 2 to Type 3 with insert and changes
void test_type2_to_type3_insert_and_change() {

  // variables
  float entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 10, 10);

  // start with type1
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*1, changes), 1);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 512*1, changes), 1);

  // transition type1 to type2 and 2 type3 with change
  TEST_EQ(manager.insert(binary_0, 2.0, "bl", 2, 512*1, changes), 2);
  TEST_EQ(manager.insert(binary_0, 2.0, "bl", 2, 512*1, changes), 2);

  // add third type3 with change
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 3, 512*1, changes), 3);
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 3, 512*1, changes), 3);

  // validate changes
  check_changes(3,3,2,3,0);

  // validate storage
  manager.find(binary_0, entropy, block_label, count, source_id_offsets);
  TEST_EQ(count, 3);
  TEST_EQ(source_id_offsets.size(), 3);
}





// test Type 2 and Type 3
void lmdb_hash_data_manager_insert_type2_and_type3() {

  // variables
  float entropy;
  std::string block_label;
  uint64_t count;
  std::set<uint64_t> file_offsets;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::lmdb_changes_t changes;
  hashdb::source_id_offsets_t::const_iterator it;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 2, 1);

  // set up file_offsets
  file_offsets.insert(512*1);
  file_offsets.insert(512*2);

  // set up Type 1
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 1, 2, file_offsets, changes), 2);

  // set up file_offsets
  file_offsets.clear();
  file_offsets.insert(512*3);
  file_offsets.insert(512*4);

  // test insert_new_type2() and max_sub_count
  // Insert to new source_id=2, use sub_count 4, and change data
  // 512*4 will not fit because max_sub_count=3
  TEST_EQ(manager.insert(binary_0, 1.5, "bl2", 2, 4, file_offsets, changes), 6);
  TEST_EQ(changes.hash_data_source_inserted, 2);
  TEST_EQ(changes.hash_data_offset_inserted, 2);
  TEST_EQ(changes.hash_data_data_changed, 1);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 1.5);
  TEST_EQ(block_label, "bl2");
  TEST_EQ(count, 6);
  TEST_EQ(source_id_offsets.size(), 2);
  it = source_id_offsets.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 2);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*1);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 4);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*3);

  // set up file_offsets
  file_offsets.clear();
  file_offsets.insert(512*5);

  // test insert_new_type3() and max_count
  // Insert to new source_id=3, use sub_count 1, and change data
  // 512*5 will not fit because max_sub_count=2
  TEST_EQ(manager.insert(binary_0, 1.0, "bl", 3, 1, file_offsets, changes), 7);
  TEST_EQ(changes.hash_data_source_inserted, 3);
  TEST_EQ(changes.hash_data_offset_inserted, 2);
  TEST_EQ(changes.hash_data_data_changed, 2);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 1.0);
  TEST_EQ(block_label, "bl");
  TEST_EQ(count, 7);
  TEST_EQ(source_id_offsets.size(), 3);

  it = source_id_offsets.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 2);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*1);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 4);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*3);
  ++it;
  TEST_EQ(it->source_id, 3);
  TEST_EQ(it->sub_count, 1);
  TEST_EQ(it->file_offsets.size(), 0);

  // test insert_update_type3() and max_count
  // Insert to existing source_id=2, use sub_count 1, and change data
  // 512*5 will not fit because max_sub_count=2
  TEST_EQ(manager.insert(binary_0, 1.5, "bl2", 2, 1, file_offsets, changes), 8);
  TEST_EQ(changes.hash_data_source_inserted, 3);
  TEST_EQ(changes.hash_data_offset_inserted, 2);
  TEST_EQ(changes.hash_data_data_changed, 3);

  // find binary_0
  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_FLOAT_EQ(entropy, 1.5);
  TEST_EQ(block_label, "bl2");
  TEST_EQ(count, 8);
  TEST_EQ(source_id_offsets.size(), 3);

  it = source_id_offsets.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 2);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*1);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 5);
  TEST_EQ(it->file_offsets.size(), 1);
  TEST_EQ(*(it->file_offsets.begin()), 512*3);
  ++it;
  TEST_EQ(it->source_id, 3);
  TEST_EQ(it->sub_count, 1);
  TEST_EQ(it->file_offsets.size(), 0);
}

void lmdb_hash_data_manager_insert_count_and_iterator() {

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 2, 1);

  // set up file_offsets
  std::set<uint64_t> file_offsets;
  file_offsets.insert(512*1);
  file_offsets.insert(512*2);

  // count in Type 1
  hashdb::lmdb_changes_t changes;
  manager.insert(binary_1, 0.0, "", 1, 10, file_offsets, changes);

  // count in Type 2
  manager.insert(binary_2, 0.0, "", 2, 5, file_offsets, changes);
  manager.insert(binary_2, 0.0, "", 3, 15, file_offsets, changes);

  // find_count
  TEST_EQ(manager.find_count(binary_0), 0);
  TEST_EQ(manager.find_count(binary_1), 10);
  TEST_EQ(manager.find_count(binary_2), 20);

  // iterator
  std::string block_hash = manager.first_hash();
  TEST_EQ(block_hash, binary_1);
  block_hash = manager.next_hash(block_hash);
  TEST_EQ(block_hash, binary_2);
  block_hash = manager.next_hash(block_hash);
  TEST_EQ(block_hash, "");
}

void lmdb_hash_data_manager_insert_maximums() {

  hashdb::lmdb_changes_t changes;
  std::set<uint64_t> file_offsets;
  float entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_id_offsets_t source_id_offsets;
  hashdb::source_id_offsets_t::const_iterator it;

  // create new manager
  make_new_hashdb_dir(hashdb_dir);
  hashdb::lmdb_hash_data_manager_t manager(
                            hashdb_dir, hashdb::RW_NEW, 512, 60, 55);

  // max offsets
  for (int i=0; i<100; ++i) {
    file_offsets.clear();
    file_offsets.insert(512*i);
    manager.insert(binary_0, 0.0, "", 1, 1, file_offsets, changes);
    manager.insert(binary_0, 0.0, "", 2, 1, file_offsets, changes);
  }

  TEST_EQ(manager.find(binary_0, entropy, block_label, count,
                                             source_id_offsets), true);
  TEST_EQ(count, 200);
  TEST_EQ(source_id_offsets.size(), 2);
  it = source_id_offsets.begin();
  TEST_EQ(it->source_id, 1);
  TEST_EQ(it->sub_count, 100);
  TEST_EQ(it->file_offsets.size(), 50);
  ++it;
  TEST_EQ(it->source_id, 2);
  TEST_EQ(it->sub_count, 100);
  TEST_EQ(it->file_offsets.size(), 50);

  // max block_label length, Type 1
  file_offsets.clear();
  file_offsets.insert(512*0);
  manager.insert(binary_1, 0.0, "0123456789a", 1, 1, file_offsets, changes);
  manager.find(binary_1, entropy, block_label, count, source_id_offsets);
  TEST_EQ(source_id_offsets.size(), 1);
  TEST_EQ(block_label, "0123456789");

  // max block_label length, Type 2
  manager.insert(binary_1, 0.0, "0123456789a", 2, 1, file_offsets, changes);
  manager.find(binary_1, entropy, block_label, count, source_id_offsets);
  TEST_EQ(source_id_offsets.size(), 2);
  TEST_EQ(block_label, "0123456789");
}


// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

  test_empty();
  test_max_3_2();
  test_max_2_3();
  test_type1();
  test_type2_to_type3_insert();
  test_type2_to_type3_insert_and_change();




  // lmdb_hash_data_manager insert
  lmdb_hash_data_manager_insert_empty();
  lmdb_hash_data_manager_insert_type1();
  lmdb_hash_data_manager_insert_type1_0_1();
  lmdb_hash_data_manager_insert_type1_1_0();
  lmdb_hash_data_manager_insert_type2_and_type3();
  lmdb_hash_data_manager_insert_count_and_iterator();
  lmdb_hash_data_manager_insert_maximums();

  // done
  std::cout << "lmdb_hash_data_manager_test Done.\n";
  return 0;
}

