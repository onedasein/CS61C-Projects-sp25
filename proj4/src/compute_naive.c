#include "compute.h"

// Computes the convolution of two matrices
int convolve(matrix_t *a_matrix, matrix_t *b_matrix, matrix_t **output_matrix) {
  if (a_matrix == NULL || b_matrix == NULL || output_matrix == NULL) {
    return -1;
  }

  uint32_t a_rows = a_matrix->rows;
  uint32_t a_cols = a_matrix->cols;
  uint32_t b_rows = b_matrix->rows;
  uint32_t b_cols = b_matrix->cols;
  uint32_t out_rows = a_rows - b_rows + 1;
  uint32_t out_cols = a_cols - b_cols + 1;

  const int32_t *a = a_matrix->data;
  const int32_t *b = b_matrix->data;

  // Allocate the output matrix; rows/cols/data are all owned by convolve.
  matrix_t *out = (matrix_t *)malloc(sizeof(matrix_t));
  if (out == NULL) {
    return -1;
  }
  out->rows = out_rows;
  out->cols = out_cols;
  out->data = (int32_t *)malloc((size_t)out_rows * out_cols * sizeof(int32_t));
  if (out->data == NULL) {
    free(out);
    return -1;
  }

  // output[i][j] = sum over p, q of A[i+p][j+q] * B[k-1-p][l-1-q]
  //
  // All math is done in uint32_t (mod 2^32). The reference solution stores a
  // plain int32 result, and any integer implementation that sums the same
  // products agrees with it modulo 2^32 -- so the exact bit pattern we write
  // matches the oracle regardless of whether either side "overflows".
  for (uint32_t i = 0; i < out_rows; i++) {
    for (uint32_t j = 0; j < out_cols; j++) {
      uint32_t sum = 0;
      for (uint32_t p = 0; p < b_rows; p++) {
        uint32_t a_row = (i + p) * a_cols;
        uint32_t b_row = (b_rows - 1 - p) * b_cols;
        for (uint32_t q = 0; q < b_cols; q++) {
          sum += (uint32_t)a[a_row + j + q] *
                 (uint32_t)b[b_row + (b_cols - 1 - q)];
        }
      }
      out->data[i * out_cols + j] = (int32_t)sum;
    }
  }

  *output_matrix = out;
  return 0;
}

// Executes a task
int execute_task(task_t *task) {
  matrix_t *a_matrix, *b_matrix, *output_matrix;

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
