
#include "qdmi.h"
#include <bits/types/struct_timeval.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>



int QDMI_backend_init(QInfo info)
{
    printf("    [Backend]..............Backend init\n");

    char *uri = NULL;
    void *regpointer = NULL;
    int err;

    //err = QDMI_core_register_belib(uri, regpointer);

    return QDMI_SUCCESS;
}

int QDMI_control_readout_size(QDMI_Device dev, QDMI_Status *status, int *numbits)
{
    printf("    [Backend]..............Control readout size\n");

    *numbits = 1;
    return QDMI_SUCCESS;
}

int QDMI_device_status(QDMI_Device dev, QInfo info, int *status)
{
    printf("    [Backend]..............Device status\n");

    *status = 1;
    return QDMI_SUCCESS;
}

int QDMI_control_pack_qir(QDMI_Device dev, void *qirmod, QDMI_Fragment *frag)
{
    printf("    [Backend]..............Control pack qir\n");

    if(!qirmod) return -2;
    if(!frag) return -3;

    //(*frag) = malloc(sizeof(QDMI_Fragment_t));
    //(*frag)->qirmod = qirmod;

    return QDMI_SUCCESS;
}

int QDMI_query_device_property_i(QDMI_Device dev, QDMI_Device_property prop, int *value)
{
    printf("    [Backend]..............Querying device property\n");

    *value = 1;
    return QDMI_SUCCESS;
}


int QDMI_control_pack_qasm2(QDMI_Device dev, char *qasmstr, QDMI_Fragment *frag)
{
    printf("    [Backend]..............Control pack qasm2\n");

    if(!frag) return 1;

    //(*frag)->qasmstr = malloc(sizeof(char) * strlen(qasmstr));
    //strcpy((*frag)->qasmstr, qasmstr);

    return QDMI_SUCCESS;
}

int QDMI_control_test(QDMI_Device dev, QDMI_Job *job, int *flag, QDMI_Status *status)
{
    printf("    [Backend]..............Control test\n");

    int random_value = rand() % 4;

    *flag = random_value;
    *status = random_value;

    return QDMI_SUCCESS;
}


int QDMI_control_wait(QDMI_Device dev, QDMI_Job *job, QDMI_Status *status)
{
    printf("    [Backend]..............Control wait\n");

    sleep(1);
    return QDMI_SUCCESS;
}

int QDMI_control_submit(QDMI_Device dev, QDMI_Fragment *frag, int numshots, QInfo info, QDMI_Job *job){
    printf("    [Backend]..............Control submit\n");
    return QDMI_SUCCESS;
}

int QDMI_control_readout_raw_num(QDMI_Device dev, QDMI_Status *status, int task_id, int *num)
{
    printf("    [Backend]..............Control readout raw num\n");

    *num = 1;
    return QDMI_SUCCESS;
}

int QDMI_query_device_property_c(QDMI_Device dev, QDMI_Device_property prop, char** value)
{
    printf("    [Backend]..............Querying device property\n");

    *value = malloc(sizeof(char) * 10);
    strcpy(*value, "test");
    return QDMI_SUCCESS;
}

// Dummy implementations for the QDMI library functions

int QDMI_control_readout_hist_size(QDMI_Device dev, QDMI_Status *status, int *size) {
    printf("    [Backend]..............Control readout hist size\n");
    *size = 1;
    return QDMI_SUCCESS;
}

int QDMI_query_device_property_exists(QDMI_Device dev, QDMI_Device_property prop, int *exists) {
    printf("    [Backend]..............Query device property exists\n");
    *exists = 1;
    return QDMI_SUCCESS;
}

int QDMI_query_device_property_f(QDMI_Device dev, QDMI_Device_property prop, float *value) {
    printf("    [Backend]..............Query device property f\n");
    *value = 1.0f;
    return QDMI_SUCCESS;
}

int QDMI_query_device_property_d(QDMI_Device dev, QDMI_Device_property prop, double *value) {
    printf("    [Backend]..............Query device property d\n");
    *value = 1.0;
    return QDMI_SUCCESS;
}

int QDMI_query_gateset_num(QDMI_Device dev, int *num) {
    printf("    [Backend]..............Query gateset num\n");
    *num = 1;
    return QDMI_SUCCESS;
}

int QDMI_query_qubits_num(QDMI_Device dev, int *num) {
    printf("    [Backend]..............Query qubits num\n");
    *num = 1;
    return QDMI_SUCCESS;
}