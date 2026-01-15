/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "velox/common/base/tests/GTestUtils.h"
#include "velox/functions/prestosql/tests/utils/FunctionBaseTest.h"

using namespace facebook::velox::test;

namespace facebook::velox::functions {
namespace {

class MapUpdateTest : public test::FunctionBaseTest {
 public:
  template <typename T>
  void testFloatNaNs() {
    static const auto kNaN = std::numeric_limits<T>::quiet_NaN();

    auto data = makeRowVector({
        makeMapVectorFromJson<T, int32_t>({
            "{1:10, NaN:20, 3:30, 4:40, 5:50}",
            "{NaN:20}",
        }),
        makeArrayVector<T>({{1, kNaN, 5}, {kNaN}}),
        makeArrayVector<int32_t>({{100, 200, 500}, {999}}),
    });

    auto expected = makeMapVectorFromJson<T, int32_t>({
        "{3:30, 4:40, 1:100, NaN:200, 5:500}",
        "{NaN:999}",
    });
    auto result = evaluate("map_update(c0, c1, c2)", data);
    assertEqualVectors(expected, result);
  }
};

TEST_F(MapUpdateTest, basicUpdate) {
  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{1:10, 2:20, 3:30}",
          "{1:10, 2:20}",
          "{}",
      }),
      makeArrayVectorFromJson<int64_t>({
          "[2, 3]",
          "[1]",
          "[1, 2]",
      }),
      makeArrayVectorFromJson<int32_t>({
          "[200, 300]",
          "[100]",
          "[10, 20]",
      }),
  });

  auto expected = makeMapVectorFromJson<int64_t, int32_t>({
      "{1:10, 2:200, 3:300}",
      "{2:20, 1:100}",
      "{1:10, 2:20}",
  });

  auto result = evaluate("map_update(c0, c1, c2)", data);
  assertEqualVectors(expected, result);
}

TEST_F(MapUpdateTest, addNewKeys) {
  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{1:10, 2:20}",
          "{}",
      }),
      makeArrayVectorFromJson<int64_t>({
          "[3, 4]",
          "[1, 2, 3]",
      }),
      makeArrayVectorFromJson<int32_t>({
          "[30, 40]",
          "[10, 20, 30]",
      }),
  });

  auto expected = makeMapVectorFromJson<int64_t, int32_t>({
      "{1:10, 2:20, 3:30, 4:40}",
      "{1:10, 2:20, 3:30}",
  });

  auto result = evaluate("map_update(c0, c1, c2)", data);
  assertEqualVectors(expected, result);
}

TEST_F(MapUpdateTest, mixedUpdateAndAdd) {
  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{1:10, 2:20, 3:30}",
          "{1:10, 2:20, 3:30, 4:40}",
      }),
      makeArrayVectorFromJson<int64_t>({
          "[2, 4, 5]",
          "[1, 5]",
      }),
      makeArrayVectorFromJson<int32_t>({
          "[200, 400, 500]",
          "[100, 500]",
      }),
  });

  auto expected = makeMapVectorFromJson<int64_t, int32_t>({
      "{1:10, 3:30, 2:200, 4:400, 5:500}",
      "{2:20, 3:30, 4:40, 1:100, 5:500}",
  });

  auto result = evaluate("map_update(c0, c1, c2)", data);
  assertEqualVectors(expected, result);
}

TEST_F(MapUpdateTest, emptyKeysAndValues) {
  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{1:10, 2:20, 3:30}",
          "{}",
      }),
      makeArrayVectorFromJson<int64_t>({
          "[]",
          "[]",
      }),
      makeArrayVectorFromJson<int32_t>({
          "[]",
          "[]",
      }),
  });

  auto expected = makeMapVectorFromJson<int64_t, int32_t>({
      "{1:10, 2:20, 3:30}",
      "{}",
  });

  auto result = evaluate("map_update(c0, c1, c2)", data);
  assertEqualVectors(expected, result);
}

