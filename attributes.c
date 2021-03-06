/* Inter-Communicator MPI Benchmarking
 * Copyright (c) 2021 Stefan Christians
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>

#include "attributes.h"

/*****************************************************************************/

/*
 * Presence of the benchmark communicator's initiator attribute indicates
 * that a process belongs to the group initiating communication (client, master,
 * left, local) rather than responding (server, worker, right, remote).
 */

/*
 * key for initiator attribute
 */
int icmb_key_initiator = MPI_KEYVAL_INVALID;

/*
 * sets initiator attribute on given communicator
 */
int icmb_set_initiator_attribute(MPI_Comm comm)
{
    // initialize key
    if (MPI_KEYVAL_INVALID == icmb_key_initiator)
    {
        if (MPI_SUCCESS != MPI_Comm_create_keyval(MPI_COMM_DUP_FN, MPI_COMM_NULL_DELETE_FN, &icmb_key_initiator, NULL))
        {
            return MPI_ERR_INTERN;
        }
    }

    // cache attribute in communicator
    return MPI_Comm_set_attr(comm, icmb_key_initiator, NULL);
}

/*
 * returns initiator attribute from given communicator
 */
int icmb_get_initiator_attribute(MPI_Comm comm)
{
    // no key means attribute was not set
    if (MPI_KEYVAL_INVALID == icmb_key_initiator)
    {
        return 0;
    }

    // read attribute
    // (we are only interested in its presence)
    int* value;
    int found;
    MPI_Comm_get_attr(comm, icmb_key_initiator, &value, &found);

    return found;
}

/*****************************************************************************/

/*
 * The benchmark communicator's intercommunicatortype attribute
 * stores how the inter-communicator was constructed
 */

/*
 * key for type attribute
 */
int icmb_key_intercommunicatortype = MPI_KEYVAL_INVALID;

/*
 * callback function when benchmark communicator is duplicated
 */
int icmb_attribute_intercommunicatortype_copier (MPI_Comm comm, int key, void* state, void* value_in, void* value_out, int* flag)
{
    icmb_attribute_intercommunicatortype_t* attribute_in = (icmb_attribute_intercommunicatortype_t*)value_in;
    icmb_attribute_intercommunicatortype_t** attribute_out = (icmb_attribute_intercommunicatortype_t**)value_out;
    attribute_in->ref_count++;
    *attribute_out = attribute_in;
    *flag = 1;
    return MPI_SUCCESS;
}

/*
 * callback function when benchmark communicator is freed
 */
int icmb_attribute_intercommunicatortype_destructor (MPI_Comm comm, int key, void* value, void* state)
{
    icmb_attribute_intercommunicatortype_t* attribute = (icmb_attribute_intercommunicatortype_t*)value;
    attribute->ref_count--;
    if (attribute->ref_count < 1) {
        free((void*) attribute );
    }
    return MPI_SUCCESS;
}

/*
 * sets type attribute on given communicator
 */
int icmb_set_intercommunicatortype_attribute(MPI_Comm comm, enum IntercommConstructionMethod method)
{
    // initialize key
    if (MPI_KEYVAL_INVALID == icmb_key_intercommunicatortype)
    {
        if (MPI_SUCCESS != MPI_Comm_create_keyval(icmb_attribute_intercommunicatortype_copier, icmb_attribute_intercommunicatortype_destructor, &icmb_key_intercommunicatortype, NULL))
        {
            return MPI_ERR_INTERN;
        }
    }

    // create attribute and store inter-communicator construction method
    icmb_attribute_intercommunicatortype_t* attribute = (icmb_attribute_intercommunicatortype_t*) malloc(sizeof(icmb_attribute_intercommunicatortype_t));
    attribute->ref_count = 1;
    attribute->method = method;

    // cache inter-communicator construction method in communicator
    return MPI_Comm_set_attr(comm, icmb_key_intercommunicatortype, attribute);
}

/*
 * returns initiator attribute from given communicator
 */
enum IntercommConstructionMethod icmb_get_intercommunicatortype_attribute(MPI_Comm comm)
{
    // no key means attribute was not set
    if (MPI_KEYVAL_INVALID == icmb_key_initiator)
    {
        return ICMB_METHOD_NONE;
    }

    // read attribute
    icmb_attribute_intercommunicatortype_t* value;
    int found;
    MPI_Comm_get_attr(comm, icmb_key_intercommunicatortype, &value, &found);

    if (!found)
    {
        return ICMB_METHOD_NONE;

    }

    return value->method;
}

/*****************************************************************************/

/*
 * The benchmark communicator's port attribute is a structure containing the
 * port name on which a server process is listening.
 * When the last reference to the port is removed, the port is closed.
 */

