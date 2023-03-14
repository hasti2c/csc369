/* File:     Naive matrix multiplication
 * Adapted from: http://www.cs.usfca.edu/~peter/math202/blocked.c
 *
 * Purpose:  Compare the run time of the standard matrix multiplication
 *           algorithm with blocked matrix multiplication.
 *
 * Compile:  gcc -g -Wall [-DDEBUG] -o matmul matmul.c
 * Run:      ./matmul <order of matrices>
 *              <-> required argument, [-] optional argument
 *
 * Output:   Elapsed time for standard matrix multiplication.
 *           If the DEBUG flag is set, the factor and the product
 *           matrices are also output.
 *
 * Notes:
 * 1.  The file timer.h should be in the directory containing the source file.
 * 2.  Set the DEBUG flag to see the factor and the product matrices.
 * 3.  Matrices are generated using a random number generator.
 * 4.  There are a number of optimizations that can be made to the source
 *     code that will improve the performance of both algorithms.
 * 5.  Note that unless the DEBUG flag is set the product matrices will,
 *     in general, be different using the two algorithms, since the two
 *     algorithms use identical storage for A and B, but they assume
 *     the storage has different structures.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for memset

#include "marker.h"
#include "timer.h"

#define PAD 120

struct record
{
  double value;
  char padding[PAD];
};

// Global Variables
const double DRAND_MAX = RAND_MAX;
struct record *A, *B, *C;
int n;

void
Usage(const char* prog_name);
void
Get_matrices(struct record A[], struct record B[], int n);
void
Mat_mult(void);
void
Print_matrix(struct record M[], int n, const char* name);

/*-------------------------------------------------------------------*/
int
main(int argc, char* argv[])
{
  write_marker_file("matmul.marker");
  MARKER_START();

  if (argc != 2) {
    Usage(argv[0]);
  }
  n = strtol(argv[1], NULL, 10);

  A = malloc(n * n * sizeof(struct record));
  B = malloc(n * n * sizeof(struct record));
  C = malloc(n * n * sizeof(struct record));
  if (A == NULL || B == NULL || C == NULL) {
    fprintf(stderr, "Can't allocate storage!\n");
    exit(1);
  }

  Get_matrices(A, B, n);
#ifdef DEBUG
  Print_matrix(A, n, "A");
  Print_matrix(B, n, "B");
#endif

  double start = get_time();
  Mat_mult();
  double finish = get_time();

#ifdef DEBUG
  Print_matrix(C, n, "C");
#endif

  printf("Elapsed time for standard algorithm = %e seconds\n", finish - start);

  free(A);
  free(B);
  free(C);

  MARKER_END();
  return 0;
}

/*-------------------------------------------------------------------
 * Function:  Usage
 * Purpose:   Print a message showing how the program is used and quit
 * In arg:    prog_name:  the program name
 */
void
Usage(const char* prog_name)
{
  fprintf(stderr, "usage:  %s <order of matrices>\n", prog_name);
  exit(1);
}

/*-------------------------------------------------------------------
 * Function:  Get_matrices
 * Purpose:   Generate the factor matrices
 * In args:   n:     order of the matrices
 * Out args:  A, B:  the matrices
 */
void
Get_matrices(struct record A[], struct record B[], int n)
{
  int i;

  for (i = 0; i < n * n; i++) {
    A[i].value = random() / DRAND_MAX;
    B[i].value = random() / DRAND_MAX;
  }
}

/*-------------------------------------------------------------------
 * Function:    Mat_mult
 * Purpose:     Use the standard algorithm for matrix multiplication
 * Globals in:  A, B:  factor matrices
 *              n:     order of matrices
 * Globals out: C:     the product matrix
 */
void
Mat_mult(void)
{
  int i, j, k;

  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      C[i * n + j].value = 0.0;
      for (k = 0; k < n; k++) {
        C[i * n + j].value += A[i * n + k].value * B[k * n + j].value;
      }
    }
  }
}

/*-------------------------------------------------------------------
 * Function:  Print_matrix
 * Purpose:   Print a matrix on stdout
 * In args:   M:     the matrix
 *            n:     order of matrix
 *            name:  name of the matrix
 */
void
Print_matrix(struct record M[], int n, const char* name)
{
  printf("Matrix %s:\n", name);

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      printf("%.2e ", M[i * n + j].value);
    }
    printf("\n");
  }
  printf("\n");
}
