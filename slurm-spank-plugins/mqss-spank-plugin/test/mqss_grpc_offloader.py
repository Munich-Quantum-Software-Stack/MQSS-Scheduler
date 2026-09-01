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

"""Reusable, self-contained MQSS gRPC offload client — import this instead of
re-implementing the SubmitCircuit round trip in every quantum program.

    from mqss_grpc_offloader import mqss_grpc_offload

    counts = mqss_grpc_offload(qc, shots=1000)

target/qc_alias/job_id all default from the same environment variables
mqss-spank-plugin already injects into every Slurm job (MQSS_BASE_URL,
MQSS_QC_ALIAS, SLURM_JOB_ID) — pass them explicitly only to override (e.g.
for local testing outside Slurm).

This module is meant to be imported, not run standalone via
`uv run --script` — it has no PEP723 header of its own. Whatever script
you're actually invoking with `uv run --script` needs `grpcio` and
`protobuf` (and `qiskit` for QuantumCircuit) in *its own* dependencies list,
since uv only inspects the entry script, not its imports. See
bell_state_offload.py for a working example.

Self-contained: the protoc/grpc-plugin generated code for quantum_job.proto
(normally quantum_job_pb2.py + quantum_job_pb2_grpc.py, kept as separate
files as protoc's own convention) is folded in below instead, so this is the
only file that needs to travel with a user's quantum program. If
quantum_job.proto ever changes, regenerate the two files the normal way and
re-merge by hand:
    python3 -m grpc_tools.protoc -I. --python_out=. --grpc_python_out=. \
        quantum_job.proto
(or `protoc --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) ...` for the
C++ side used by examples/qrm-example/ — this file only needs the Python
one), then paste quantum_job_pb2.py's body in place of the "generated:
quantum_job_pb2.py" section below and quantum_job_pb2_grpc.py's body in
place of the other, dropping the now-redundant `import quantum_job_pb2`
(everything shares this module's namespace here, so
`quantum_job_pb2.CircuitRequest` etc. becomes plain `CircuitRequest`).
"""

from __future__ import annotations

import os
import urllib.parse

import grpc

# ------------------------------------------------------------------------------
# generated: quantum_job_pb2.py (protoc, from quantum_job.proto)
# ------------------------------------------------------------------------------
from google.protobuf import descriptor as _descriptor
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf import symbol_database as _symbol_database
from google.protobuf.internal import builder as _builder

_sym_db = _symbol_database.Default()

DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(
    b'\n\x11quantum_job.proto\x12\x04mqss\"j\n\x0e\x43ircuitRequest\x12\x0e\n'
    b'\x06job_id\x18\x01 \x01(\t\x12\x10\n\x08qc_alias\x18\x02 \x01(\t\x12'
    b'\x16\n\x0eprogram_format\x18\x03 \x01(\t\x12\x0f\n\x07program\x18\x04 '
    b'\x01(\t\x12\r\n\x05shots\x18\x05 \x01(\x05\"\xa7\x01\n\rCircuitResult'
    b'\x12\x0e\n\x06job_id\x18\x01 \x01(\t\x12\x0f\n\x07success\x18\x02 \x01'
    b'(\x08\x12\x15\n\rerror_message\x18\x03 \x01(\t\x12/\n\x06\x63ounts\x18'
    b'\x04 \x03(\x0b\x32\x1f.mqss.CircuitResult.CountsEntry\x1a-\n\x0b\x43'
    b'ountsEntry\x12\x0b\n\x03key\x18\x01 \x01(\t\x12\r\n\x05value\x18\x02 '
    b'\x01(\x05:\x02\x38\x01\x32O\n\x11QuantumJobService\x12:\n\rSubmitCircuit'
    b'\x12\x14.mqss.CircuitRequest\x1a\x13.mqss.CircuitResultb\x06proto3'
)

_globals = globals()
_builder.BuildMessageAndEnumDescriptors(DESCRIPTOR, _globals)
_builder.BuildTopDescriptorsAndMessages(DESCRIPTOR, 'mqss_grpc_offloader', _globals)
if not _descriptor._USE_C_DESCRIPTORS:
    DESCRIPTOR._loaded_options = None
    _globals['_CIRCUITRESULT_COUNTSENTRY']._loaded_options = None
    _globals['_CIRCUITRESULT_COUNTSENTRY']._serialized_options = b'8\001'
    _globals['_CIRCUITREQUEST']._serialized_start = 27
    _globals['_CIRCUITREQUEST']._serialized_end = 133
    _globals['_CIRCUITRESULT']._serialized_start = 136
    _globals['_CIRCUITRESULT']._serialized_end = 303
    _globals['_CIRCUITRESULT_COUNTSENTRY']._serialized_start = 258
    _globals['_CIRCUITRESULT_COUNTSENTRY']._serialized_end = 303
    _globals['_QUANTUMJOBSERVICE']._serialized_start = 305
    _globals['_QUANTUMJOBSERVICE']._serialized_end = 384
# CircuitRequest, CircuitResult are now real classes in this module's
# namespace, courtesy of the _builder calls above.

# ------------------------------------------------------------------------------
# generated: quantum_job_pb2_grpc.py (grpc's protoc plugin, from the same file)
# ------------------------------------------------------------------------------
GRPC_GENERATED_VERSION = '1.83.0'
GRPC_VERSION = grpc.__version__
_version_not_supported = False

try:
    from grpc._utilities import first_version_is_lower
    _version_not_supported = first_version_is_lower(GRPC_VERSION, GRPC_GENERATED_VERSION)
