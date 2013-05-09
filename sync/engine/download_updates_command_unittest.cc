// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/engine/download_updates_command.h"
#include "sync/protocol/sync.pb.h"
#include "sync/sessions/nudge_tracker.h"
#include "sync/test/engine/fake_model_worker.h"
#include "sync/test/engine/syncer_command_test.h"

using ::testing::_;

namespace syncer {

// A test fixture for tests exercising DownloadUpdatesCommandTest.
class DownloadUpdatesCommandTest : public SyncerCommandTest {
 protected:
  DownloadUpdatesCommandTest()
      : command_(true /* create_mobile_bookmarks_folder */) {}

  virtual void SetUp() {
    workers()->clear();
    mutable_routing_info()->clear();
    workers()->push_back(
        make_scoped_refptr(new FakeModelWorker(GROUP_DB)));
    workers()->push_back(
        make_scoped_refptr(new FakeModelWorker(GROUP_UI)));
    (*mutable_routing_info())[AUTOFILL] = GROUP_DB;
    (*mutable_routing_info())[BOOKMARKS] = GROUP_UI;
    (*mutable_routing_info())[PREFERENCES] = GROUP_UI;
    SyncerCommandTest::SetUp();
  }

  DownloadUpdatesCommand command_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadUpdatesCommandTest);
};

TEST_F(DownloadUpdatesCommandTest, ExecuteNoStates) {
  ConfigureMockServerConnection();

  sessions::NudgeTracker nudge_tracker;
  nudge_tracker.RecordLocalChange(ModelTypeSet(BOOKMARKS));

  mock_server()->ExpectGetUpdatesRequestTypes(
      GetRoutingInfoTypes(routing_info()));
  command_.ExecuteImpl(
      sessions::SyncSession::BuildForNudge(context(),
                                           delegate(),
                                           nudge_tracker.GetSourceInfo(),
                                           &nudge_tracker));
}

TEST_F(DownloadUpdatesCommandTest, ExecuteWithStates) {
  ConfigureMockServerConnection();

  sessions::NudgeTracker nudge_tracker;
  nudge_tracker.RecordRemoteInvalidation(
      ModelTypeSetToInvalidationMap(ModelTypeSet(AUTOFILL),
                                    "autofill_payload"));
  nudge_tracker.RecordRemoteInvalidation(
      ModelTypeSetToInvalidationMap(ModelTypeSet(BOOKMARKS),
                                    "bookmark_payload"));
  nudge_tracker.RecordRemoteInvalidation(
      ModelTypeSetToInvalidationMap(ModelTypeSet(PREFERENCES),
                                    "preferences_payload"));

  mock_server()->ExpectGetUpdatesRequestTypes(
      GetRoutingInfoTypes(routing_info()));
  mock_server()->ExpectGetUpdatesRequestStates(
      nudge_tracker.GetSourceInfo().types);
  command_.ExecuteImpl(
      sessions::SyncSession::BuildForNudge(context(),
                                           delegate(),
                                           nudge_tracker.GetSourceInfo(),
                                           &nudge_tracker));
}

TEST_F(DownloadUpdatesCommandTest, VerifyAppendDebugInfo) {
  sync_pb::DebugInfo debug_info;
  EXPECT_CALL(*(mock_debug_info_getter()), GetAndClearDebugInfo(_))
      .Times(1);
  command_.AppendClientDebugInfoIfNeeded(session(), &debug_info);

  // Now try to add it once more and make sure |GetAndClearDebugInfo| is not
  // called.
  command_.AppendClientDebugInfoIfNeeded(session(), &debug_info);
}

}  // namespace syncer
