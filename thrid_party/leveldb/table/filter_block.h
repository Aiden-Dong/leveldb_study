// Copyright (c) 2012 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A filter block is stored near the end of a Table file.  It contains
// filters (e.g., bloom filters) for all data blocks in the table combined
// into a single filter block.

#ifndef STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
#define STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "leveldb/slice.h"
#include "util/hash.h"

namespace leveldb {

class FilterPolicy;

/***
 * FilterBlock 也受到 datablock 更新而更新， 当 datablock 开启新的 block 时候，
 * 每一个 DataBlock 具有一个BloomFilter
 *
 * 数据结构 :
 *      [filter 0]                     : string
 *      [filter 1]                     : string
 *      [filter 2]                     : string
 *      ...
 *      [filter n-1]                   : string
 *
 *      [offset of filter 0]           : 4 bytes
 *      [offset of filter 1]           : 4 bytes
 *      .....
 *      [offset of filter n-1]         : 4 bytes
 *      [offset of filter all result]  : 4 bytes
 *      lg(base)                       : 1 bytes
 */
class FilterBlockBuilder {
 public:
  explicit FilterBlockBuilder(const FilterPolicy*);

  FilterBlockBuilder(const FilterBlockBuilder&) = delete;
  FilterBlockBuilder& operator=(const FilterBlockBuilder&) = delete;

  /***
   * 根据 datablock 偏移量构建新的 filter block
   */
  void StartBlock(uint64_t block_offset);

  /***
   * 在 key 添加到 datablock 后，也会添加到 filter block 中
   */
  void AddKey(const Slice& key);

  /***
   * 内部数据 :
   *    - 每个 filter 的偏移量
   *    - filter 总的字节大小
   *    - kFilterBaseLg 数据保存
   * @return 构造的 FilterBlock 块
   */
  Slice Finish();

 private:

  // 生成 bloomfilter 过程
  void GenerateFilter();

  const FilterPolicy* policy_;              // 过滤策略， 默认是 BloomFilter
  std::string keys_;                        // 将所有的key都平铺拼接在一起，使用start_ 记录每个key的起始为孩子
  std::vector<size_t> start_;               // Starting index in keys_ of each key  每个 key 的起始点

  std::vector<Slice> tmp_keys_;             // 用于计算 Bloom 过滤器的临时存储key
  std::string result_;                      // 存放得到的过滤器结果， 具有多个过滤器，拼接，使用filter_offsets_记录拼接位置
  std::vector<uint32_t> filter_offsets_;    // 每个过滤器的偏移量
};

/***
 * Bloom过滤器读取器
 */
class FilterBlockReader {
 public:
  /***
   * 过滤器读取器构造
   * @param policy 策略， Bloom过滤器
   * @param contents  filterblock 消息体
   */
  FilterBlockReader(const FilterPolicy* policy, const Slice& contents);

  bool KeyMayMatch(uint64_t block_offset, const Slice& key);

 private:
  const FilterPolicy* policy_;
  const char* data_;         // 数据体指针
  const char* offset_;       // Bloom 过滤器定位点 初始位置指针
  size_t num_;               // Bloom 过滤器数量
  size_t base_lg_;           // kFilterBase
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
