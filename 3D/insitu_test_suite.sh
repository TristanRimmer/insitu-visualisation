if ! ./compile.sh; then
    echo "Error: Compilation failed. Exiting."
    exit 1
fi

echo "Compilation successful. Running MPI tests..."

for NPROCS in $(seq 1 64); do
    echo "Running with ${NPROCS} MPI processes..."

    if ! mpiexec -n "${NPROCS}" --oversubscribe \
        ./build/InsituVisualisationWithMPI \
        > "execution_times_${NPROCS}.txt"; then
        echo "Error: Execution failed with ${NPROCS} processes."
        exit 1
    fi
done
