#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <x86intrin.h>

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

int min (int A , int B){
  if( A < B){
    return A;
  }
  
  else{
    return B; 
  }

}

void mm_cb (dtype *C, dtype *A, dtype *B, int N, int K, int M)
{
  int i, j, k, i0 , j0 , k0 , block;
  block = 4;
  double temp1 , temp2;
  
  for(int i0 = 0; i0 < N ; i0 += block) {
    for(int j0 = 0; j0 < M  ; j0 += block ) {
      for(int k0 = 0; k0 < K; k0 += block) {
        for(int i = i0; i < min(i0 + block ,N) ; i++) {
          for(int j = j0; j < min(j0 + block ,K); j++) {
            for(int k = k0; k < min(k0 + block ,M); k++) {
			  temp1 =  A[i * K + k];
			  temp2 = B[k * M+ j];
              C[i * M + j] += temp1 * temp2;
            }
          }
        }
      }
    }
  }
}


void mm_sv (dtype *C, dtype *A, dtype *B, int N, int K, int M)
{
  __m128d vecA, vecB , vecC;
  int i, j, k, i0 , j0 , k0 , block;
  block = 4;
  
  for(int i0 = 0; i0 < N ; i0 += block) {
    for(int j0 = 0; j0 < M  ; j0 += block ) {
      for(int k0 = 0; k0 < K; k0 += block) {
        for(int i = i0; i < min(i0 + block ,N) ; i++) {
          for(int j = j0; j < min(j0 + block ,K); j+=2) {
			vecC = _mm_load_pd(C +(i*M + j));
			for(int k = k0; k < min(k0 + block ,M); k++) {
			  vecA = _mm_set1_pd(A[i * K + k]);
			  vecB = _mm_load_pd(B + (k * M + j));
              vecC = _mm_add_pd(_mm_mul_pd(vecA, vecB) , vecC);
            }
			_mm_store_pd(C + (i*M +j) , vecC);
          }
        }
      }
    }
  }
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
