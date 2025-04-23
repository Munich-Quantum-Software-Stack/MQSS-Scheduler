#include "qdmi/client.h"
#include <bits/types/struct_timeval.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <jansson.h>
#include <string.h>

// int QDMI_query_device_property_c(QDMI_Device dev, QDMI_Device_property prop, char** value)
// {
//     //printf("    [Backend]..............Querying device name\n");
//     *value = strdup("test_backend_1");
//     return QDMI_SUCCESS;
// }

// int QDMI_backend_init(QInfo info)
// {
//     printf("    [Backend]..............Backend init\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_control_readout_size(QDMI_Device dev, QDMI_Status *status, QDMI_Job job,
//                               int *numbits)
// {
//     printf("    [Backend]..............Control readout size\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_device_status(QDMI_Device dev, QInfo info, int *status)
// {
//     printf("    [Backend]..............Device status\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_control_pack_qir(QDMI_Device dev, void *qirmod, QDMI_Fragment *frag)
// {
//     //printf("    [Backend]..............Control pack qir\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_device_property_i(QDMI_Device dev, QDMI_Device_property prop, int *value)
// {
//     printf("    [Backend]..............Querying device property\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_control_cancel(QDMI_Device dev, QDMI_Job *job, QInfo info)
// {
//     printf("    [Backend]..............Control cancel\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_control_pause(QDMI_Device dev, QDMI_Job *job, QInfo info)
// {
//     printf("    [Backend]..............Control pause\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_control_readout_hist_top(QDMI_Device dev, QDMI_Status *status, QDMI_Job job,
//                                   int numhist, QInfo info, long *hist)
// {
//     printf("    [Backend]..............Control readout hist top\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_device_property_type(QDMI_Device dev, QDMI_Device_property prop)
// {
//     printf("    [Backend]..............Querying device property type\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_gate_unitary(QDMI_Device dev, QDMI_Gate gate,
//                             QDMI_Unitary *unitary)
// {
//     printf("    [Backend]..............Querying gate unitary\n");
//     return QDMI_SUCCESS;
// }
// int QDMI_query_gate_property_i(QDMI_Device dev, QDMI_Gate_property prop, QDMI_Gate gate, int *coor, int *value)
// {
//     printf("    [Backend]..............Querying gate property i\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_gate_property_f(QDMI_Device dev, QDMI_Gate_property prop, QDMI_Gate gate, int *coor, float *value)
// {
//     printf("    [Backend]..............Querying gate property f\n");
//     return QDMI_SUCCESS;
// }
// int QDMI_query_qubit_property_type(QDMI_Device dev, QDMI_Qubit qubit,
//                                    QDMI_Qubit_property prop)
// {
//     printf("    [Backend]..............Querying qubit property type\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_device_quality_check(QDMI_Device dev, double *result)
// {
//     printf("    [Backend]..............Device quality check\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_device_quality_limit(QDMI_Device dev, double *result)
// {
//     printf("    [Backend]..............Device quality limit\n");
//     return QDMI_SUCCESS;
// }
// int QDMI_device_quality_calibrate(QDMI_Device dev)
// {
//     printf("    [Backend]..............Device quality calibrate\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_gate_property_d(QDMI_Device dev, QDMI_Gate_property prop, QDMI_Gate gate, int *coor, double *value)
// {
//     printf("    [Backend]..............Querying gate property d\n");
//     return QDMI_SUCCESS;
// }


// int QDMI_query_gate_property_type(QDMI_Device dev, QDMI_Gate gate, QDMI_Gate_property prop)
// {
//     printf("    [Backend]..............Querying gate property type\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_gate_property_exists(QDMI_Device dev, QDMI_Gate gate, QDMI_Gate_property prop, int *exists)
// {
//     printf("    [Backend]..............Querying gate property exists\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_gate_size(QDMI_Device dev, QDMI_Gate gate, int *size)
// {
//     printf("    [Backend]..............Querying gate size\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_control_readout_raw_sample(QDMI_Device dev, QDMI_Status *status, int numraw, QInfo info, long *hist)
// {
//     printf("    [Backend]..............Control readout raw sample\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_control_extract_state(QDMI_Device dev, QDMI_Status status, int *state)
// {
//     printf("    [Backend]..............Control extract state\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_control_pack_qasm2(QDMI_Device dev, char *qasmstr, QDMI_Fragment *frag)
// {
//     printf("    [Backend]..............Control pack qasm2\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_control_test(QDMI_Device dev, QDMI_Job *job, int *flag, QDMI_Status *status)
// {
//     printf("    [Backend]..............Control test\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_control_wait(QDMI_Device dev, QDMI_Job *job, QDMI_Status *status)
// {
//     //printf("    [Backend]..............Control wait\n");
//     sleep(1);
//     return QDMI_SUCCESS;
// }

