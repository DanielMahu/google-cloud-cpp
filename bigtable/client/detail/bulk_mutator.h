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

#ifndef GOOGLE_CLOUD_CPP_BIGTABLE_CLIENT_DETAIL_BULK_MUTATOR_H_
#define GOOGLE_CLOUD_CPP_BIGTABLE_CLIENT_DETAIL_BULK_MUTATOR_H_

#include "bigtable/client/idempotent_mutation_policy.h"

#include <google/bigtable/v2/bigtable.grpc.pb.h>

namespace bigtable {
inline namespace BIGTABLE_CLIENT_NS {
namespace detail {
/// Keep the state in the Table::BulkApply() member function.
class BulkMutator {
 public:
  BulkMutator(std::string const& table_name,
              IdempotentMutationPolicy& idempotent_policy, BulkMutation&& mut);

  /// Return true if there are pending mutations in the mutator
  bool HasPendingMutations() const {
    return pending_mutations_.entries_size() != 0;
  }

  /// Send one batch request to the given stub.
  grpc::Status MakeOneRequest(
      google::bigtable::v2::Bigtable::StubInterface& stub,
      grpc::ClientContext& client_context);

  /// Give up on any pending mutations, move them to the failures array.
  std::vector<FailedMutation> ExtractFinalFailures();

 private:
  /// Get ready for a new request.
  void PrepareForRequest();

  /// Process a single response.
  void ProcessResponse(google::bigtable::v2::MutateRowsResponse& response);

  /// A request has finished and we have processed all the responses.
  void FinishRequest();

 private:
  /// Accumulate any permanent failures and the list of mutations we gave up on.
  std::vector<FailedMutation> failures_;

  /// The current request proto.
  google::bigtable::v2::MutateRowsRequest mutations_;

  /**
   * A small type to keep the annotations about pending mutations.
   *
   * As we process a MutateRows RPC we need to track the partial results for
   * each mutation in the request.  This object groups them in a small POD-type.
   */
  struct Annotations {
    /**
     * The index of this mutation in the original request.
     *
     * Each time the request is retried the operations might be reordered, but
     * we want to report any permanent failures using the index in the original
     * request provided by the application.
     */
    int original_index;
    bool is_idempotent;
    /// Set to false if the result is unknown.
    bool has_mutation_result;
  };

  /// The annotations about the current bulk request.
  std::vector<Annotations> annotations_;

  /// Accumulate mutations for the next request.
  google::bigtable::v2::MutateRowsRequest pending_mutations_;

  /// Accumulate annotations for the next request.
  std::vector<Annotations> pending_annotations_;
};
}  // namespace detail
}  // namespace BIGTABLE_CLIENT_NS
}  // namespace bigtable

#endif  // GOOGLE_CLOUD_CPP_BIGTABLE_CLIENT_DETAIL_BULK_MUTATOR_H_
