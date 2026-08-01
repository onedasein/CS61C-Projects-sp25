#include <omp.h>
#include <x86intrin.h>

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

  // Allocate the output matrix.
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

  // output[i][j] = sum_{p,q} A[i+p][j+q] * B[k-1-p][l-1-q]
  //
  // Flip B once so every output element is a plain dot product of an A window
  // with the (now contiguous, unflipped) kernel. Doing the flip once here --
  // instead of indexing B backwards inside the hot loop -- removes a
  // multiplication from every kernel access.
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

  // Columns of the kernel that fit into whole 256-bit (8 x int32) vectors.
  uint32_t vec_cols = (b_cols / 8) * 8;

  // Parallelize over output rows: rows are independent, and consecutive rows
  // reuse a sliding window of A rows (good cache behavior per thread).
  #pragma omp parallel for schedule(static)
  for (int32_t i = 0; i < (int32_t)out_rows; i++) {
    for (uint32_t j = 0; j < out_cols; j++) {
      // Accumulate in uint32_t so every operation is well-defined mod 2^32;
      // the stored int32 bit pattern then matches the naive version (and the
      // reference) exactly, including when intermediate sums overflow.
      __m256i vsum = _mm256_setzero_si256();
      uint32_t scalar_sum = 0;

      for (uint32_t p = 0; p < b_rows; p++) {
        // Start of the A window row (i+p), shifted right by j.
        const uint32_t *a_row =
            (const uint32_t *)(a + (uint32_t)(i + (int32_t)p) * a_cols + j);
        const uint32_t *bf_row = bflip + (size_t)p * b_cols;

        uint32_t q = 0;
        // SIMD: 8 products + 8 adds per iteration.
        for (; q < vec_cols; q += 8) {
          __m256i va = _mm256_loadu_si256((const __m256i *)(a_row + q));
          __m256i vb = _mm256_loadu_si256((const __m256i *)(bf_row + q));
          vsum = _mm256_add_epi32(vsum, _mm256_mullo_epi32(va, vb));
        }
        // Tail: remaining columns that don't fill a vector.
        for (; q < b_cols; q++) {
          scalar_sum += a_row[q] * bf_row[q];
        }
      }

      // Sum the 8 lanes of vsum (mod 2^32), then fold in the scalar tail.
      __m128i lo = _mm256_castsi256_si128(vsum);
      __m128i hi = _mm256_extracti128_si256(vsum, 1);
      uint32_t lanes[4];
      _mm_storeu_si128((__m128i *)lanes, _mm_add_epi32(lo, hi));

      uint32_t total = ((lanes[0] + lanes[1]) + (lanes[2] + lanes[3])) + scalar_sum;
      out->data[(size_t)i * out_cols + j] = (int32_t)total;
    }
  }

  free(bflip);
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
