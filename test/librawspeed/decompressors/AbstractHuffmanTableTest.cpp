/*
    RawSpeed - RAW file decoder.

    Copyright (C) 2018 Roman Lebedev

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; withexpected even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "decompressors/AbstractHuffmanTable.h" // for AbstractHuffmanTable
#include "common/Common.h"                      // for uchar8
#include "decoders/RawDecoderException.h"       // for RawDecoderException
#include "io/Buffer.h"                          // for Buffer
#include <array>                                // for array
#include <gtest/gtest.h>                        // for AssertionResult, ...
#include <utility>                              // for make_pair, pair, move
#include <utility>                              // for move
#include <vector>                               // for vector

using rawspeed::AbstractHuffmanTable;
using rawspeed::Buffer;
using std::make_tuple;

namespace rawspeed {

bool operator!=(const AbstractHuffmanTable& lhs,
                const AbstractHuffmanTable& rhs) {
  return !(lhs == rhs);
}

} // namespace rawspeed

namespace rawspeed_test {

auto genHT = [](std::initializer_list<rawspeed::uchar8>&& nCodesPerLength)
    -> AbstractHuffmanTable {
  AbstractHuffmanTable ht;
  std::vector<rawspeed::uchar8> v(nCodesPerLength.begin(),
                                  nCodesPerLength.end());
  v.resize(16);
  Buffer b(v.data(), v.size());
  ht.setNCodesPerLength(b);

  return ht;
};

auto genHTCount = [](std::initializer_list<rawspeed::uchar8>&& nCodesPerLength)
    -> rawspeed::uint32 {
  AbstractHuffmanTable ht;
  std::vector<rawspeed::uchar8> v(nCodesPerLength.begin(),
                                  nCodesPerLength.end());
  v.resize(16);
  Buffer b(v.data(), v.size());
  return ht.setNCodesPerLength(b);
};

auto genHTFull = [](std::initializer_list<rawspeed::uchar8>&& nCodesPerLength,
                    std::initializer_list<rawspeed::uchar8>&& codeValues)
    -> AbstractHuffmanTable {
  auto ht = genHT(std::move(nCodesPerLength));
  std::vector<rawspeed::uchar8> v(codeValues.begin(), codeValues.end());
  Buffer b(v.data(), v.size());
  ht.setCodeValues(b);
  return ht;
};

#ifndef NDEBUG
TEST(AbstractHuffmanTableDeathTest, setNCodesPerLengthRequires16Lengths) {
  for (int i = 0; i < 32; i++) {
    std::vector<rawspeed::uchar8> v(i, 1);
    ASSERT_EQ(v.size(), i);

    Buffer b(v.data(), v.size());
    ASSERT_EQ(b.getSize(), v.size());

    AbstractHuffmanTable ht;

    if (b.getSize() != 16) {
      ASSERT_DEATH({ ht.setNCodesPerLength(b); },
                   "setNCodesPerLength.*data.getSize\\(\\) == 16");
    } else {
      ASSERT_EXIT(
          {
            ht.setNCodesPerLength(b);

            exit(0);
          },
          ::testing::ExitedWithCode(0), "");
    }
  }
}
#endif

TEST(AbstractHuffmanTableTest, setNCodesPerLengthEqualCompareAndTrimming) {
  {
    AbstractHuffmanTable a;
    AbstractHuffmanTable b;

    ASSERT_EQ(a, b);
  }

  ASSERT_EQ(genHT({1}), genHT({1}));
  ASSERT_EQ(genHT({1}), genHT({1, 0}));
  ASSERT_EQ(genHT({1, 0}), genHT({1}));
  ASSERT_EQ(genHT({1, 0}), genHT({1, 0}));
  ASSERT_EQ(genHT({0, 1}), genHT({0, 1}));
  ASSERT_EQ(genHT({1, 1}), genHT({1, 1}));

  ASSERT_NE(genHT({1, 0}), genHT({1, 1}));
  ASSERT_NE(genHT({0, 1}), genHT({1}));
  ASSERT_NE(genHT({0, 1}), genHT({1, 0}));
  ASSERT_NE(genHT({0, 1}), genHT({1, 1}));
  ASSERT_NE(genHT({1}), genHT({1, 1}));
}

TEST(AbstractHuffmanTableTest, setNCodesPerLengthEmptyIsBad) {
  ASSERT_THROW(genHT({}), rawspeed::RawDecoderException);
  ASSERT_THROW(genHT({0}), rawspeed::RawDecoderException);
  ASSERT_THROW(genHT({0, 0}), rawspeed::RawDecoderException);
}

TEST(AbstractHuffmanTableTest, setNCodesPerLengthTooManyCodesTotal) {
  ASSERT_NO_THROW(genHT({0, 0, 0, 0, 0, 0, 0, 162}));
  ASSERT_THROW(genHT({0, 0, 0, 0, 0, 0, 0, 163}),
               rawspeed::RawDecoderException);
}

TEST(AbstractHuffmanTableTest, setNCodesPerLengthTooManyCodesForLenght) {
  for (int len = 1; len < 8; len++) {
    AbstractHuffmanTable ht;
    std::vector<rawspeed::uchar8> v(16, 0);
    Buffer b(v.data(), v.size());
    for (auto i = 1U; i <= (1U << len) - 1U; i++) {
      v[len - 1] = i;
      ASSERT_NO_THROW(ht.setNCodesPerLength(b););
    }
    v[len - 1]++;
    ASSERT_THROW(ht.setNCodesPerLength(b), rawspeed::RawDecoderException);
  }
}

TEST(AbstractHuffmanTableTest, setNCodesPerLengthCounts) {
  ASSERT_EQ(genHTCount({1}), 1);
  ASSERT_EQ(genHTCount({1, 0}), 1);
  ASSERT_EQ(genHTCount({0, 1}), 1);
  ASSERT_EQ(genHTCount({0, 2}), 2);
  ASSERT_EQ(genHTCount({0, 3}), 3);
  ASSERT_EQ(genHTCount({1, 1}), 2);
  ASSERT_EQ(genHTCount({1, 2}), 3);
  ASSERT_EQ(genHTCount({1, 3}), 4);
}

#ifndef NDEBUG
TEST(AbstractHuffmanTableDeathTest, setCodeValuesRequiresCount) {
  for (int len = 1; len < 8; len++) {
    AbstractHuffmanTable ht;
    std::vector<rawspeed::uchar8> l(16, 0);
    Buffer bl(l.data(), l.size());
    l[len - 1] = (1U << len) - 1U;
    const auto count = ht.setNCodesPerLength(bl);
    std::vector<rawspeed::uchar8> v;
    v.reserve(count + 1);
    for (auto cnt = count - 1; cnt <= count + 1; cnt++) {
      v.resize(cnt);
      Buffer bv(v.data(), v.size());
      if (cnt != count) {
        ASSERT_DEATH(
            { ht.setCodeValues(bv); },
            "setCodeValues\\(.*\\).*data.getSize\\(\\) == maxCodesCount\\(\\)");
      } else {
        ASSERT_EXIT(
            {
              ht.setCodeValues(bv);
              exit(0);
            },
            ::testing::ExitedWithCode(0), "");
      }
    }
  }
}

TEST(AbstractHuffmanTableDeathTest, setCodeValuesRequiresLessThan162) {
  auto ht = genHT({0, 0, 0, 0, 0, 0, 0, 162});
  std::vector<rawspeed::uchar8> v(163, 0);
  Buffer bv(v.data(), v.size());
  ASSERT_DEATH({ ht.setCodeValues(bv); },
               "setCodeValues\\(.*\\).*data.getSize\\(\\) <= 162");
}
#endif

TEST(AbstractHuffmanTableTest, setCodeValuesValueLessThan16) {
  auto ht = genHT({1});
  std::vector<rawspeed::uchar8> v(1);

  for (int i = 0; i < 256; i++) {
    v[0] = i;
    Buffer b(v.data(), v.size());
    if (i <= 16)
      ASSERT_NO_THROW(ht.setCodeValues(b););
    else
      ASSERT_THROW(ht.setCodeValues(b), rawspeed::RawDecoderException);
  }
}

TEST(AbstractHuffmanTableTest, EqualCompareAndTrimming) {
  ASSERT_EQ(genHTFull({1}, {0}), genHTFull({1}, {0}));
  ASSERT_EQ(genHTFull({1}, {1}), genHTFull({1}, {1}));

  ASSERT_EQ(genHTFull({1}, {0}), genHTFull({1, 0}, {0}));
  ASSERT_EQ(genHTFull({1, 0}, {0}), genHTFull({1, 0}, {0}));
  ASSERT_EQ(genHTFull({1, 0}, {0}), genHTFull({1}, {0}));

  ASSERT_NE(genHTFull({1}, {0}), genHTFull({1}, {1}));
  ASSERT_NE(genHTFull({1}, {1}), genHTFull({1}, {0}));

  ASSERT_NE(genHTFull({1}, {0}), genHTFull({1, 0}, {1}));
  ASSERT_NE(genHTFull({1, 0}, {0}), genHTFull({1, 0}, {1}));
  ASSERT_NE(genHTFull({1, 0}, {0}), genHTFull({1}, {1}));
}

using SignExtendDataType =
    std::tr1::tuple<rawspeed::uint32, rawspeed::uint32, int>;
class SignExtendTest : public ::testing::TestWithParam<SignExtendDataType> {
protected:
  SignExtendTest() = default;
  virtual void SetUp() {
    auto p = GetParam();

    diff = std::tr1::get<0>(p);
    len = std::tr1::get<1>(p);
    value = std::tr1::get<2>(p);
  }

  rawspeed::uint32 diff;
  rawspeed::uint32 len;
  int value;
};

auto zeroDiff = [](int len) { return make_tuple(0, len, -((1 << len) - 1)); };
auto passthrough = [](int len) {
  return make_tuple(((1 << len) - 1), len, ((1 << len) - 1));
};
auto one = [](int len) { return make_tuple((1 << len), len, 1); };
static const SignExtendDataType signExtendData[]{
    // clang-format off
    zeroDiff(1),
    zeroDiff(2),
    zeroDiff(3),
    zeroDiff(4),
    zeroDiff(5),
    zeroDiff(6),
    zeroDiff(7),
    zeroDiff(8),
    zeroDiff(9),
    zeroDiff(10),
    zeroDiff(11),
    zeroDiff(12),
    zeroDiff(13),
    zeroDiff(14),
    zeroDiff(15),
    zeroDiff(16),

    passthrough(1),
    passthrough(2),
    passthrough(3),
    passthrough(4),
    passthrough(5),
    passthrough(6),
    passthrough(7),
    passthrough(8),
    passthrough(9),
    passthrough(10),
    passthrough(11),
    passthrough(12),
    passthrough(13),
    passthrough(14),
    passthrough(15),
    passthrough(16),

    one(1),
    one(2),
    one(3),
    one(4),
    one(5),
    one(6),
    one(7),
    one(8),
    one(9),
    one(10),
    one(11),
    one(12),
    one(13),
    one(14),
    one(15),
    one(16),

    make_tuple(0b00, 0b01, -0b001),
    make_tuple(0b01, 0b01,  0b001),
    make_tuple(0b10, 0b01,  0b001),
    make_tuple(0b11, 0b01,  0b011),
    make_tuple(0b00, 0b10, -0b011),
    make_tuple(0b01, 0b10, -0b010),
    make_tuple(0b10, 0b10,  0b010),
    make_tuple(0b11, 0b10,  0b011),
    make_tuple(0b00, 0b11, -0b111),
    make_tuple(0b01, 0b11, -0b110),
    make_tuple(0b10, 0b11, -0b101),
    make_tuple(0b11, 0b11, -0b100),
    // clang-format on
};
INSTANTIATE_TEST_CASE_P(SignExtendTest, SignExtendTest,
                        ::testing::ValuesIn(signExtendData));
TEST_P(SignExtendTest, SignExtendTest) {
  ASSERT_EQ(AbstractHuffmanTable::signExtended(diff, len), value);
}

} // namespace rawspeed_test
