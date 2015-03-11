#include <mpi.h>

#include <iostream>
#include <vector> 

#include "tests/mpi_sa_test.hpp"


void my_mpi_errorhandler(MPI_Comm*, int* errorcode, ...)
{
    // throw exception, enables gdb stack trace analysis
    throw std::runtime_error("MPI Error!");
}

int main(int argc, char *argv[])
{
    // set up MPI
    MPI_Init(&argc, &argv);

    // get communicator size and my rank
    MPI_Comm comm = MPI_COMM_WORLD;
    int p, rank;
    MPI_Comm_size(comm, &p);
    MPI_Comm_rank(comm, &rank);

    // set custom error handler (for debugging with working stack-trace on gdb)
    MPI_Errhandler errhandler;
    MPI_Errhandler_create(&my_mpi_errorhandler, &errhandler);
    MPI_Errhandler_set(comm, errhandler);

    // run the suffix array construction
    //sa_test_random_dna(comm, 20000000);
    //sa_test_random_dna(comm, 10000, true);
    //sa_test_random_dna(comm, 10000, true);
    //sa_test_random_dna(comm, 100000, true);
    if (argc < 2)
    {
        std::cerr << "Usage ./test_sac <filename>" << std::endl;
        exit(1);
    }
    sa_test_file(argv[1], comm, 0, false);

    // finalize MPI
    MPI_Finalize();
    return 0;
}
