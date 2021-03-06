/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */
#pragma once

#include <memory>

#include "logdevice/common/Metadata.h"
#include "logdevice/common/test/TestUtil.h"
#include "logdevice/server/locallogstore/LocalLogStore.h"
#include "logdevice/server/locallogstore/LocalLogStoreFactory.h"

namespace facebook { namespace logdevice {

/**
 * @file Wrapper store that initializes an empty log store in a temporary
 *       location and deletes it on destruction.  Great for tests!
 */

class TemporaryLogStore : public LocalLogStore {
 public:
  typedef std::function<LocalLogStore*(std::string path)> factory_func_t;

  explicit TemporaryLogStore(factory_func_t factory);
  ~TemporaryLogStore() override;

  /**
   * Closes the wrapped RocksDB store and underlying RocksDB instance.  In
   * combination with getPath(), this allows tests to inspect the RocksDB
   * instance directly.  The database will still get deleted when this object
   * is destroyed.
   */
  void close();

  void open();

  const char* getPath() const {
    return temp_dir_->path().c_str();
  }

  //
  // LocalLogStore interface
  //
  void stallLowPriWrite() override;
  void adviseUnstallingLowPriWrites(bool never_stall = false) override;
  int writeMulti(const std::vector<const WriteOp*>& writes,
                 const WriteOptions& options = WriteOptions()) override;
  int sync(Durability) override;
  FlushToken maxFlushToken() const override;
  FlushToken flushedUpThrough() const override;
  FlushToken maxWALSyncToken() const override;
  FlushToken walSyncedUpThrough() const override;

  std::unique_ptr<ReadIterator>
  read(logid_t log_id, const LocalLogStore::ReadOptions&) const override;
  std::unique_ptr<AllLogsIterator>
  readAllLogs(const LocalLogStore::ReadOptions&) const override;

  int readLogMetadata(logid_t log_id, LogMetadata* metadata) override;
  int writeLogMetadata(logid_t log_id,
                       const LogMetadata& metadata,
                       const WriteOptions& options) override;
  int deleteStoreMetadata(const StoreMetadataType type,
                          const WriteOptions& opts = WriteOptions()) override;
  int deleteLogMetadata(logid_t first_log_id,
                        logid_t last_log_id,
                        const LogMetadataType type,
                        const WriteOptions& opts = WriteOptions()) override;
  int deletePerEpochLogMetadata(
      logid_t log_id,
      epoch_t epoch,
      const PerEpochLogMetadataType type,
      const WriteOptions& opts = WriteOptions()) override;
  int updateLogMetadata(logid_t log_id,
                        ComparableLogMetadata& metadata,
                        const WriteOptions& options) override;
  int readStoreMetadata(StoreMetadata* metadata) override;
  int writeStoreMetadata(const StoreMetadata& metadata,
                         const WriteOptions& options) override;

  int readPerEpochLogMetadata(logid_t log_id,
                              epoch_t epoch,
                              PerEpochLogMetadata* metadata,
                              bool find_last_available = false,
                              bool allow_blocking_io = true) const override;
  int updatePerEpochLogMetadata(logid_t log_id,
                                epoch_t epoch,
                                PerEpochLogMetadata& metadata,
                                LocalLogStore::SealPreemption seal_preempt,
                                const WriteOptions& write_options) override;

  Status acceptingWrites() const override;

  int isEmpty() const override;

  int getShardIdx() const override;

  int getHighestInsertedLSN(logid_t log_id, lsn_t* highestLSN) override;

  int getApproximateTimestamp(
      logid_t log_id,
      lsn_t lsn,
      bool allow_blocking_io,
      std::chrono::milliseconds* timestamp_out) override;

  int readAllLogSnapshotBlobs(LocalLogStore::LogSnapshotBlobType type,
                              LogSnapshotBlobCallback callback) override;

  int writeLogSnapshotBlobs(
      LocalLogStore::LogSnapshotBlobType snapshots_type,
      const std::vector<std::pair<logid_t, Slice>>& snapshots) override;

  int deleteAllLogSnapshotBlobs() override;

  int findTime(logid_t log_id,
               std::chrono::milliseconds timestamp,
               lsn_t* lo,
               lsn_t* hi,
               bool approximate = false,
               bool allow_blocking_io = true,
               std::chrono::steady_clock::time_point deadline =
                   std::chrono::steady_clock::time_point::max()) const override;

  int findKey(logid_t log_id,
              std::string key,
              lsn_t* lo,
              lsn_t* hi,
              bool approximate = false,
              bool allow_blocking_io = true) const override;

 private:
  factory_func_t factory_;
  std::unique_ptr<folly::test::TemporaryDirectory> temp_dir_;
  std::unique_ptr<LocalLogStore> db_;
};

struct TemporaryRocksDBStore : public TemporaryLogStore {
  explicit TemporaryRocksDBStore(bool read_find_time_index = false);
};

}} // namespace facebook::logdevice