TEST_F(MapUpdateTest, nullValuesInInput) {
  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{1:10, 2:null, 3:30}",
          "{1:null, 2:20}",
      }),
      makeArrayVectorFromJson<int64_t>({
          "[2, 4]",
          "[1]",
      }),
      makeArrayVectorFromJson<int32_t>({
          "[200, 400]",
          "[100]",
      }),
  });

  auto expected = makeMapVectorFromJson<int64_t, int32_t>({
      "{1:10, 3:30, 2:200, 4:400}",
      "{2:20, 1:100}",
  });

  auto result = evaluate("map_update(c0, c1, c2)", data);
  assertEqualVectors(expected, result);
}

TEST_F(MapUpdateTest, nullValuesInUpdate) {
  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{1:10, 2:20, 3:30}",
          "{1:10, 2:20}",
      }),
      makeArrayVectorFromJson<int64_t>({
          "[2, 4]",
          "[1, 3]",
      }),
      makeNullableArrayVector<int32_t>({
          {std::nullopt, 400},
          {std::nullopt, 300},
      }),
  });

  auto expected = makeMapVectorFromJson<int64_t, int32_t>({
      "{1:10, 3:30, 2:null, 4:400}",
      "{2:20, 1:null, 3:300}",
  });

  auto result = evaluate("map_update(c0, c1, c2)", data);
  assertEqualVectors(expected, result);
}

TEST_F(MapUpdateTest, varcharKey) {
  auto data = makeRowVector({
      makeMapVectorFromJson<std::string, int32_t>({
          R"({"apple": 1, "banana": 2, "cherry": 3})",
          R"({"banana": 2, "orange": 4})",
      }),
      makeArrayVectorFromJson<std::string>({
          R"(["apple", "date"])",
          R"(["banana", "grape"])",
      }),
      makeArrayVectorFromJson<int32_t>({
          "[100, 400]",
          "[200, 500]",
      }),
  });

  auto expected = makeMapVectorFromJson<std::string, int32_t>({
      R"({"banana": 2, "cherry": 3, "apple": 100, "date": 400})",
      R"({"orange": 4, "banana": 200, "grape": 500})",
  });

  auto result = evaluate("map_update(c0, c1, c2)", data);
  assertEqualVectors(expected, result);
}

TEST_F(MapUpdateTest, duplicateKeysInUpdateArrayThrows) {
  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{1:10, 2:20}",
      }),
      makeArrayVectorFromJson<int64_t>({
          "[1, 1, 2]",
      }),
      makeArrayVectorFromJson<int32_t>({
          "[100, 200, 300]",
      }),
  });

  VELOX_ASSERT_THROW(
      evaluate("map_update(c0, c1, c2)", data), "Duplicate key in keys array");
}

TEST_F(MapUpdateTest, mismatchedKeysValuesLengthThrows) {
  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{1:10, 2:20}",
      }),
      makeArrayVectorFromJson<int64_t>({
          "[1, 2, 3]",
      }),
      makeArrayVectorFromJson<int32_t>({
          "[100, 200]",
      }),
  });

  VELOX_ASSERT_THROW(
      evaluate("map_update(c0, c1, c2)", data),
      "Keys and values arrays must have the same length");
}

TEST_F(MapUpdateTest, nullKeysInUpdateArrayAreIgnored) {
  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{1:10, 2:20, 3:30}",
      }),
      makeNullableArrayVector<int64_t>({
          {std::nullopt, 2, std::nullopt, 4},
      }),
      makeArrayVectorFromJson<int32_t>({
          "[100, 200, 300, 400]",
      }),
  });

  auto expected =
      makeMapVectorFromJson<int64_t, int32_t>({"{1:10, 3:30, 2:200, 4:400}"});

  auto result = evaluate("map_update(c0, c1, c2)", data);
  assertEqualVectors(expected, result);
}

