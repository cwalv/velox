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

#include <cstdint>
#include <optional>

#include <gtest/gtest.h>

#include "velox/functions/prestosql/tests/utils/FunctionBaseTest.h"
#include "velox/type/StringView.h"

using namespace facebook::velox::test;

namespace facebook::velox::functions {
namespace {

class MapTrimValuesTest : public test::FunctionBaseTest {};

TEST_F(MapTrimValuesTest, basicTrimming) {
  // Create input map: {1->[10,20,30], 2->[40,50,60]}
  auto keys = makeFlatVector<int32_t>({1, 2});
  auto valueArrays = makeArrayVector<int64_t>({{10, 20, 30}, {40, 50, 60}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 2)", makeRowVector({inputMap}));

  // Expected: {1->[10,20], 2->[40,50]}
  auto expectedKeys = makeFlatVector<int32_t>({1, 2});
  auto expectedValues = makeArrayVector<int64_t>({{10, 20}, {40, 50}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, trimToZero) {
  // Create input map: {1->[10,20], 2->[30,40,50]}
  auto keys = makeFlatVector<int32_t>({1, 2});
  auto valueArrays = makeArrayVector<int64_t>({{10, 20}, {30, 40, 50}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 0)", makeRowVector({inputMap}));

  // Expected: {1->[], 2->[]}
  auto expectedKeys = makeFlatVector<int32_t>({1, 2});
  auto expectedValues = makeArrayVector<int64_t>({{}, {}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, trimLargerThanArraySize) {
  // Create input map: {1->[10,20], 2->[30]}
  auto keys = makeFlatVector<int32_t>({1, 2});
  auto valueArrays = makeArrayVector<int64_t>({{10, 20}, {30}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 100)", makeRowVector({inputMap}));

  // Expected: {1->[10,20], 2->[30]} (no change)
  auto expectedKeys = makeFlatVector<int32_t>({1, 2});
  auto expectedValues = makeArrayVector<int64_t>({{10, 20}, {30}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, negativeN) {
  // Create input map: {1->[10,20,30]}
  auto keys = makeFlatVector<int32_t>({1});
  auto valueArrays = makeArrayVector<int64_t>({{10, 20, 30}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, -1)", makeRowVector({inputMap}));

  // Expected: {1->[10,20,30]} (no change)
  auto expectedKeys = makeFlatVector<int32_t>({1});
  auto expectedValues = makeArrayVector<int64_t>({{10, 20, 30}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, emptyMap) {
  // Create empty input map
  auto keys = makeFlatVector<int32_t>({});
  auto valueArrays = makeArrayVector<int64_t>({});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 2)", makeRowVector({inputMap}));

  // Expected: {} (empty map)
  auto expectedKeys = makeFlatVector<int32_t>({});
  auto expectedValues = makeArrayVector<int64_t>({});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, emptyValueArrays) {
  // Create input map: {1->[], 2->[]}
  auto keys = makeFlatVector<int32_t>({1, 2});
  auto valueArrays = makeArrayVector<int64_t>({{}, {}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 5)", makeRowVector({inputMap}));

  // Expected: {1->[], 2->[]} (no change)
  auto expectedKeys = makeFlatVector<int32_t>({1, 2});
  auto expectedValues = makeArrayVector<int64_t>({{}, {}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, nullValuesInArray) {
  // Create input map: {1->[10,null,30], 2->[null,50]}
  auto keys = makeFlatVector<int32_t>({1, 2});
  auto valueArrays = makeNullableArrayVector<int64_t>(
      {{10, std::nullopt, 30}, {std::nullopt, 50}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 2)", makeRowVector({inputMap}));

  // Expected: {1->[10,null], 2->[null,50]}
  auto expectedKeys = makeFlatVector<int32_t>({1, 2});
  auto expectedValues = makeNullableArrayVector<int64_t>(
      {{10, std::nullopt}, {std::nullopt, 50}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, stringKeys) {
  // Create input map: {'apple'->[1,2,3], 'banana'->[4,5,6]}
  auto keys = makeFlatVector<StringView>({"apple", "banana"});
  auto valueArrays = makeArrayVector<int64_t>({{1, 2, 3}, {4, 5, 6}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 2)", makeRowVector({inputMap}));

  // Expected: {'apple'->[1,2], 'banana'->[4,5]}
  auto expectedKeys = makeFlatVector<StringView>({"apple", "banana"});
  auto expectedValues = makeArrayVector<int64_t>({{1, 2}, {4, 5}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, stringValues) {
  // Create input map: {1->['a','b','c'], 2->['d','e','f']}
  auto keys = makeFlatVector<int32_t>({1, 2});
  auto valueArrays =
      makeArrayVector<StringView>({{"a", "b", "c"}, {"d", "e", "f"}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 2)", makeRowVector({inputMap}));

  // Expected: {1->['a','b'], 2->['d','e']}
  auto expectedKeys = makeFlatVector<int32_t>({1, 2});
  auto expectedValues = makeArrayVector<StringView>({{"a", "b"}, {"d", "e"}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, floatValues) {
  // Create input map: {1->[1.1,2.2,3.3], 2->[4.4,5.5,6.6]}
  auto keys = makeFlatVector<int32_t>({1, 2});
  auto valueArrays =
      makeArrayVector<double>({{1.1, 2.2, 3.3}, {4.4, 5.5, 6.6}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 2)", makeRowVector({inputMap}));

  // Expected: {1->[1.1,2.2], 2->[4.4,5.5]}
  auto expectedKeys = makeFlatVector<int32_t>({1, 2});
  auto expectedValues = makeArrayVector<double>({{1.1, 2.2}, {4.4, 5.5}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, singleElementArrays) {
  // Create input map: {1->[100], 2->[200]}
  auto keys = makeFlatVector<int32_t>({1, 2});
  auto valueArrays = makeArrayVector<int64_t>({{100}, {200}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 1)", makeRowVector({inputMap}));

  // Expected: {1->[100], 2->[200]} (no change)
  auto expectedKeys = makeFlatVector<int32_t>({1, 2});
  auto expectedValues = makeArrayVector<int64_t>({{100}, {200}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, mixedArraySizes) {
  // Create input map: {1->[10], 2->[20,30,40], 3->[50,60,70]}
  auto keys = makeFlatVector<int32_t>({1, 2, 3});
  auto valueArrays =
      makeArrayVector<int64_t>({{10}, {20, 30, 40}, {50, 60, 70}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 2)", makeRowVector({inputMap}));

  // Expected: {1->[10], 2->[20,30], 3->[50,60]}
  auto expectedKeys = makeFlatVector<int32_t>({1, 2, 3});
  auto expectedValues = makeArrayVector<int64_t>({{10}, {20, 30}, {50, 60}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, booleanValues) {
  // Create input map: {1->[true,false,true], 2->[false,true,false]}
  auto keys = makeFlatVector<int32_t>({1, 2});
  auto valueArrays =
      makeArrayVector<bool>({{true, false, true}, {false, true, false}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 2)", makeRowVector({inputMap}));

  // Expected: {1->[true,false], 2->[false,true]}
  auto expectedKeys = makeFlatVector<int32_t>({1, 2});
  auto expectedValues = makeArrayVector<bool>({{true, false}, {false, true}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, trimToOne) {
  // Create input map: {1->[10,20,30], 2->[40,50,60]}
  auto keys = makeFlatVector<int32_t>({1, 2});
  auto valueArrays = makeArrayVector<int64_t>({{10, 20, 30}, {40, 50, 60}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 1)", makeRowVector({inputMap}));

  // Expected: {1->[10], 2->[40]}
  auto expectedKeys = makeFlatVector<int32_t>({1, 2});
  auto expectedValues = makeArrayVector<int64_t>({{10}, {40}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, singleKeyMap) {
  // Create input map: {1->[10,20,30,40,50]}
  auto keys = makeFlatVector<int32_t>({1});
  auto valueArrays = makeArrayVector<int64_t>({{10, 20, 30, 40, 50}});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 3)", makeRowVector({inputMap}));

  // Expected: {1->[10,20,30]}
  auto expectedKeys = makeFlatVector<int32_t>({1});
  auto expectedValues = makeArrayVector<int64_t>({{10, 20, 30}});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, multipleRows) {
  // Create two rows with different maps
  // Row 0: {1->[10,20,30], 2->[40,50,60]}
  // Row 1: {3->[70,80,90], 4->[100,110,120]}
  auto keys = makeFlatVector<int32_t>({1, 2, 3, 4});
  auto valueArrays = makeArrayVector<int64_t>(
      {{10, 20, 30}, {40, 50, 60}, {70, 80, 90}, {100, 110, 120}});
  auto inputMap = makeMapVector({0, 2}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 2)", makeRowVector({inputMap}));

  // Expected:
  // Row 0: {1->[10,20], 2->[40,50]}
  // Row 1: {3->[70,80], 4->[100,110]}
  auto expectedKeys = makeFlatVector<int32_t>({1, 2, 3, 4});
  auto expectedValues =
      makeArrayVector<int64_t>({{10, 20}, {40, 50}, {70, 80}, {100, 110}});
  auto expected = makeMapVector({0, 2}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, nullMapValue) {
  // Create input map where a value is null: {1->[10,20,30], 2->null}
  auto keys = makeFlatVector<int32_t>({1, 2});
  auto valueArrays =
      makeNullableArrayVector<int64_t>({{{10, 20, 30}}, std::nullopt});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, 2)", makeRowVector({inputMap}));

  // Expected: {1->[10,20], 2->null}
  auto expectedKeys = makeFlatVector<int32_t>({1, 2});
  auto expectedValues =
      makeNullableArrayVector<int64_t>({{{10, 20}}, std::nullopt});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

TEST_F(MapTrimValuesTest, nullMapValueNegativeN) {
  // Create input map where a value is null: {1->[10,20,30], 2->null}
  auto keys = makeFlatVector<int32_t>({1, 2});
  auto valueArrays =
      makeNullableArrayVector<int64_t>({{{10, 20, 30}}, std::nullopt});
  auto inputMap = makeMapVector({0}, keys, valueArrays);

  auto result = evaluate("map_trim_values(c0, -1)", makeRowVector({inputMap}));

  // Expected: {1->[10,20,30], 2->null} (no change)
  auto expectedKeys = makeFlatVector<int32_t>({1, 2});
  auto expectedValues =
      makeNullableArrayVector<int64_t>({{{10, 20, 30}}, std::nullopt});
  auto expected = makeMapVector({0}, expectedKeys, expectedValues);

  assertEqualVectors(expected, result);
}

} // namespace
} // namespace facebook::velox::functions
