/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2018 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "ompi/mpi/c/bindings.h"
#include "ompi/runtime/params.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/runtime/ompi_spc.h"

#include "ompi/runtime/params.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Finalize = PMPI_Finalize
#endif
#define MPI_Finalize PMPI_Finalize
#endif

static const char FUNC_NAME[] = "MPI_Finalize";


int MPI_Finalize(void)
{
    /* If --with-spc was specified, print all of the final SPC values
     * aggregated across the whole MPI run.
     */
#if SPC_ENABLE == 1
    if(!ompi_mpi_spc_dump_enabled)
        goto skip_dump;

    int i, j, rank, world_size, offset;
    long long *recv_buffer, *send_buffer;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    /* Aggregate all of the information on rank 0 using MPI_Gather on MPI_COMM_WORLD */
    if(rank == 0) {
        send_buffer = (long long*)malloc(OMPI_NUM_COUNTERS * sizeof(long long));
        recv_buffer = (long long*)malloc(world_size * OMPI_NUM_COUNTERS * sizeof(long long));
        for(i = 0; i < OMPI_NUM_COUNTERS; i++) {
            send_buffer[i] = events[i].value;
        }
        MPI_Gather(send_buffer, OMPI_NUM_COUNTERS, MPI_LONG_LONG, recv_buffer, OMPI_NUM_COUNTERS, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    } else {
        send_buffer = (long long*)malloc(OMPI_NUM_COUNTERS * sizeof(long long));
        for(i = 0; i < OMPI_NUM_COUNTERS; i++) {
            send_buffer[i] = events[i].value;
        }
        MPI_Gather(send_buffer, OMPI_NUM_COUNTERS, MPI_LONG_LONG, recv_buffer, OMPI_NUM_COUNTERS, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    }

    /* Once rank 0 has all of the information, print the aggregated counter values for each rank in order */
    if(rank == 0) {
        fprintf(stdout, "OMPI Software Counters:\n");
        offset = 0; /* Offset into the recv_buffer for each rank */
        for(j = 0; j < world_size; j++) {
            fprintf(stdout, "World Rank %d:\n", j);
            for(i = 0; i < OMPI_NUM_COUNTERS; i++) {
                if(attached_event[i]) {
                    /* If this is a timer-based counter, we need to covert from cycles to usecs */
                    if(timer_event[i])
                        SPC_CYCLES_TO_USECS(&recv_buffer[offset+i]);
                    fprintf(stdout, "%s -> %lld\n", events[i].name, recv_buffer[offset+i]);
                } else {
                    fprintf(stdout, "%s -> Disabled\n", events[i].name);
                }
            }
            fprintf(stdout, "\n");
            offset += OMPI_NUM_COUNTERS;
        }
        free(recv_buffer);
        free(send_buffer);
        SPC_FINI();
    } else {
        free(send_buffer);
    }

    MPI_Barrier(MPI_COMM_WORLD);
 skip_dump:
#endif

    OPAL_CR_FINALIZE_LIBRARY();

    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
    }

    /* Pretty simple */

    return ompi_mpi_finalize();
}