/*
 * key for port attribute
 */
int icmb_key_port = MPI_KEYVAL_INVALID;

/*
 * callback function when benchmark communicator is duplicated
 */
int icmb_attribute_port_copier (MPI_Comm comm, int key, void* state, void* value_in, void* value_out, int* flag)
{
    icmb_attribute_port_t* attribute_in = (icmb_attribute_port_t*)value_in;
    icmb_attribute_port_t** attribute_out = (icmb_attribute_port_t**)value_out;
    attribute_in->ref_count++;
    *attribute_out = attribute_in;
    *flag = 1;
    return MPI_SUCCESS;
}

/*
 * callback function when benchmark communicator is freed
 */
int icmb_attribute_port_destructor (MPI_Comm comm, int key, void* value, void* state)
{
    icmb_attribute_port_t* attribute = (icmb_attribute_port_t*)value;
    attribute->ref_count--;
    if (attribute->ref_count < 1) {
		MPI_Close_port(attribute->port_name);
        free(attribute->port_name);
        free((void*) attribute );
    }
    return MPI_SUCCESS;
}

/*
 * sets port attribute on given communicator
 */
int icmb_set_port_attribute(MPI_Comm comm, const char* port_name)
{
    // initialize key
    if (MPI_KEYVAL_INVALID == icmb_key_port)
    {
        if (MPI_SUCCESS != MPI_Comm_create_keyval(icmb_attribute_port_copier, icmb_attribute_port_destructor, &icmb_key_port, NULL))
        {
            return MPI_ERR_INTERN;
        }
    }

    // create copy of port name
    char* port = (char*)malloc((strlen(port_name)+1) * sizeof(char));
    strcpy(port, port_name);

    // create attribute and store port name
    icmb_attribute_port_t* attribute = (icmb_attribute_port_t*) malloc(sizeof(icmb_attribute_port_t));
    attribute->ref_count = 1;
    attribute->port_name = port;

    // cache port name in communicator
    return MPI_Comm_set_attr(comm, icmb_key_port, attribute);
}

/*****************************************************************************/

/*
 * MPI_COMM_SELF's benchmark communicator attribute is a handle for the
 * communicator to be used for benchmarking.
 * It can be an inter- or an intra-communicator.
 * When the last reference to the benchmark communicator is removed, the
 * communicator is freed.
 */

/*
 * key for benchmark communicator attribute
 */
int icmb_key_benchmarkcommunicator = MPI_KEYVAL_INVALID;

/*
 * callback function when MPI_COMM_SELF is duplicated
 */
int icmb_attribute_communicator_copier (MPI_Comm comm, int key, void* state, void* value_in, void* value_out, int* flag)
{
    icmb_attribute_communicator_t* attribute_in = (icmb_attribute_communicator_t*)value_in;
    icmb_attribute_communicator_t** attribute_out = (icmb_attribute_communicator_t**)value_out;
    attribute_in->ref_count++;
    *attribute_out = attribute_in;
    *flag = 1;
    return MPI_SUCCESS;
}

/*
 * callback function when MPI_COMM_SELF communicator is freed
 */
int icmb_attribute_communicator_destructor (MPI_Comm comm, int key, void* value, void* state)
{
    icmb_attribute_communicator_t* attribute = (icmb_attribute_communicator_t*)value;
    attribute->ref_count--;
    if (attribute->ref_count < 1) {
        // actually prefer MPI_Comm_disconnect here,
        // but then openMPI hangs on MPI_Finalize()
        // (no problem with MPICH)
        MPI_Comm_free(&attribute->communicator);
        free((void*) attribute);
    }
    return MPI_SUCCESS;
}

/*
 * sets benchmark communicator attribute on MPI_COMM_SELF
 */
int icmb_set_benchmarkcommunicator_attribute (MPI_Comm benchmark_communicator)
{
    // initialize key
    if (MPI_KEYVAL_INVALID == icmb_key_benchmarkcommunicator)
    {
        if (MPI_SUCCESS != MPI_Comm_create_keyval(icmb_attribute_communicator_copier, icmb_attribute_communicator_destructor, &icmb_key_benchmarkcommunicator, NULL))
        {
            return MPI_ERR_INTERN;
        }
    }

    // create attribute and store communicator
    icmb_attribute_communicator_t* attribute = (icmb_attribute_communicator_t*) malloc(sizeof(icmb_attribute_communicator_t));
    attribute->ref_count = 1;
    attribute->communicator = benchmark_communicator;

    // cache benchmark communicator in MPI_COMM_SELF
    return MPI_Comm_set_attr(MPI_COMM_SELF, icmb_key_benchmarkcommunicator, attribute);
}