except ImportError:
    _version_not_supported = True

if _version_not_supported:
    raise RuntimeError(
        f'The grpc package installed is at version {GRPC_VERSION},'
        + ' but the generated code in mqss_grpc_offloader.py depends on'
        + f' grpcio>={GRPC_GENERATED_VERSION}.'
        + f' Please upgrade your grpc module to grpcio>={GRPC_GENERATED_VERSION}'
        + f' or downgrade your generated code using grpcio-tools<={GRPC_VERSION}.'
    )


class QuantumJobServiceStub:
    """Missing associated documentation comment in .proto file."""

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.SubmitCircuit = channel.unary_unary(
                '/mqss.QuantumJobService/SubmitCircuit',
                request_serializer=CircuitRequest.SerializeToString,
                response_deserializer=CircuitResult.FromString,
                _registered_method=True)


class QuantumJobServiceServicer:
    """Missing associated documentation comment in .proto file."""

    def SubmitCircuit(self, request, context):
        """Submit a single circuit and block until it has been executed.
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')


def add_QuantumJobServiceServicer_to_server(servicer, server):
    rpc_method_handlers = {
            'SubmitCircuit': grpc.unary_unary_rpc_method_handler(
                    servicer.SubmitCircuit,
                    request_deserializer=CircuitRequest.FromString,
                    response_serializer=CircuitResult.SerializeToString,
            ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
            'mqss.QuantumJobService', rpc_method_handlers)
    server.add_generic_rpc_handlers((generic_handler,))
    server.add_registered_method_handlers('mqss.QuantumJobService', rpc_method_handlers)


# This class is part of an EXPERIMENTAL API.
class QuantumJobService:
    """Missing associated documentation comment in .proto file."""

    @staticmethod
    def SubmitCircuit(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(
            request,
            target,
            '/mqss.QuantumJobService/SubmitCircuit',
            CircuitRequest.SerializeToString,
            CircuitResult.FromString,
            options,
            channel_credentials,
            insecure,
            call_credentials,
            compression,
            wait_for_ready,
            timeout,
            metadata,
            _registered_method=True)

# ------------------------------------------------------------------------------
# end generated code
# ------------------------------------------------------------------------------

from qiskit import QuantumCircuit, qasm3  # noqa: E402

__all__ = ["mqss_grpc_offload", "resolve_grpc_target"]


def resolve_grpc_target(base_url: str) -> str:
    """Derive a gRPC "host:port" target from an MQSS_BASE_URL-style URL."""
    parsed = urllib.parse.urlparse(base_url)
    if not parsed.netloc:
        msg = f"Cannot derive a gRPC target from MQSS_BASE_URL={base_url!r}"
        raise ValueError(msg)
    return parsed.netloc


def mqss_grpc_offload(
    qc: QuantumCircuit,
    shots: int = 1000,
    *,
    target: str | None = None,
    qc_alias: str | None = None,
    job_id: str | None = None,
    timeout: float = 30,
) -> dict[str, int]:
    """Serialize `qc` and offload it to a remote MQSS-Scheduler gRPC listener.

    Args:
        qc: The circuit to run.
        shots: Number of shots to request.
        target: gRPC "host:port". Defaults to the netloc of the
            MQSS_BASE_URL env var.
        qc_alias: Target QC alias. Defaults to the MQSS_QC_ALIAS env var,
            or "" if unset.
        job_id: Correlation id sent with the request. Defaults to the
            SLURM_JOB_ID env var, or "local" outside Slurm.
        timeout: Seconds to wait for the RPC to be *accepted* (not for the
            circuit to finish running — see the return value note below).

    Returns:
        Measurement outcome -> count. Today's MQSS-Scheduler listeners are
        fire-and-forget (see examples/qrm-example/Readme.md's Known gaps):
        SubmitCircuit returns as soon as the task is queued, not after it
        actually runs — an EMPTY dict here means "accepted", not "ran with
        zero shots". There is no results round trip yet, so check for this
        explicitly (`if not counts: ...`) rather than assuming a result is
        always populated.

    Raises:
        ValueError: `target` wasn't given and MQSS_BASE_URL isn't set (or
            isn't a resolvable host:port).
        RuntimeError: the listener rejected the circuit
            (response.success is False) — see the message for
            response.error_message.
        grpc.RpcError: the RPC itself failed (e.g. UNAVAILABLE if the
            target host is unreachable, DEADLINE_EXCEEDED if it doesn't
            respond within `timeout`).
    """
    if target is None:
        base_url = os.environ.get("MQSS_BASE_URL")
        if not base_url:
            msg = "target was not given and MQSS_BASE_URL is not set"
            raise ValueError(msg)
        target = resolve_grpc_target(base_url)
    if qc_alias is None:
        qc_alias = os.environ.get("MQSS_QC_ALIAS", "")
    if job_id is None:
        job_id = os.environ.get("SLURM_JOB_ID", "local")

    program = qasm3.dumps(qc)

    with grpc.insecure_channel(target) as channel:
        stub = QuantumJobServiceStub(channel)
        request = CircuitRequest(
            job_id=job_id,
            qc_alias=qc_alias,
            program_format="qasm3",
            program=program,
            shots=shots,
        )
        response = stub.SubmitCircuit(request, timeout=timeout)

    if not response.success:
        msg = f"Remote execution failed: {response.error_message}"
        raise RuntimeError(msg)
    return dict(response.counts)
