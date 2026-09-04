/*
 * Copyright 2024 - 2026 Munich Quantum Software Stack
 * All rights reserved.
 *
 * Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "qdmi.hpp"

#include <cstddef>
#include <regex>
#include <utility>
#include <vector>

qdmi::Job::Job(qdmi::Job &&other) noexcept : job(other.job) { other.job = nullptr; }

qdmi::Job::~Job() {
    if (job != nullptr) {
        QDMI_job_free_ptr(job);
        job = nullptr;
    }
}

void qdmi::Job::submit() {
    callFuncAndThrowOnError(QDMI_job_submit_ptr, "Failed to submit the job.", job);
}

void qdmi::Job::cancel() {
    callFuncAndThrowOnError(QDMI_job_cancel_ptr, "Failed to cancel the job.", job);
}

QDMI_Job_Status qdmi::Job::status() {

    QDMI_Job_Status status;
    callFuncAndThrowOnError(QDMI_job_check_ptr, "Failed to check the job status.", job, &status);
    return status;
}

void qdmi::Job::wait(size_t timeout) {
    callFuncAndThrowOnError(QDMI_job_wait_ptr, "Failed to wait for the job.", job, timeout);
}

template <> auto qdmi::Job::get_results<char>(QDMI_Job_Result result_type) {
    size_t size_ret{};

    callFuncAndThrowOnError(QDMI_job_get_results_ptr, "Failed to get the job results", job,
                            result_type, 0, nullptr, &size_ret);

    std::vector<char> value(size_ret);
    callFuncAndThrowOnError(QDMI_job_get_results_ptr, "Failed to get the job results", job,
                            result_type, size_ret, value.data(), nullptr);

    std::string values(static_cast<char *>(value.data()), size_ret - 1);
    auto str_vec = splitString(values, ",");

    return str_vec;
}

template <typename T> auto qdmi::Job::get_results(QDMI_Job_Result result_type) {
    size_t size_ret{};

    callFuncAndThrowOnError(QDMI_job_get_results_ptr, "Failed to get the job results", job,
                            result_type, 0, nullptr, &size_ret);

    std::vector<T> value(size_ret / sizeof(T));
    callFuncAndThrowOnError(QDMI_job_get_results_ptr, "Failed to get the job results", job,
                            result_type, size_ret, value.data(), nullptr);

    return value;
}

std::optional<qdmi::JobResultVariant> qdmi::Job::results(QDMI_Job_Result result_type) {

    wait();

    size_t size_ret{};
    int status_code = QDMI_job_get_results_ptr(job, result_type, 0, nullptr, &size_ret);
    if (status_code == QDMI_ERROR_NOTSUPPORTED) {
        return std::nullopt;
    }

    switch (result_type) {
    case QDMI_JOB_RESULT_SHOTS: {
        std::vector<std::string> shots_list;
        auto shots_v = get_results<char>(QDMI_JOB_RESULT_SHOTS);
        shots_list.reserve(shots_v.size());
        for (auto &shot : shots_v) {
            shots_list.push_back(shot);
        }
        return shots_list;
    }
    case QDMI_JOB_RESULT_HIST_KEYS: {
        std::vector<std::string> hist_keys;
        auto keys_v = get_results<char>(QDMI_JOB_RESULT_HIST_KEYS);
        hist_keys.reserve(keys_v.size());
        for (auto &key : keys_v) {
            hist_keys.push_back(key);
        }
        std::map<std::string, size_t> hist;
        auto values_v = get_results<size_t>(QDMI_JOB_RESULT_HIST_VALUES);
        for (size_t i = 0; i < values_v.size(); i++) {
            hist[hist_keys[i]] = values_v[i];
        }
        return hist;
    }
    case QDMI_JOB_RESULT_STATEVECTOR_DENSE: {
        std::vector<std::pair<double, double>> statevector_list;
        auto values_v = get_results<double>(QDMI_JOB_RESULT_STATEVECTOR_DENSE);
        statevector_list.reserve(values_v.size() / 2);
        for (size_t i = 0; i < values_v.size(); i += 2) {
            statevector_list.emplace_back(values_v[i], values_v[i + 1]);
        }
        return statevector_list;
    }
    case QDMI_JOB_RESULT_STATEVECTOR_SPARSE_KEYS: {
        std::vector<std::string> statevector_keys;
        auto keys_v = get_results<char>(QDMI_JOB_RESULT_STATEVECTOR_SPARSE_KEYS);
        statevector_keys.reserve(keys_v.size());
        for (auto &key : keys_v) {
            statevector_keys.push_back(key);
        }
        std::map<std::string, std::pair<double, double>> statevector_dict;

        auto values_v = get_results<double>(QDMI_JOB_RESULT_STATEVECTOR_SPARSE_VALUES);
        for (size_t i = 0, k = 0; i < values_v.size(); i += 2, k++) {
            statevector_dict[statevector_keys[k]] = std::make_pair(values_v[i], values_v[i + 1]);
        }
        return statevector_dict;
    }
    case QDMI_JOB_RESULT_PROBABILITIES_DENSE: {
        std::vector<double> probabilities_list;
        auto values_v = get_results<double>(QDMI_JOB_RESULT_PROBABILITIES_DENSE);
        probabilities_list.reserve(values_v.size());
        for (size_t i = 0; i < values_v.size(); i++) {
            probabilities_list.push_back(values_v[i]);
        }
        return probabilities_list;
    }
    case QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS: {
        std::vector<std::string> probabilities_keys;
        auto keys_v = get_results<char>(QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS);
        probabilities_keys.reserve(keys_v.size());
        for (auto &key : keys_v) {
            probabilities_keys.push_back(key);
        }
        std::map<std::string, double> probabilities_dict;
        auto values_v = get_results<double>(QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES);
        for (size_t i = 0; i < values_v.size(); i++) {
            probabilities_dict[probabilities_keys[i]] = values_v[i];
        }
        return probabilities_dict;
    }
    default:
        throw std::runtime_error("Unknown result type");
        break;
    }
    return std::nullopt;
}

std::vector<std::string> qdmi::Job::splitString(const std::string &str,
                                                const std::string &delimiter) {
    std::regex regex(delimiter);
    std::sregex_token_iterator it(str.begin(), str.end(), regex, -1);
    std::sregex_token_iterator end;

    return {it, end};
}
