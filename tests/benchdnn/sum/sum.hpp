/*******************************************************************************
* Copyright 2019-2020 Intel Corporation
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

#ifndef SUM_HPP
#define SUM_HPP

#include <iostream>

#include "dnnl.h"

#include "common.hpp"
#include "dnn_types.hpp"
#include "dnnl_common.hpp"
#include "dnnl_memory.hpp"
#include "perf_report.hpp"

namespace sum {

struct settings_t {
    settings_t() = default;

    // ctor to save certain fields from resetting
    settings_t(const char *perf_template) : settings_t() {
        this->perf_template = perf_template;
    }

    dims_t dims;

    std::vector<std::vector<dnnl_data_type_t>> sdt {{dnnl_f32, dnnl_f32}};
    std::vector<dnnl_data_type_t> ddt {dnnl_f32};
    std::vector<std::vector<std::string>> stag {{tag::abx}};
    std::vector<std::string> dtag {tag::undef};
    std::vector<std::vector<float>> scales {{0.25}, {1}, {4}};
    bool allow_unimpl = false;

    const char *perf_template_csv
            = "perf,%engine%,%sdt%,%ddt%,%stag%,%dtag%,%DESC%,%-time%,%0time%";
    const char *perf_template_def = "perf,%engine%,%prb%,%-time%,%0time%";
    const char *perf_template = perf_template_def;

    void reset() { *this = settings_t(perf_template); }
};

struct prb_t {
    prb_t(const dims_t &dims, const std::vector<dnnl_data_type_t> &sdt,
            dnnl_data_type_t ddt, const std::vector<std::string> &stag,
            const std::string &dtag, const std::vector<float> &scales)
        : dims(dims)
        , sdt(sdt)
        , ddt(ddt)
        , stag(stag)
        , dtag(dtag)
        , scales(sdt.size())
        , ndims((int)dims.size()) {
        // if there is a single scale then broadcast it
        for (int i_input = 0; i_input < n_inputs(); i_input++)
            this->scales[i_input]
                    = ((int)scales.size() == 1) ? scales[0] : scales[i_input];
    }
    ~prb_t() {}

    dims_t dims;
    std::vector<dnnl_data_type_t> sdt;
    dnnl_data_type_t ddt;
    std::vector<std::string> stag;
    std::string dtag;
    std::vector<float> scales;
    int ndims;

    int n_inputs() const { return (int)sdt.size(); }
};
std::ostream &operator<<(std::ostream &s, const prb_t &p);

struct perf_report_t : public base_perf_report_t {
    using base_perf_report_t::base_perf_report_t;

    void report(const prb_t *p, const res_t *r, const char *prb_str) {
        p_ = p;
        base_report(r, prb_str);
    }

    void dump_desc(std::ostream &s) const override { s << p_->dims; }

    void dump_desc_csv(std::ostream &s) const override { s << p_->dims; }

    const std::vector<dnnl_data_type_t> *sdt() const override {
        return &p_->sdt;
    }
    const dnnl_data_type_t *ddt() const override { return &p_->ddt; }
    const std::vector<std::string> *stag() const override { return &p_->stag; }
    const std::string *dtag() const override { return &p_->dtag; }

private:
    const prb_t *p_ = NULL;
};

void compute_ref(
        const prb_t *p, const std::vector<dnn_mem_t> &src, dnn_mem_t &dst);

int doit(const prb_t *p, res_t *res);
int bench(int argc, char **argv);

} // namespace sum

#endif
