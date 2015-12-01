/*
 * Copyright 2015 Georgia Institute of Technology
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

/**
 * @brief   Unit tests for ANSV
 */

#include <gtest/gtest.h>
#include <cxx-prettyprint/prettyprint.hpp>
#include <ansv.hpp>
#include <rmq.hpp>
#include <mxx/distribution.hpp>
#include <vector>
#include <algorithm>

// check ansv via a rmq
template <typename T>
void check_ansv(const std::vector<T>& in, const std::vector<size_t>& nsv, bool left) {
    // construct RMQ
    rmq<typename std::vector<T>::const_iterator> minquery(in.cbegin(), in.cend());

    // for each position check the nsv and direction
    for (size_t i = 0; i < in.size(); ++i) {
        if (nsv[i] == 0) {
            if (left && i > 0) {
                // expect this is an overall minimum of the range [0, i]
                T m = *minquery.query(in.cbegin(), in.cbegin()+i+1);
                EXPECT_TRUE(in[i] == m || in[0] == m) << " at i=" << i;
            } else if (!left && i+1 < in.size()) {
                T m = *minquery.query(in.cbegin()+i, in.cend());
                EXPECT_TRUE(in[i] == m) << " at i=" << i;
            }
        } else {
            size_t s = nsv[i];
            if (left) {
                EXPECT_LT(s, i);
                // no other element can be smaller than `in[i]` after `in[s]`
                if (s+1 < i) {
                    T m = *minquery.query(in.cbegin()+s+1, in.cbegin()+i);
                    // TODO:this accounts for the case that the "nearest" min
                    // is actually a further away equal value to the "nearest" min
                    EXPECT_TRUE(in[i] <= m || in[s] == m) << " for range [" << s+1 << "," << i << "]";
                }
                // element at `s` is smaller than `in[i]`
                EXPECT_LT(in[s], in[i]);
            } else {
                EXPECT_GT(s, i);
                // no other element can be smaller than `in[i]` before `in[s]`
                if (i < s-1) {
                    T m = *minquery.query(in.cbegin()+i, in.cbegin()+s-1);
                    EXPECT_TRUE(in[i] <= m || in[s] == m);
                }
                // element at `s` is smaller than in[i]
                EXPECT_LT(in[s], in[i]);
            }

        }
    }
}

TEST(PsacANSV, SeqANSVrand) {

    for (size_t n : {8, 137, 1000, 4200, 13790}) {
        std::vector<size_t> vec(n);
        std::srand(0);
        std::generate(vec.begin(), vec.end(), [](){return std::rand() % 997;});
        // calc ansv
        std::vector<size_t> left_nsv;
        std::vector<size_t> right_nsv;
        ansv_sequential(vec, left_nsv, right_nsv);

        check_ansv(vec, left_nsv, true);
        check_ansv(vec, right_nsv, false);
    }
}

TEST(PsacANSV, ParallelANSVrand) {
    mxx::comm c;

    for (size_t n : {13, 137, 1000, 66666, 137900}) {
        std::vector<size_t> in;
        if (c.rank() == 0) {
            in.resize(n);
            std::srand(7);
            std::generate(in.begin(), in.end(), [](){return std::rand() % 10000;});
        }
        std::vector<size_t> vec = mxx::stable_distribute(in, c);

        // calc ansv
        std::vector<size_t> left_nsv;
        std::vector<size_t> right_nsv;
        ansv(vec, left_nsv, right_nsv, c);

        left_nsv = mxx::gatherv(left_nsv, 0, c);
        right_nsv = mxx::gatherv(right_nsv, 0, c);

        if (c.rank() == 0) {
            check_ansv(in, left_nsv, true);
            check_ansv(in, right_nsv, false);
        }
    }
}