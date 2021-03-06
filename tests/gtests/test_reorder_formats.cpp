/*******************************************************************************
* Copyright 2020 Intel Corporation
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
*******************************************************************************/

#include <memory>
#include <numeric>
#include <utility>
#include <type_traits>

#include "dnnl_test_common.hpp"
#include "gtest/gtest.h"

#include "dnnl.hpp"
#include "dnnl_debug.h"

namespace dnnl {

using dt = memory::data_type;
using tag = memory::format_tag;
using md = memory::desc;

class reorder_formats_test : public ::testing::Test {
public:
    engine e;

protected:
    virtual void SetUp() { e = get_test_engine(); }
};

TEST_F(reorder_formats_test, TestChecksAllFormats) {
    static auto isa = get_effective_cpu_isa();
    bool has_bf16 = isa >= cpu_isa::avx512_core;

    bool is_cpu = get_test_engine_kind() == engine::kind::cpu;

    memory::dims SP1D = {2};
    memory::dims SP2D = {2, 2};
    memory::dims SP3D = {2, 2, 2};
    memory::dims SP4D = {2, 2, 2, 2};
    memory::dims SP5D = {2, 2, 2, 2, 2};
    memory::dims SP6D = {2, 2, 2, 2, 2, 2};
    std::vector<memory::dims> v_dims = {SP1D, SP2D, SP3D, SP4D, SP5D, SP6D};

    // first one is f16 which is not supported on cpu
    unsigned start_dt = 1 + is_cpu;
    unsigned end_dt = 7;

    unsigned start_tag = static_cast<unsigned>(tag::any) + 1;
    unsigned end_tag = static_cast<unsigned>(tag::format_tag_last);

    dt in_dt, out_dt;
    tag in_tag, out_tag;
    md in_md, out_md;

    auto flag_comp = dnnl_memory_extra_flag_compensation_conv_s8s8;
    dnnl_memory_extra_desc_t none {}, conv_s8s8 {}, gconv_s8s8 {};
    gconv_s8s8.flags = conv_s8s8.flags = flag_comp;
    conv_s8s8.compensation_mask = (1 << 0);
    gconv_s8s8.compensation_mask = (1 << 0) + (1 << 1);

    std::vector<dnnl_memory_extra_desc_t> extra {none, conv_s8s8, gconv_s8s8};

    for (unsigned i_dt = start_dt; i_dt < end_dt; i_dt++) {
        in_dt = static_cast<dt>(i_dt);
        if (in_dt == dt::bf16 && !has_bf16) continue;

        for (unsigned i_tag = start_tag; i_tag < end_tag; i_tag++) {
            in_tag = static_cast<tag>(i_tag);
            for (const auto &i_dims : v_dims) {
                in_md = md(i_dims, in_dt, in_tag, true);
                if (in_md) break;
            }
            ASSERT_TRUE(in_md);

            // check only plain -> anything reorders, otherwise test takes too
            // long, and no much sense in testing blocked -> blocked.
            if (in_md.data.format_desc.blocking.inner_nblks != 0) continue;

            for (unsigned o_dt = start_dt; o_dt < end_dt; o_dt++) {
                out_dt = static_cast<dt>(o_dt);
                if (out_dt == dt::bf16 && !has_bf16) continue;

                for_(unsigned o_tag = start_tag; o_tag < end_tag; o_tag++)
                for (const auto &i_extra : extra) {
                    out_tag = static_cast<tag>(o_tag);
                    for (const auto &i_dims : v_dims) {
                        out_md = md(i_dims, out_dt, out_tag, true);
                        if (out_md) break;
                    }
                    ASSERT_TRUE(out_md);
                    if (in_md.data.ndims != out_md.data.ndims) continue;

                    out_md.data.extra = i_extra;

                    auto src = memory(in_md, e);
                    auto dst = memory(out_md, e);
                    reorder::primitive_desc r_pd(
                            e, in_md, e, out_md, primitive_attr(), true);
                    if (r_pd) {
                        auto r = reorder(r_pd);
                        auto strm = make_stream(r_pd.get_engine());
                        r.execute(strm, src, dst);
                        strm.wait();
                    }
                }
            }
        }
    }
}

} // namespace dnnl
