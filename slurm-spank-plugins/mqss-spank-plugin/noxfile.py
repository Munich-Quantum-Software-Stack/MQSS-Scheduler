#!/usr/bin/env -S uv run --script --quiet
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

# /// script
# dependencies = ["nox"]
# ///

"""Nox sessions for MQSS Slurm SPANK plugin testing."""

from __future__ import annotations

import argparse
import os

import nox

nox.needs_version = ">=2026.04.10"
nox.options.default_venv_backend = "uv"


@nox.session(reuse_venv=True)
def smoke_tests(session: nox.Session) -> None:
    """Run the SPANK smoke scripts against an active Slurm deployment."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--partition",
        default="mqss",
        help="Slurm partition to target (default: mqss).",
    )
    parser.add_argument(
        "--hook-mode",
        choices=("local", "full"),
        default="local",
        help="Hook mode for test_hook_logs.sh (default: local).",
    )
    parser.add_argument(
        "--require-all-hooks",
        action="store_true",
        help="Require all hook callbacks when running in full mode.",
    )
    parser.add_argument(
        "--test-base-url",
        default=os.environ.get("MQSS_BASE_URL"),
        help="Base URL to validate with test_env_injection.sh. Defaults to MQSS_BASE_URL.",
    )
    parser.add_argument(
        "--test-tokens-file",
        default=os.environ.get("MQSS_TOKENS_FILE"),
        help="Optional tokens file path to validate with test_env_injection.sh.",
    )
    args, posargs = parser.parse_known_args(session.posargs)
    if posargs:
        joined_args = " ".join(posargs)
        session.error(f"Unexpected arguments for the smoke_tests session: {joined_args}")
    if not args.test_base_url:
        session.error("Pass --test-base-url or export MQSS_BASE_URL before running smoke_tests.")

    # Nox always runs from the noxfile's own directory, and test/ is a direct
    # child of it.
    hook_args = [
        "bash",
        "test/test_hook_logs.sh",
        "--partition",
        args.partition,
        "--hook-mode",
        args.hook_mode,
    ]
    if args.require_all_hooks:
        hook_args.append("--require-all-hooks")

    session.run(*hook_args, external=True)

    env_args = [
        "bash",
        "test/test_env_injection.sh",
        "--partition",
        args.partition,
        "--test-base-url",
        args.test_base_url,
    ]
    if args.test_tokens_file:
        env_args.extend(["--test-tokens-file", args.test_tokens_file])

    session.run(*env_args, external=True)

    license_args = [
        "bash",
        "test/test_license_alignment.sh",
        "--partition",
        args.partition,
    ]
    session.run(*license_args, external=True)
