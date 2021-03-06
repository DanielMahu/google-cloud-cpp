// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GOOGLE_CLOUD_CPP_BIGTABLE_CLIENT_TESTING_TABLE_TEST_FIXTURE_H_
#define GOOGLE_CLOUD_CPP_BIGTABLE_CLIENT_TESTING_TABLE_TEST_FIXTURE_H_

#include "bigtable/client/testing/mock_client.h"
#include "bigtable/client/data.h"

namespace bigtable {
namespace testing {

/// Common fixture for the bigtable::Table tests.
class TableTestFixture : public ::testing::Test {
 protected:
  void SetUp() override;

  std::shared_ptr<MockClient> client_ = std::make_shared<MockClient>();
  std::shared_ptr<::google::bigtable::v2::MockBigtableStub> bigtable_stub_ =
      std::make_shared<::google::bigtable::v2::MockBigtableStub>();
  bigtable::Table table_ = bigtable::Table(client_.get(), "foo-table");
};

}  // namespace testing
}  // namespace bigtable

#endif  // GOOGLE_CLOUD_CPP_BIGTABLE_CLIENT_TESTING_TABLE_TEST_FIXTURE_H_