// int QDMI_control_submit(QDMI_Device dev, QDMI_Fragment *frag, int numshots, QInfo info, QDMI_Job *job){
//     //printf("    [Backend]..............Control submit\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_control_readout_raw_num(QDMI_Device dev, QDMI_Status *status, 
//                                 QDMI_Job job, int *num)
// {
//     printf("    [Backend]..............Control readout raw num\n");

//     *num = 1;
//     return QDMI_SUCCESS;
// }

// int QDMI_control_readout_hist_size(QDMI_Device dev, QDMI_Status *status, QDMI_Job job,
//                                    int *size) {
//     printf("    [Backend]..............Control readout hist size\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_device_property_exists(QDMI_Device dev, QDMI_Device_property prop, int *exists) {
//     printf("    [Backend]..............Query device property exists\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_device_property_f(QDMI_Device dev, QDMI_Device_property prop, float *value) {
//     printf("    [Backend]..............Query device property f\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_device_property_d(QDMI_Device dev, QDMI_Device_property prop, double *value) {
//     printf("    [Backend]..............Query device property d\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_gateset_num(QDMI_Device dev, int *num) {
//     printf("    [Backend]..............Query gateset num\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_qubits_num(QDMI_Device dev, int *num) {
//     printf("    [Backend]..............Query qubits num\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_populate_gateset(int num_gates, json_t *basis_gates)
// {
//     printf("    [Backend]..............Populating gateset\n");
//     return QDMI_SUCCESS;
// }


// void QDMI_get_gate_info(QDMI_Device dev, int gate_index, QDMI_Gate gate)
// {
//     printf("    [Backend]..............Getting gate info\n");
//     return;
// }

// int QDMI_query_all_gates(QDMI_Device dev, QDMI_Gate *gates)
// {
//     printf("    [Backend]..............Querying all gates\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_byname(QDMI_Device dev, char *name, QDMI_Gate *gate)
// {
//     printf("    [Backend]..............Querying by name\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_gate_name(QDMI_Device dev, QDMI_Gate gate, char* name, int* len)
// {
//     printf("    [Backend]..............Querying gate name\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_set_coupling_mapping(QDMI_Device dev, int qubit_index, QDMI_Qubit qubit)
// {
//     printf("    [Backend]..............Setting coupling mapping\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_set_qubit_properties(QDMI_Device dev, QDMI_Qubit qubit)
// {
//     printf("    [Backend]..............Setting qubit properties\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_all_qubits(QDMI_Device dev, QDMI_Qubit *qubits)
// {
//     printf("    [Backend]..............Querying all qubits\n");
//     return QDMI_SUCCESS;
// }


// int QDMI_query_qubit_property_exists(QDMI_Device dev, QDMI_Qubit qubit, QDMI_Qubit_property prop, int* scope)
// {
//     printf("    [Backend]..............Querying qubit property exists\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_qubit_property_c(QDMI_Device dev, QDMI_Qubit qubit, QDMI_Qubit_property prop, char *value)
// {
//     printf("    [Backend]..............Querying qubit property c\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_qubit_property_i(QDMI_Device dev, QDMI_Qubit qubit, QDMI_Qubit_property prop, int *value)
// {
//     printf("    [Backend]..............Querying qubit property i\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_qubit_property_f(QDMI_Device dev, QDMI_Qubit qubit, QDMI_Qubit_property prop, float *value)
// {
//     printf("    [Backend]..............Querying qubit property f\n");
//     return QDMI_SUCCESS;
// }

// int QDMI_query_qubit_property_d(QDMI_Device dev, QDMI_Qubit qubit, QDMI_Qubit_property prop, double *value)
// {
//     printf("    [Backend]..............Querying qubit property d\n");
//     return QDMI_SUCCESS;
// }
