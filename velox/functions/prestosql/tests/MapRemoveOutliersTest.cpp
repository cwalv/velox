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

#include "velox/functions/prestosql/tests/utils/FunctionBaseTest.h"

using namespace facebook::velox::test;

namespace facebook::velox::functions {
namespace {

class MapRemoveOutliersTest : public test::FunctionBaseTest {
 protected:
  void testMapRemoveOutliers(
      const std::string& expression,
      const std::vector<VectorPtr>& input,
      const VectorPtr& expected) {
    auto result = evaluate(expression, makeRowVector(input));
    assertEqualVectors(expected, result);
  }
};

TEST_F(MapRemoveOutliersTest, basicInteger) {
  auto inputMap = makeMapVectorFromJson<int64_t, int64_t>({
      "{1:10, 2:20, 3:30, 4:40, 5:50}",
      "{1:5, 2:15, 3:25, 4:35, 5:100}",
      "{}",
      "{1:1, 2:2, 3:3}",
  });

  auto expected = makeMapVectorFromJson<int64_t, int64_t>({
      "{2:20, 3:30, 4:40}",
      "{2:15, 3:25, 4:35}",
      "{}",
      "{}",
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(15 as bigint), cast(40 as bigint))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, basicDouble) {
  auto inputMap = makeMapVectorFromJson<int64_t, double>({
      "{1:1.5, 2:2.5, 3:3.5, 4:4.5, 5:5.5}",
      "{1:0.5, 2:1.5, 3:10.5}",
  });

  auto expected = makeMapVectorFromJson<int64_t, double>({
      "{2:2.5, 3:3.5, 4:4.5}",
      "{}",
  });

  auto result =
      evaluate("map_remove_outliers(c0, 2.0, 5.0)", makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, varcharKey) {
  auto inputMap = makeMapVectorFromJson<std::string, int64_t>({
      R"({"apple":10, "banana":20, "cherry":30, "date":40})",
      R"({"x":5, "y":50, "z":25})",
  });

  auto expected = makeMapVectorFromJson<std::string, int64_t>({
      R"({"banana":20, "cherry":30})",
      R"({"z":25})",
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(15 as bigint), cast(35 as bigint))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, emptyMap) {
  auto inputMap =
      makeMapVectorFromJson<int64_t, int64_t>({"{}", "{}", "{1:10}"});

  auto expected =
      makeMapVectorFromJson<int64_t, int64_t>({"{}", "{}", "{1:10}"});

  auto result = evaluate(
      "map_remove_outliers(c0, cast(0 as bigint), cast(100 as bigint))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, allValuesOutOfRange) {
  auto inputMap = makeMapVectorFromJson<int64_t, int64_t>({
      "{1:1, 2:2, 3:3}",
      "{1:100, 2:200, 3:300}",
  });

  auto expected = makeMapVectorFromJson<int64_t, int64_t>({
      "{}",
      "{}",
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(50 as bigint), cast(60 as bigint))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, allValuesInRange) {
  auto inputMap = makeMapVectorFromJson<int64_t, int64_t>({
      "{1:10, 2:20, 3:30}",
      "{1:15, 2:25}",
  });

  auto expected = makeMapVectorFromJson<int64_t, int64_t>({
      "{1:10, 2:20, 3:30}",
      "{1:15, 2:25}",
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(0 as bigint), cast(100 as bigint))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, boundaryValues) {
  auto inputMap = makeMapVectorFromJson<int64_t, int64_t>({
      "{1:10, 2:20, 3:30, 4:40, 5:50}",
  });

  auto expected = makeMapVectorFromJson<int64_t, int64_t>({
      "{1:10, 2:20, 3:30, 4:40, 5:50}",
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(10 as bigint), cast(50 as bigint))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, nullValuesPreserved) {
  auto inputMap = makeNullableMapVector<int64_t, int64_t>({
      {{{1, 10}, {2, std::nullopt}, {3, 30}, {4, std::nullopt}, {5, 50}}},
      {{{1, std::nullopt}, {2, 20}}},
  });

  auto expected = makeNullableMapVector<int64_t, int64_t>({
      {{{2, std::nullopt}, {3, 30}, {4, std::nullopt}}},
      {{{1, std::nullopt}, {2, 20}}},
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(15 as bigint), cast(45 as bigint))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, floatValues) {
  auto inputMap = makeMapVectorFromJson<int64_t, double>({
      "{1:1.1, 2:2.2, 3:3.3, 4:4.4, 5:5.5}",
      "{1:0.1, 2:10.1}",
  });

  auto expected = makeMapVectorFromJson<int64_t, double>({
      "{2:2.2, 3:3.3, 4:4.4}",
      "{}",
  });

  auto result =
      evaluate("map_remove_outliers(c0, 2.0, 5.0)", makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, negativeBounds) {
  auto inputMap = makeMapVectorFromJson<int64_t, int64_t>({
      "{1:-50, 2:-20, 3:0, 4:20, 5:50}",
  });

  auto expected = makeMapVectorFromJson<int64_t, int64_t>({
      "{2:-20, 3:0, 4:20}",
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(-30 as bigint), cast(30 as bigint))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, singleElementMap) {
  auto inputMap = makeMapVectorFromJson<int64_t, int64_t>({
      "{1:25}",
      "{1:5}",
      "{1:100}",
  });

  auto expected = makeMapVectorFromJson<int64_t, int64_t>({
      "{1:25}",
      "{}",
      "{}",
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(10 as bigint), cast(50 as bigint))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, int32KeyAndValue) {
  auto inputMap = makeMapVectorFromJson<int32_t, int32_t>({
      "{1:10, 2:20, 3:30, 4:40}",
      "{1:5, 2:25, 3:45}",
  });

  auto expected = makeMapVectorFromJson<int32_t, int32_t>({
      "{2:20, 3:30}",
      "{2:25}",
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(15 as integer), cast(35 as integer))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, varcharKeyDoubleValue) {
  auto inputMap = makeMapVectorFromJson<std::string, double>({
      R"({"a":1.5, "b":2.5, "c":3.5, "d":4.5})",
      R"({"x":0.5, "y":5.5})",
  });

  auto expected = makeMapVectorFromJson<std::string, double>({
      R"({"b":2.5, "c":3.5})",
      "{}",
  });

  auto result =
      evaluate("map_remove_outliers(c0, 2.0, 4.0)", makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, sameLowerAndUpperBound) {
  auto inputMap = makeMapVectorFromJson<int64_t, int64_t>({
      "{1:10, 2:25, 3:25, 4:30}",
      "{1:25, 2:25}",
  });

  auto expected = makeMapVectorFromJson<int64_t, int64_t>({
      "{2:25, 3:25}",
      "{1:25, 2:25}",
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(25 as bigint), cast(25 as bigint))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, largeMap) {
  auto inputMap = makeMapVectorFromJson<int64_t, int64_t>({
      "{1:10, 2:20, 3:30, 4:40, 5:50, 6:60, 7:70, 8:80, 9:90, 10:100}",
  });

  auto expected = makeMapVectorFromJson<int64_t, int64_t>({
      "{2:20, 3:30, 4:40, 5:50, 6:60, 7:70, 8:80}",
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(20 as bigint), cast(80 as bigint))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, varcharKeyFloatValue) {
  auto inputMap = makeMapVectorFromJson<std::string, float>({
      R"({"temp1":20.5, "temp2":25.0, "temp3":30.5, "temp4":35.0})",
      R"({"sensor1":10.0, "sensor2":50.0})",
  });

  auto expected = makeMapVectorFromJson<std::string, float>({
      R"({"temp2":25.0, "temp3":30.5})",
      "{}",
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(22.0 as real), cast(32.0 as real))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

TEST_F(MapRemoveOutliersTest, extremeValues) {
  auto inputMap = makeMapVectorFromJson<int64_t, int64_t>({
      "{1:-9223372036854775808, 2:0, 3:9223372036854775807}",
  });

  auto expected = makeMapVectorFromJson<int64_t, int64_t>({
      "{2:0}",
  });

  auto result = evaluate(
      "map_remove_outliers(c0, cast(-100 as bigint), cast(100 as bigint))",
      makeRowVector({inputMap}));
  assertEqualVectors(expected, result);
}

} // namespace
} // namespace facebook::velox::functions
