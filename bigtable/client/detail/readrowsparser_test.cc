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

#include "bigtable/client/detail/readrowsparser.h"

#include <absl/strings/str_join.h>
#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include <numeric>
#include <sstream>
#include <vector>

using google::bigtable::v2::ReadRowsResponse_CellChunk;

TEST(ReadRowsParserTest, NoChunksNoRowsSucceeds) {
  bigtable::ReadRowsParser parser;

  EXPECT_FALSE(parser.HasNext());
  parser.HandleEndOfStream();
  EXPECT_FALSE(parser.HasNext());
}

TEST(ReadRowsParserTest, HandleEndOfStreamCalledTwiceThrows) {
  bigtable::ReadRowsParser parser;

  EXPECT_FALSE(parser.HasNext());
  parser.HandleEndOfStream();
  EXPECT_THROW(parser.HandleEndOfStream(), std::exception);
  EXPECT_FALSE(parser.HasNext());
}

TEST(ReadRowsParserTest, HandleChunkAfterEndOfStreamThrows) {
  bigtable::ReadRowsParser parser;
  ReadRowsResponse_CellChunk chunk;
  chunk.set_value_size(1);

  EXPECT_FALSE(parser.HasNext());
  parser.HandleEndOfStream();
  EXPECT_THROW(parser.HandleChunk(chunk), std::exception);
  EXPECT_FALSE(parser.HasNext());
}

TEST(ReadRowsParserTest, SingleChunkSucceeds) {
  using google::protobuf::TextFormat;
  bigtable::ReadRowsParser parser;
  ReadRowsResponse_CellChunk chunk;
  std::string chunk1 = R"(
    row_key: "RK"
    family_name: < value: "F">
    qualifier: < value: "C">
    timestamp_micros: 42
    value: "V"
    commit_row: true
    )";
  ASSERT_TRUE(TextFormat::ParseFromString(chunk1, &chunk));

  EXPECT_FALSE(parser.HasNext());
  parser.HandleChunk(chunk);
  EXPECT_TRUE(parser.HasNext());

  std::vector<bigtable::Row> rows;
  rows.emplace_back(parser.Next());
  EXPECT_FALSE(parser.HasNext());
  ASSERT_EQ(1U, rows[0].cells().size());
  auto cell_it = rows[0].cells().begin();
  EXPECT_EQ("RK", cell_it->row_key());
  EXPECT_EQ("F", cell_it->family_name());
  EXPECT_EQ("C", cell_it->column_qualifier());
  EXPECT_EQ("V", cell_it->value());
  EXPECT_EQ(42, cell_it->timestamp());

  parser.HandleEndOfStream();
}

TEST(ReadRowsParserTest, NextAfterEndOfStreamSucceeds) {
  using google::protobuf::TextFormat;
  bigtable::ReadRowsParser parser;
  ReadRowsResponse_CellChunk chunk;
  std::string chunk1 = R"(
    row_key: "RK"
    family_name: < value: "F">
    qualifier: < value: "C">
    timestamp_micros: 42
    value: "V"
    commit_row: true
    )";
  ASSERT_TRUE(TextFormat::ParseFromString(chunk1, &chunk));

  EXPECT_FALSE(parser.HasNext());
  parser.HandleChunk(chunk);
  parser.HandleEndOfStream();

  EXPECT_TRUE(parser.HasNext());
  ASSERT_EQ(1U, parser.Next().cells().size());

  EXPECT_FALSE(parser.HasNext());
}

TEST(ReadRowsParserTest, NextWithNoDataThrows) {
  bigtable::ReadRowsParser parser;

  EXPECT_FALSE(parser.HasNext());
  parser.HandleEndOfStream();

  EXPECT_FALSE(parser.HasNext());
  EXPECT_THROW(parser.Next(), std::exception);
}

// **** Acceptance tests helpers ****

namespace bigtable {
namespace {

// Can also be used by gtest to print Cell values
void PrintTo(Cell const& c, std::ostream* os) {
  *os << "rk: " << std::string(c.row_key()) << "\n";
  *os << "fm: " << std::string(c.family_name()) << "\n";
  *os << "qual: " << std::string(c.column_qualifier()) << "\n";
  *os << "ts: " << c.timestamp() << "\n";
  *os << "value: " << std::string(c.value()) << "\n";
  *os << "label: " << absl::StrJoin(c.labels(), ",") << "\n";
}

std::string CellToString(Cell const& cell) {
  std::stringstream ss;
  PrintTo(cell, &ss);
  return ss.str();
}

}  // namespace
}  // namespace bigtable

class AcceptanceTest : public ::testing::Test {
 protected:
  std::vector<std::string> ExtractCells() {
    std::vector<std::string> cells;

    for (auto const& r : rows_) {
      std::transform(r.cells().begin(), r.cells().end(),
                     std::back_inserter(cells), bigtable::CellToString);
    }
    return cells;
  }

  std::vector<ReadRowsResponse_CellChunk> ConvertChunks(
      std::vector<std::string> chunk_strings) {
    using google::protobuf::TextFormat;

    std::vector<ReadRowsResponse_CellChunk> chunks;
    for (std::string const& chunk_string : chunk_strings) {
      ReadRowsResponse_CellChunk chunk;
      if (not TextFormat::ParseFromString(chunk_string, &chunk)) {
        return {};
      }
      chunks.emplace_back(std::move(chunk));
    }

    return chunks;
  }

  void FeedChunks(std::vector<ReadRowsResponse_CellChunk> chunks) {
    for (auto const& chunk : chunks) {
      parser_.HandleChunk(chunk);
      if (parser_.HasNext()) {
        rows_.emplace_back(parser_.Next());
      }
    }
    parser_.HandleEndOfStream();
  }

 private:
  bigtable::ReadRowsParser parser_;
  std::vector<bigtable::Row> rows_;
};

// Auto-generated acceptance tests
#include "bigtable/client/detail/readrowsparser_acceptance_tests.inc"
