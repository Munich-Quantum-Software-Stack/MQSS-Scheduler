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

# Pinned to a named stage (rather than a bare COPY --from=<image> below) so its
# platform can be forced explicitly — otherwise it resolves to the build host's
# native arch (e.g. arm64 on Apple Silicon) regardless of the main stage's
# platform, and those binaries then run unemulated and misdetect the platform
# for anything they download at runtime.
FROM --platform=linux/amd64 ghcr.io/astral-sh/uv:latest AS uv-source

# Force x86_64 emulation (via QEMU) so this builds/runs on arm64 hosts like Apple Silicon Macs
FROM --platform=linux/amd64 ubuntu:24.04

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    curl \
    ca-certificates \
    libssl-dev \
    libcurl4-openssl-dev \
    libslurm-dev \
    slurm-wlm \
    munge \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# Install uv and uvx from the official image (see uv-source stage above)
COPY --from=uv-source /uv /uvx /usr/local/bin/

# Create a non-root user 'testuser' with UID 1000 and passwordless sudo rights
RUN userdel -r ubuntu || true
RUN useradd -m -u 1000 -s /bin/bash testuser && \
    echo "testuser ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Create Slurm state, spool, and log directories/files with appropriate ownership/permissions
RUN mkdir -p /var/spool/slurmctld /var/spool/slurmd && \
    chown slurm:slurm /var/spool/slurmctld && \
    chown root:root /var/spool/slurmd && \
    chmod 755 /var/spool/slurmctld /var/spool/slurmd && \
    touch /var/log/slurmctld.log /var/log/slurmd.log && \
    chown slurm:slurm /var/log/slurmctld.log && \
    chown root:root /var/log/slurmd.log

# Configure Slurm with a static localhost setup (Gres=qpu:... requires
# select/cons_tres — select/linear does not support Gres scheduling at all)
COPY slurm-config/slurm.conf /etc/slurm/slurm.conf
COPY slurm-config/cgroup.conf /etc/slurm/cgroup.conf
COPY slurm-config/gres.conf /etc/slurm/gres.conf

# Set up plugstack configuration directories
RUN mkdir -p /etc/slurm/plugstack.conf.d && \
    echo "include /etc/slurm/plugstack.conf.d/*.conf" > /etc/slurm/plugstack.conf

# Create workspace directory and set ownership
RUN mkdir -p /workspace && chown -R testuser:testuser /workspace

WORKDIR /workspace

# Copy the workspace into the container (used when building the self-contained image)
COPY --chown=testuser:testuser . /workspace

# Build, install, and globally activate the SPANK plugin at image build time
# Also derive the Slurm multiarch library directory dynamically
RUN SLURM_LIB_DIR=$(dpkg-architecture -qDEB_HOST_MULTIARCH | xargs -I{} echo /usr/lib/{}/slurm-wlm) && \
    cmake -S /workspace -B /workspace/build-spank-docker -DCMAKE_BUILD_TYPE=Release -DBUILD_MQSS_SPANK=ON \
    -DMQSS_SPANK_INSTALL_DIR="$SLURM_LIB_DIR" -DMQSS_SPANK_SLURM_CONF_DIR=/etc/slurm && \
    cmake --build /workspace/build-spank-docker --target mqss-spank-plugin && \
    cmake --install /workspace/build-spank-docker --component mqss-spank-plugin && \
    rm -rf /workspace/build-spank-docker && \
    echo "required $SLURM_LIB_DIR/mqss-spank-plugin.so mqss_base_url=http://mock-mqss.invalid mqss_license_prefix=mqss_" > /etc/slurm/plugstack.conf.d/mqss-spank.conf

# Create a sentinel file to detect bind-mount shadowing
RUN touch /workspace/.built-in-image

# Change user to testuser
USER testuser

# Set entrypoint to run the services and command
ENTRYPOINT ["/workspace/test/entrypoint.sh"]
CMD ["/workspace/test/run_docker_test.sh"]