TEST_F(MapUpdateTest, floatNaNs) {
  testFloatNaNs<float>();
  testFloatNaNs<double>();
}

TEST_F(MapUpdateTest, booleanKey) {
  auto data = makeRowVector({
      makeMapVector<bool, int32_t>({{{true, 10}, {false, 20}}}),
      makeArrayVector<bool>({{true}}),
      makeArrayVector<int32_t>({{100}}),
  });

  auto expected = makeMapVector<bool, int32_t>({{{false, 20}, {true, 100}}});

  auto result = evaluate("map_update(c0, c1, c2)", data);
  assertEqualVectors(expected, result);
}

TEST_F(MapUpdateTest, allKeysUpdated) {
  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{1:10, 2:20, 3:30}",
      }),
      makeArrayVectorFromJson<int64_t>({
          "[1, 2, 3]",
      }),
      makeArrayVectorFromJson<int32_t>({
          "[100, 200, 300]",
      }),
  });

  auto expected = makeMapVectorFromJson<int64_t, int32_t>({
      "{1:100, 2:200, 3:300}",
  });

  auto result = evaluate("map_update(c0, c1, c2)", data);
  assertEqualVectors(expected, result);
}

TEST_F(MapUpdateTest, largeMap) {
  // Create a map with 10 entries and update/add 3
  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{1:10, 2:20, 3:30, 4:40, 5:50, 6:60, 7:70, 8:80, 9:90, 10:100}",
      }),
      makeArrayVectorFromJson<int64_t>({
          "[5, 10, 11]",
      }),
      makeArrayVectorFromJson<int32_t>({
          "[500, 1000, 1100]",
      }),
  });

  auto result = evaluate("map_update(c0, c1, c2)", data);

  auto mapResult = result->as<MapVector>();
  ASSERT_EQ(mapResult->size(), 1);
}

TEST_F(MapUpdateTest, timestampKeys) {
  auto data = makeRowVector({
      makeMapVector<Timestamp, int32_t>(
          {{{Timestamp(1, 0), 10}, {Timestamp(2, 0), 20}}}),
      makeArrayVector<Timestamp>({{Timestamp(2, 0), Timestamp(3, 0)}}),
      makeArrayVector<int32_t>({{200, 30}}),
  });

  auto result = evaluate("map_update(c0, c1, c2)", data);

  auto expected = makeMapVector<Timestamp, int32_t>(
      {{{Timestamp(1, 0), 10}, {Timestamp(2, 0), 200}, {Timestamp(3, 0), 30}}});

  assertEqualVectors(expected, result);
}

TEST_F(MapUpdateTest, preserveUnchangedKeys) {
  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{5:50, 3:30, 1:10}",
      }),
      makeArrayVectorFromJson<int64_t>({
          "[2, 4]",
      }),
      makeArrayVectorFromJson<int32_t>({
          "[20, 40]",
      }),
  });

  auto result = evaluate("map_update(c0, c1, c2)", data);

  auto expected = makeMapVectorFromJson<int64_t, int32_t>({
      "{5:50, 3:30, 1:10, 2:20, 4:40}",
  });

  assertEqualVectors(expected, result);
}

TEST_F(MapUpdateTest, updateToNull) {
  // Using explicit vector type to avoid ambiguity
  const std::vector<std::vector<std::optional<int32_t>>> valuesData = {
      {std::nullopt},
  };

  auto data = makeRowVector({
      makeMapVectorFromJson<int64_t, int32_t>({
          "{1:10, 2:20}",
      }),
      makeArrayVectorFromJson<int64_t>({
          "[1]",
      }),
      makeNullableArrayVector<int32_t>(valuesData),
  });

  auto expected = makeMapVectorFromJson<int64_t, int32_t>({
      "{2:20, 1:null}",
  });

  auto result = evaluate("map_update(c0, c1, c2)", data);
  assertEqualVectors(expected, result);
}

} // namespace
} // namespace facebook::velox::functions
