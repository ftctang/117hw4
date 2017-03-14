#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "timer.c"

#define N_ 4096
#define K_ 4096
#define M_ 4096

typedef double dtype;

void verify(dtype *C, dtype *C_ans, int N, int M)
{
  int i, cnt;
  cnt = 0;
  for(i = 0; i < N * M; i++) {
    if(abs (C[i] - C_ans[i]) > 1e-6) cnt++;
  }
  if(cnt != 0) printf("ERROR\n"); else printf("SUCCESS\n");
}

void mm_serial (dtype *C, dtype *A, dtype *B, int N, int K, int M)
{
  int i, j, k;
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < M; j++) {
      for(int k = 0; k < K; k++) {
        C[i * M + j] += A[i * K + k] * B[k * M + j];
      }
    }
  }
}

void mm_cb (dtype *C, dtype *A, dtype *B, int N, int K, int M)
{
  /* =======================================================+ */
  /* Implement your own cache-blocked matrix-matrix multiply  */
  /* =======================================================+ */
  
  int           i, j, k;
  double        tempC1, tempC2, tempC3, tempC4,
                tempA1, tempA2, tempA3, tempA4,
                tempB1, tempB2, tempB3, tempB4;

  for(i = 0; i < (N/4); i++){
	  for(j = 0; j < (M/4); j++){

		  // setup up block C
		  tempC1 = C[i * M + j*2];
		  tempC2 = C[i * M + (j*2+1)];
		  tempC3 = C[(i+1) * M + j*2];
		  tempC4 = C[(i+1) * M + (j*2+1)];
		  
		  for(k = 0; k < (K/4); k++){
			  
			  //read block A
			  tempA1 = A[i * K + k*2];
			  tempA2 = A[i * K + (k*2+1)];
			  tempA3 = A[(i+1) * K + k*2];
			  tempA4 = A[(i+1) * K + (k*2+1)];
			  
			  //read block B
			  tempB1 = B[k * M + j*2];
			  tempB2 = B[k * M + (j*2+1)];
			  tempB3 = B[(k+1) * M + j*2];
			  tempB4 = B[(k+1) * M + (j*2+1)];
			  
			  //update block C
			  tempC1 += tempA1 * tempB1;
			  tempC2 += tempA2 * tempB2;
			  tempC3 += tempA3 * tempB3;
			  tempC4 += tempA4 * tempB4;
		  }
		  
		  //write values into block C
		  C[i * M + j*2] 			= tempC1;
		  C[i * M + (j*2+1)] 		= tempC2;
		  C[(i+1) * M + j*2] 		= tempC3;
		  C[(i+1) * M + (j*2+1)] 	= tempC4;
	  }
  }
}

void mm_sv (dtype *C, dtype *A, dtype *B, int N, int K, int M)
{
  /* =======================================================+ */
  /* Implement your own SIMD-vectorized matrix-matrix multiply  */
  /* =======================================================+ */
}

int main(int argc, char** argv)
{
  int i, j, k;
  int N, K, M;

  if(argc == 4) {
    N = atoi (argv[1]);		
    K = atoi (argv[2]);		
    M = atoi (argv[3]);		
    printf("N: %d K: %d M: %d\n", N, K, M);
  } else {
    N = N_;
    K = K_;
    M = M_;
    printf("N: %d K: %d M: %d\n", N, K, M);	
  }

  dtype *A = (dtype*) malloc (N * K * sizeof (dtype));
  dtype *B = (dtype*) malloc (K * M * sizeof (dtype));
  dtype *C = (dtype*) malloc (N * M * sizeof (dtype));
  dtype *C_cb = (dtype*) malloc (N * M * sizeof (dtype));
  dtype *C_sv = (dtype*) malloc (N * M * sizeof (dtype));
  assert (A && B && C);

  /* initialize A, B, C */
  srand48 (time (NULL));
  for(i = 0; i < N; i++) {
    for(j = 0; j < K; j++) {
      A[i * K + j] = drand48 ();
    }
  }
  for(i = 0; i < K; i++) {
    for(j = 0; j < M; j++) {
      B[i * M + j] = drand48 ();
    }
  }
  bzero(C, N * M * sizeof (dtype));
  bzero(C_cb, N * M * sizeof (dtype));
  bzero(C_sv, N * M * sizeof (dtype));

  stopwatch_init ();
  struct stopwatch_t* timer = stopwatch_create ();
  assert (timer);
  long double t;

  printf("Naive matrix multiply\n");
  stopwatch_start (timer);
  /* do C += A * B */
  mm_serial (C, A, B, N, K, M);
  t = stopwatch_stop (timer);
  printf("Done\n");
  printf("time for naive implementation: %Lg seconds\n\n", t);


  printf("Cache-blocked matrix multiply\n");
  stopwatch_start (timer);
  /* do C += A * B */
  mm_cb (C_cb, A, B, N, K, M);
  t = stopwatch_stop (timer);
  printf("Done\n");
  printf("time for cache-blocked implementation: %Lg seconds\n", t);

  /* verify answer */
  verify (C_cb, C, N, M);

  printf("SIMD-vectorized Cache-blocked matrix multiply\n");
  stopwatch_start (timer);
  /* do C += A * B */
  mm_sv (C_sv, A, B, N, K, M);
  t = stopwatch_stop (timer);
  printf("Done\n");
  printf("time for SIMD-vectorized cache-blocked implementation: %Lg seconds\n", t);

  /* verify answer */
  verify (C_sv, C, N, M);

  return 0;
}
