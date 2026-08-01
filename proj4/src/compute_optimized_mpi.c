#include <mpi.h>

#include <omp.h>
#include <x86intrin.h>

#include "compute.h"

// Computes the convolution of two matrices.
//
// MPI version: the output rows are independent, so they are partitioned across
// the MPI ranks; each rank computes its rows into a local buffer and
// MPI_Gatherv assembles the full output on rank 0, which fills *output_matrix.
// Non-root ranks set *output_matrix = NULL (the caller must only dereference
// it on rank 0). If MPI has not been initialized (e.g. running as a plain
// single process), the whole convolution is computed locally so the function
// is still self-sufficient.
int convolve(matrix_t *a_matrix, matrix_t *b_matrix, matrix_t **output_matrix) {
  if (a_matrix == NULL || b_matrix == NULL || output_matrix == NULL) {
    return -1;
  }

  int initialized = 0;
  int rank = 0;
  int size = 1;
  MPI_Initialized(&initialized);
  if (initialized) {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
  }

  uint32_t a_rows = a_matrix->rows;
  uint32_t a_cols = a_matrix->cols;
  uint32_t b_rows = b_matrix->rows;
  uint32_t b_cols = b_matrix->cols;
  uint32_t out_rows = a_rows - b_rows + 1;
  uint32_t out_cols = a_cols - b_cols + 1;

  const int32_t *a = a_matrix->data;
  const int32_t *b = b_matrix->data;

  // output[i][j] = sum_{p,q} A[i+p][j+q] * B[k-1-p][l-1-q].
  // Flip B once so each output element is a plain dot product of an A window
  // with the (contiguous) kernel. All arithmetic is done in uint32_t so every
  // operation is defined mod 2^32, matching the int32 bit pattern the
  // reference solution writes.
  uint32_t *bflip = (uint32_t *)malloc((size_t)b_rows * b_cols * sizeof(uint32_t));
  if (bflip == NULL) {
    return -1;
  }
  for (uint32_t p = 0; p < b_rows; p++) {
    for (uint32_t q = 0; q < b_cols; q++) {
      bflip[(size_t)p * b_cols + q] =
          (uint32_t)b[(b_rows - 1 - p) * b_cols + (b_cols - 1 - q)];
    }
  }

  // Partition the output rows evenly across ranks (block distribution).
  uint32_t base = out_rows / (uint32_t)size;
  uint32_t extra = out_rows % (uint32_t)size;
  uint32_t local_rows = base + ((uint32_t)rank < extra ? 1u : 0u);
  uint32_t local_start =
      ((uint32_t)rank < extra)
          ? (uint32_t)rank * (base + 1)
          : extra * (base + 1) + ((uint32_t)rank - extra) * base;

  int32_t *local = NULL;
  if (local_rows > 0) {
    local = (int32_t *)malloc((size_t)local_rows * out_cols * sizeof(int32_t));
    if (local == NULL) {
      free(bflip);
      return -1;
    }
  }

  for (uint32_t r = 0; r < local_rows; r++) {
    uint32_t i = local_start + r;
    for (uint32_t j = 0; j < out_cols; j++) {
      uint32_t sum = 0;
      for (uint32_t p = 0; p < b_rows; p++) {
        uint32_t a_row = (i + p) * a_cols;
        uint32_t b_row = p * b_cols;
        for (uint32_t q = 0; q < b_cols; q++) {
          sum += (uint32_t)a[a_row + j + q] * bflip[b_row + q];
        }
      }
      local[r * out_cols + j] = (int32_t)sum;
    }
  }

  free(bflip);

  if (initialized) {
    int *counts = NULL;
    int *displs = NULL;
    int32_t *all = NULL;
    if (rank == 0) {
      counts = (int *)malloc((size_t)size * sizeof(int));
      displs = (int *)malloc((size_t)size * sizeof(int));
      all = (int32_t *)malloc((size_t)out_rows * out_cols * sizeof(int32_t));
      if (counts == NULL || displs == NULL || all == NULL) {
        free(counts);
        free(displs);
        free(all);
        free(local);
        return -1;
      }
      int offset = 0;
      for (int r = 0; r < size; r++) {
        uint32_t rr = base + ((uint32_t)r < extra ? 1u : 0u);
        counts[r] = (int)(rr * out_cols);
        displs[r] = offset;
        offset += counts[r];
      }
    }

    MPI_Gatherv(local, (int)(local_rows * out_cols), MPI_INT32_T,
                all, counts, displs, MPI_INT32_T, 0, MPI_COMM_WORLD);

    if (rank == 0) {
      matrix_t *out = (matrix_t *)malloc(sizeof(matrix_t));
      if (out == NULL) {
        free(all);
        free(counts);
        free(displs);
        free(local);
        return -1;
      }
      out->rows = out_rows;
      out->cols = out_cols;
      out->data = all;  // assembled full output
      *output_matrix = out;
      free(counts);
      free(displs);
    } else {
      *output_matrix = NULL;
    }
    free(local);
    return 0;
  }

  // MPI not initialized: this process computes the whole output.
  matrix_t *out = (matrix_t *)malloc(sizeof(matrix_t));
  if (out == NULL) {
    free(local);
    return -1;
  }
  out->rows = out_rows;
  out->cols = out_cols;
  out->data = local;
  *output_matrix = out;
  return 0;
}

// Executes a task. Under MPI every rank reads the inputs and runs convolve,
// but only rank 0 writes the assembled output file.
int execute_task(task_t *task) {
  matrix_t *a_matrix, *b_matrix, *output_matrix;

  int initialized = 0;
  int rank = 0;
  MPI_Initialized(&initialized);
  if (initialized) {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  }

  char *a_matrix_path = get_a_matrix_path(task);
  if (read_matrix(a_matrix_path, &a_matrix)) {
    printf("Error reading matrix from %s\n", a_matrix_path);
    return -1;
  }
  free(a_matrix_path);

  char *b_matrix_path = get_b_matrix_path(task);
  if (read_matrix(b_matrix_path, &b_matrix)) {
    printf("Error reading matrix from %s\n", b_matrix_path);
    return -1;
  }
  free(b_matrix_path);

  if (convolve(a_matrix, b_matrix, &output_matrix)) {
    printf("convolve returned a non-zero integer\n");
    return -1;
  }

  if (initialized) {
    free(a_matrix->data);
    free(b_matrix->data);
    free(a_matrix);
    free(b_matrix);
    if (rank == 0) {
      char *output_matrix_path = get_output_matrix_path(task);
      if (write_matrix(output_matrix_path, output_matrix)) {
        printf("Error writing matrix to %s\n", output_matrix_path);
        free(output_matrix_path);
        free(output_matrix->data);
        free(output_matrix);
        return -1;
      }
      free(output_matrix_path);
      free(output_matrix->data);
      free(output_matrix);
    }
    return 0;
  }

  char *output_matrix_path = get_output_matrix_path(task);
  if (write_matrix(output_matrix_path, output_matrix)) {
    printf("Error writing matrix to %s\n", output_matrix_path);
    return -1;
  }
  free(output_matrix_path);

  free(a_matrix->data);
  free(b_matrix->data);
  free(output_matrix->data);
  free(a_matrix);
  free(b_matrix);
  free(output_matrix);
  return 0;
}
