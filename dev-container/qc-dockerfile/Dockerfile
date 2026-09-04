# ------------------------------------------------------------------------------
# Copyright 2024 - 2026 Munich Quantum Software Stack
# All rights reserved.
#
# Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://llvm.org/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# ------------------------------------------------------------------------------

# Ubuntu 24.04 to match ../slurm-spank-plugins/mqss-spank-plugin/docker-testbed/Dockerfile
# (one base OS/toolchain family for both dev-container images). This is a
# distinct image from the Rocky Linux 9.3 one the pre-existing
# mqss_scheduler_dev container runs
# (~/Projects/mqss/containers/docker_images/mqss/Dockerfile) — the two
# coexist under different names (this one is qc-container) rather than one
# replacing the other. Bakes in grpc/protobuf/openmpi-devel, which in
# mqss_scheduler_dev were only ever installed ad hoc into the running
# container via `dnf install` and would have been lost on recreation.
#
# NOTE: LLVM at ~/Projects/mqss/dependencies/installed was compiled on Rocky
# Linux 9 — despite that, it's verified working here (compiled, linked, and
# ran a real test program against it inside a container from this exact
# image): glibc/libstdc++ are backward compatible in this direction (an
# older-distro-built binary running against a newer runtime), so no
# Ubuntu-native LLVM rebuild is needed. See dev-container/docker-compose.yml
# for how it's mounted read-only into qc-container at LLVM_ROOT's default
# path. QDMI/QInfo/submitter/scheduler under scheduler/build and
# scheduler/dependencies/installed (also Rocky-built) haven't specifically
# been tested the same way yet — likely fine by the same reasoning, but
# unconfirmed; examples/grpc-qcscheduler's Makefile also still has
# Rocky-specific absolute paths (e.g. /usr/lib64/openmpi/bin/mpicxx) that
# would need updating for Ubuntu (whose openmpi-bin package puts
# mpicxx/mpirun on PATH directly) regardless of ABI compatibility.

# Force x86_64 emulation (via QEMU) so this builds/runs on arm64 hosts like
# Apple Silicon Macs.
FROM --platform=linux/amd64 ubuntu:24.04

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ninja-build \
    cmake \
    git \
    curl \
    wget \
    vim \
    unzip \
    bash-completion \
    ca-certificates \
    sudo \
    openssh-client \
    pkg-config \
    bison \
    flex \
    python3 \
    python3-dev \
    python3-pip \
    libz3-dev \
    libboost-all-dev \
    libzip-dev \
    libopenmpi-dev \
    openmpi-bin \
    libprotobuf-dev \
    protobuf-compiler \
    libgrpc++-dev \
    libgrpc-dev \
    protobuf-compiler-grpc \
    && rm -rf /var/lib/apt/lists/*

# Create an admin user with passwordless sudo (mirrors the 'admin' user the
# original mqss_img used, so /home/admin/shared paths in existing scripts/docs
# keep working)
RUN useradd -m admin && \
    echo 'admin:admin' | chpasswd && \
    usermod -aG sudo admin && \
    echo 'admin ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Create the shared directory and give it to admin
RUN mkdir -p /home/admin/shared && \
    chown admin:admin /home/admin/shared

# 22: ssh (unused by compose, kept for parity with the original mqss_img).
# 50051: grpc-qcscheduler's gRPC listener port.
EXPOSE 22 50051

USER admin
WORKDIR /home/admin

# uv, for `uv run --script` PEP723 scripts - specifically
# dependencies/QDMI/examples/device/src/run_circuit.py, which the real (Aer)
# execution path in cxx_device.cpp shells out to. Installed to
# /home/admin/.local/bin, which Ubuntu's default admin .profile already adds
# to PATH for login shells.
RUN curl -LsSf https://astral.sh/uv/install.sh | sh

# Long-running placeholder command so `docker compose up -d` has something to
# stay alive on; actual work happens via `docker compose exec ... bash`, same
# as the other docker-testbed/ containers in this repo.
CMD ["sleep", "infinity"]
