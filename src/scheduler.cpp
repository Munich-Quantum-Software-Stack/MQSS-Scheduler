/*------------------------------------------------------------------------------
Copyright 2024 - 2026 Munich Quantum Software Stack
All rights reserved.

Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://llvm.org/LICENSE.txt

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/

#include <scheduler/scheduler.hpp>
#include <scheduler/quantum_task.hpp>

// Scheduler's methods are defined header-side (scheduler.tpp). As we can keep
// instantiable for whatever TaskType a project uses (see
// scheduler.tpp's header comment). This can be used to explicitly 
// instantiate the Scheduler for a specific TaskType.
template class mqss::Scheduler<mqss::QuantumTask>;