/*
 * returns benchmark communicator attribute from MPI_COMM_SELF
 */
MPI_Comm icmb_get_benchmarkcommunicator_attribute()
{
    // no key means attribute was not set
    if (MPI_KEYVAL_INVALID == icmb_key_benchmarkcommunicator)
    {
        return MPI_COMM_NULL;
    }

    // read attribute
    icmb_attribute_communicator_t* value;
    int found;
    MPI_Comm_get_attr(MPI_COMM_SELF, icmb_key_benchmarkcommunicator, &value, &found);

    if (!found)
    {
        return MPI_COMM_NULL;

    }

    return value->communicator;
}

/*****************************************************************************/

/*
 * MPI_COMM_SELF's global communicator attribute is a handle for an intra-
 * communicator spanning all processes.
 * It is used for synchronizing the processes and collecting measurement
 * results.
 * When the last reference to the global communicator is removed, the
 * communicator is freed.
 */

/*
 * key for global communicator attribute
 */
int icmb_key_globalcommunicator = MPI_KEYVAL_INVALID;

/*
 * sets global communicator attribute on MPI_COMM_SELF
 */
int icmb_set_globalcommunicator_attribute (MPI_Comm global_communicator)
{
    // initialize key
    if (MPI_KEYVAL_INVALID == icmb_key_globalcommunicator)
    {
        if (MPI_SUCCESS != MPI_Comm_create_keyval(icmb_attribute_communicator_copier, icmb_attribute_communicator_destructor, &icmb_key_globalcommunicator, NULL))
        {
            return MPI_ERR_INTERN;
        }
    }

    // create attribute and store communicator
    icmb_attribute_communicator_t* attribute = (icmb_attribute_communicator_t*) malloc(sizeof(icmb_attribute_communicator_t));
    attribute->ref_count = 1;
    attribute->communicator = global_communicator;

    // cache global communicator in MPI_COMM_SELF
    return MPI_Comm_set_attr(MPI_COMM_SELF, icmb_key_globalcommunicator, attribute);
}

/*
 * returns global communicator attribute from MPI_COMM_SELF
 */
MPI_Comm icmb_get_globalcommunicator_attribute()
{
    // no key means attribute was not set
    if (MPI_KEYVAL_INVALID == icmb_key_globalcommunicator)
    {
        return MPI_COMM_NULL;
    }

    // read attribute
    icmb_attribute_communicator_t* value;
    int found;
    MPI_Comm_get_attr(MPI_COMM_SELF, icmb_key_globalcommunicator, &value, &found);

    if (!found)
    {
        return MPI_COMM_NULL;

    }

    return value->communicator;
}

/*****************************************************************************/

/*
 * MPI_COMM_SELF's partial communicator attribute is a handle for an intra-
 * communicator to access other members of the local group.
 * When the last reference to the partial communicator is removed, the
 * communicator is freed.
 */

/*
 * key for partial communicator attribute
 */
int icmb_key_partialcommunicator = MPI_KEYVAL_INVALID;

/*
 * sets partial communicator attribute on MPI_COMM_SELF
 */
int icmb_set_partialcommunicator_attribute (MPI_Comm partial_communicator)
{
    // initialize key
    if (MPI_KEYVAL_INVALID == icmb_key_partialcommunicator)
    {
        if (MPI_SUCCESS != MPI_Comm_create_keyval(icmb_attribute_communicator_copier, icmb_attribute_communicator_destructor, &icmb_key_partialcommunicator, NULL))
        {
            return MPI_ERR_INTERN;
        }
    }

    // create attribute and store communicator
    icmb_attribute_communicator_t* attribute = (icmb_attribute_communicator_t*) malloc(sizeof(icmb_attribute_communicator_t));
    attribute->ref_count = 1;
    attribute->communicator = partial_communicator;

    // cache partial communicator in MPI_COMM_SELF
    return MPI_Comm_set_attr(MPI_COMM_SELF, icmb_key_partialcommunicator, attribute);
}

/*
 * returns partial communicator attribute from MPI_COMM_SELF
 */
MPI_Comm icmb_get_partialcommunicator_attribute()
{
    // no key means attribute was not set
    if (MPI_KEYVAL_INVALID == icmb_key_partialcommunicator)
    {
        return MPI_COMM_NULL;
    }

    // read attribute
    icmb_attribute_communicator_t* value;
    int found;
    MPI_Comm_get_attr(MPI_COMM_SELF, icmb_key_partialcommunicator, &value, &found);

    if (!found)
    {
        return MPI_COMM_NULL;

    }

    return value->communicator;
}
