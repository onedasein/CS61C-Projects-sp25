#include <omp.h>
#include <x86intrin.h>

#include "compute.h"

// Computes the convolution of two matrices
int convolve(matrix_t *a_matrix, matrix_t *b_matrix, matrix_t **output_matrix) {
  // TODO: convolve matrix a and matrix b, and store the resulting matrix in
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





      uint32_t *bflip = (uint32_t *)malloc((size_t)b_rows * b_cols * sizeof(uint32_t));
  if (bflip == NULL) {
    free(out->data);
    free(out);
    return -1;
  }
  for (uint32_t p = 0; p < b_rows; p++) {
    for (uint32_t q = 0; q < b_cols; q++) {
      bflip[(size_t)p * b_cols + q] =
          (uint32_t)b[(b_rows - 1 - p) * b_cols + (b_cols - 1 - q)];
    }
  }

  // out[i][j] = sum_{p,q} a[i+p][j+q] * b[b_rows-1-p][b_cols-1-q]  (mod 2^32)
  // out[i * a_cols + j] = sum_{p, q} a[] + b[]
  #pragma omp parallel for schedule(static)
  for (uint32_t i = 0; i < out_rows; i++) {
    uint32_t j = 0;
    for (; j + 7 < out_cols; j+=8) {
      __m256i sum_vec = _mm256_setzero_si256(); 
      for (uint32_t p = 0; p < b_rows; p++) {
        for (uint32_t q = 0; q < b_cols; q++) {
            __m256i a_vec = _mm256_loadu_si256((const __m256i *)&a[(size_t)(i + p)* a_cols + (j + q)]);

            __m256i bflip_vec = _mm256_set1_epi32(bflip[(size_t)p * b_cols + q]);

            sum_vec = _mm256_add_epi32(sum_vec, _mm256_mullo_epi32(a_vec, bflip_vec));
        }
      }

      _mm256_storeu_si256(
          (__m256i*)&out->data[(size_t)i * out_cols + j],
          sum_vec
          );
    }

    for (; j < out_cols; j++) {
        uint32_t sum = 0;
        for (uint32_t p = 0; p < b_rows; p++) {
            for (uint32_t q = 0; q < b_cols; q++) {
                sum += a[(size_t)(i + p) * a_cols + (j + q)] * bflip[(size_t)p*b_cols + q];
            }
        }
        out->data[(size_t)i * out_cols + j] = (int32_t)sum;
    }
  }
  *output_matrix = out;
  return 0;

  // output_matrix

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
