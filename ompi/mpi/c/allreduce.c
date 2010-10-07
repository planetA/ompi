/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2008 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"
#include <stdio.h>

#include "ompi/mpi/c/bindings.h"
#include "ompi/datatype/datatype.h"
#include "ompi/op/op.h"
#include "ompi/memchecker.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Allreduce = PMPI_Allreduce
#endif

#if OMPI_PROFILING_DEFINES
#include "ompi/mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Allreduce";


int MPI_Allreduce(void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) 
{
    int err;

    MEMCHECKER(
        memchecker_datatype(datatype);
        memchecker_comm(comm);
        
        /* check whether receive buffer is defined. */
        memchecker_call(&opal_memchecker_base_isaddressable, recvbuf, count, datatype);
        
        /* check whether the actual send buffer is defined. */
        if (MPI_IN_PLACE == sendbuf) {
            memchecker_call(&opal_memchecker_base_isdefined, recvbuf, count, datatype);
        } else {
            memchecker_call(&opal_memchecker_base_isdefined, sendbuf, count, datatype);
        }
    );
    printf("Past memchecker\n");

    if (MPI_PARAM_CHECK) {
        char *msg;

        /* Unrooted operation -- same checks for all ranks on both
           intracommunicators and intercommunicators */

        err = MPI_SUCCESS;
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        printf("Past err_init_finalize\n");
	if (ompi_comm_invalid(comm)) {
            printf("Bad comm!\n");
	    return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_COMM, 
                                          FUNC_NAME);
        } else if (MPI_OP_NULL == op) {
            printf("Op is null!\n");
            err = MPI_ERR_OP;
        } else if (!ompi_op_is_valid(op, datatype, &msg, FUNC_NAME)) {
            int ret;
            printf("Op is invalid!\n");
            ret = OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_OP, msg);
            free(msg);
            return ret;
        } else if( MPI_IN_PLACE == recvbuf ) {
            printf("recvbuf = MPI_IN_PLACE!\n");
	    return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_BUFFER, 
                                          FUNC_NAME);
        } else if( (sendbuf == recvbuf) && 
                   (MPI_BOTTOM != sendbuf) &&
                   (count > 1) ) {
            printf("BOTTOM = bad!\n");
	    return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_BUFFER, 
                                          FUNC_NAME);
        } else {
            printf("Checking datatype for send\n");
            OMPI_CHECK_DATATYPE_FOR_SEND(err, datatype, count);
        }
        printf("Past all other param checking: err=%d\n", err);
        OMPI_ERRHANDLER_CHECK(err, comm, err, FUNC_NAME);
    }
    printf("Past param check\n");

    /* MPI-1, p114, says that each process must supply at least
       one element.  But at least the Pallas benchmarks call
       MPI_REDUCE with a count of 0.  So be sure to handle it. */
    
    if (0 == count) {
        return MPI_SUCCESS;
    }
    printf("Past 0 count check\n");

    OPAL_CR_ENTER_LIBRARY();
    printf("Past opal_cr_enter_library\n");

    /* Invoke the coll component to perform the back-end operation */

    OBJ_RETAIN(op);
    printf("Past op retain\n");
    err = comm->c_coll.coll_allreduce(sendbuf, recvbuf, count,
                                      datatype, op, comm,
                                      comm->c_coll.coll_allreduce_module);
    printf("Past coll invocation\n");
    OBJ_RELEASE(op);
    printf("Past op release\n");
    OMPI_ERRHANDLER_RETURN(err, comm, err, FUNC_NAME);
}

