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
#pragma once

#include "qdmi/client.h"
#include "qdmi_lib.hpp"

#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Template function to call QDMI functions and throw exceptions on error
template <typename Func, typename... Args>
void callFuncAndThrowOnError(Func func, const char *error_message, Args &&...args) {
    int status_code = func(std::forward<Args>(args)...);
    if (status_code != QDMI_SUCCESS) {
        switch (status_code) {
        case QDMI_WARN_GENERAL:
            std::cout << std::string(error_message) + " : " + std::string("WARN_GENERAL")
                      << std::endl;
            break;
        case QDMI_ERROR_FATAL:
            throw std::runtime_error(std::string(error_message) + " : " + std::string("FATAL"));
        case QDMI_ERROR_OUTOFMEM:
            throw std::runtime_error(std::string(error_message) + " : " + std::string("OUTOFMEM"));
        case QDMI_ERROR_NOTIMPLEMENTED:
            throw std::runtime_error(std::string(error_message) + " : " +
                                     std::string("NOTIMPLEMENTED"));
        case QDMI_ERROR_LIBNOTFOUND:
            throw std::runtime_error(std::string(error_message) + " : " +
                                     std::string("LIBNOTFOUND"));
        case QDMI_ERROR_NOTFOUND:
            throw std::runtime_error(std::string(error_message) + " : " + std::string("NOTFOUND"));
        case QDMI_ERROR_OUTOFRANGE:
            throw std::runtime_error(std::string(error_message) + " : " +
                                     std::string("OUTOFRANGE"));
        case QDMI_ERROR_INVALIDARGUMENT:
            throw std::runtime_error(std::string(error_message) + " : " +
                                     std::string("INVALIDARGUMENT"));
        case QDMI_ERROR_PERMISSIONDENIED:
            throw std::runtime_error(std::string(error_message) + " : " +
                                     std::string("PERMISSIONDENIED"));
        case QDMI_ERROR_NOTSUPPORTED:
            throw std::runtime_error(std::string(error_message) + " : " +
                                     std::string("NOTSUPPORTED"));
        case QDMI_ERROR_BADSTATE:
            throw std::runtime_error(std::string(error_message) + " : " + std::string("BADSTATE"));
        default:
            throw std::runtime_error(std::string(error_message) + " : " +
                                     std::to_string(status_code));
        }
    }
}

namespace qdmi {

class Site {
public:
    explicit Site(QDMI_Device device, QDMI_Site site) : device(device), site(site) {}
    Site(const Site &other) = default;
    Site(Site &&other) noexcept : device(other.device), site(other.site) {
        other.device = nullptr;
        other.site = nullptr;
    }
    Site &operator=(const Site &other) = default;
    Site &operator=(Site &&other) noexcept {
        if (this != &other) {
            device = other.device;
            site = other.site;
            other.device = nullptr;
            other.site = nullptr;
        }
        return *this;
    }

    ~Site() {
        device = nullptr;
        site = nullptr;
    };

    friend class Operation;

    [[nodiscard]] size_t get_id() const;
    [[nodiscard]] double get_t1() const;
    [[nodiscard]] double get_t2() const;

private:
    QDMI_Device device;
    QDMI_Site site;
};

class Operation {
public:
    explicit Operation(QDMI_Device device, QDMI_Operation operation)
        : device(device), operation(operation) {}
    Operation(const Operation &other) = default;
    Operation(Operation &&other) noexcept : device(other.device), operation(other.operation) {
        other.device = nullptr;
        other.operation = nullptr;
    }
    Operation &operator=(const Operation &other) = default;
    Operation &operator=(Operation &&other) noexcept {
        if (this != &other) {
            device = other.device;
            operation = other.operation;
            other.device = nullptr;
            other.operation = nullptr;
        }
        return *this;
    }
    ~Operation() {
        device = nullptr;
        operation = nullptr;
    }

    [[nodiscard]] std::string get_name() const;
    [[nodiscard]] size_t get_num_qubits() const;
    [[nodiscard]] size_t get_num_parameters() const;
    [[nodiscard]] double get_duration(const std::vector<std::reference_wrapper<Site>> &sites = {},
                                      const std::vector<double> &parameters = {}) const;
    [[nodiscard]] double get_fidelity(const std::vector<std::reference_wrapper<Site>> &sites = {},
                                      const std::vector<double> &parameters = {}) const;

private:
    QDMI_Device device;
    QDMI_Operation operation;
};

using JobResultVariant = std::variant<std::vector<std::string>, std::map<std::string, size_t>,
                                      std::vector<std::pair<double, double>>,
                                      std::map<std::string, std::pair<double, double>>,
                                      std::vector<double>, std::map<std::string, double>>;

class Job {

public:
    Job(QDMI_Job job) : job(job) {}
    Job(const Job &other) = delete;
    Job &operator=(const Job &other) = delete;
    Job(Job &&other) noexcept;
    Job &operator=(Job &&other) = delete;
    ~Job();

    void submit();
    void cancel();
    QDMI_Job_Status status();
    void wait();
    std::optional<JobResultVariant> results(QDMI_Job_Result result);

private:
    QDMI_Job job;

    template <typename T> auto get_results(QDMI_Job_Result result_type);

    static std::vector<std::string> splitString(const std::string &str,
                                                const std::string &delimiter);
};

class Device {
public:
    explicit Device(QDMI_Device device) : device(device) {}
    Device(const Device &other) = default;
    Device(Device &&other) noexcept : device(other.device) { other.device = nullptr; }
    Device &operator=(const Device &other) = default;
    Device &operator=(Device &&other) noexcept {
        if (this != &other) {
            device = other.device;
            other.device = nullptr;
        }
        return *this;
    }
    ~Device() { device = nullptr; };

    [[nodiscard]] std::string get_name() const;
    [[nodiscard]] std::string get_version() const;
    [[nodiscard]] size_t get_status() const;
    [[nodiscard]] std::string get_library_version() const;
    [[nodiscard]] size_t get_num_qubits() const;
    [[nodiscard]] std::vector<Site> get_sites() const;
    [[nodiscard]] std::vector<Operation> get_operations() const;
    [[nodiscard]] std::vector<std::pair<Site, Site>> get_coupling_map() const;
    [[nodiscard]] bool needs_calibration() const;

    [[nodiscard]] Job create_job(QDMI_Program_Format format, const std::string &prog,
                                 const size_t &num_shots = 1024) const;

private:
    QDMI_Device device;

    template <typename T>
    auto query_property(QDMI_Device_Property property, const char *error_message) const;
};

class Session {
public:
    explicit Session(const std::string &driver_name = "", const std::string &token = "",
                     const std::string &username = "", const std::string &projectid = "");
    Session() = delete;
    Session(const Session &other) = delete;
    Session &operator=(const Session &other) = delete;
    Session(Session &&other) noexcept;
    Session &operator=(Session &&other) = delete;
    ~Session();

    std::vector<Device> get_devices();

private:
    std::string token;
    std::string username;
    std::string projectid;
    QDMIDriverLibrary qdmi_driver_lib;

    QDMI_Session session;
    std::vector<QDMI_Device> qdmi_devices;

    void set_token(const std::string &token);
    void set_username(const std::string &username);
    void set_projectid(const std::string &projectid);
};
} // namespace qdmi
