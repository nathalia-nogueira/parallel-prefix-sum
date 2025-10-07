// TRABALHO1: ci1316 2o semestre 2025
// Aluno1: Bianca Mendes Francisco    GRR: 20234263
// Aluno2: Nathalia Nogueira Alves    GRR: 20232349

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "chrono.c"

#define DEBUG 0
// #define SEQUENTIAL_VERSION 1      // ATENÇAO: COMENTAR esse #define para rodar o seu codigo paralelo 
#define MAX_THREADS 64
#define LONG_LONG_T  1
#define DOUBLE_T     2
#define UINT_T        3

// ESCOLHA o tipo dos elementos usando o #define MY_TYPE adequado abaixo a fazer a SOMA DE PREFIXOS:
#define MY_TYPE LONG_LONG_T        
// #define MY_TYPE DOUBLE_T 
// #define MY_TYPE UINT_T

#if MY_TYPE == LONG_LONG_T
  #define TYPE long long
  #define TYPE_NAME  "long long"
  #define TYPE_FORMAT "%lld"
#elif MY_TYPE == DOUBLE_T
  #define TYPE double
  #define TYPE_NAME  "double"
  #define TYPE_FORMAT "%F"   
#elif MY_TYPE == UINT_T
  #define TYPE unsigned int   
  #define TYPE_NAME  "unsigned int"
  #define TYPE_FORMAT "%u"
#endif

#define MAX_TOTAL_ELEMENTS (500 * 1000 * 1000) 
                                                   
int nThreads;		        // numero efetivo de threads obtido da linha de comando
int nTotalElements;     // numero total de elementos obtido da linha de comando  
volatile TYPE *Vector;	// a pointer to the GLOBAL Vector that will by processed by the threads - this will be allocated by malloc - will use malloc e free to allow large (>2GB) vectors
chronometer_t parallelPrefixSumTime;
chronometer_t memcpyTime;
volatile TYPE partialSum[MAX_THREADS];
pthread_barrier_t syncBarrier;

int min (int a, int b) {
	if (a < b)
		return a;
	else
		return b;
}

void verifyPrefixSum(const TYPE *InputVec, volatile TYPE *prefixSumVec, long nTotalElmts) {
  volatile TYPE last = InputVec[0];
  int ok = 1;

  for (long i = 1; i < nTotalElmts; i++) {
    if (prefixSumVec[i] != (InputVec[i] + last)) {
      fprintf(stderr, "In[%ld]= " TYPE_FORMAT "\n"
                      "Out[%ld]= " TYPE_FORMAT " (wrong result!)\n"
                      "Out[%ld]= " TYPE_FORMAT " (ok)\n"
                      "last=" TYPE_FORMAT "\n" , 
                      i, InputVec[i],
                      i, prefixSumVec[i],
                      i-1, prefixSumVec[i-1],
                      last);
      ok = 0;
      break;
    }
    last = prefixSumVec[i];    
  }  
  
  if (ok)
    printf("\nPrefix Sum verified correctly.\n");
  else
    printf("\nPrefix Sum DID NOT compute correctly!!!\n");   
}

void sequentialPrefixSum(volatile TYPE *Vec, long nTotalElmts, int nThreads) {
  TYPE last = Vec[0];
  int ok = 1;

  for (long i = 1; i<nTotalElmts; i++)
    Vec[i] += Vec[i-1];
}  

void *prefixSumBody(void *ptr) {
  int myId = *((int *)ptr);
  int nElementsPerThread = (nTotalElements + nThreads - 1) / nThreads;
  int first = myId * nElementsPerThread;
  int last = min((myId + 1) * nElementsPerThread, nTotalElements);

  while (1) {
    pthread_barrier_wait(&syncBarrier);

    TYPE myPartialSum = 0;
    for (int i = first; i < last; i++) {
      myPartialSum += Vector[i];
    }
    partialSum[myId] = myPartialSum;
 
    pthread_barrier_wait(&syncBarrier);

    TYPE myPrefixSum = 0;
    for (int i = 0; i < myId; i++) {
      myPrefixSum += partialSum[i];
    }

    TYPE localPrefixSum = 0;
    for (int i = first; i < last; i++) {
      localPrefixSum += Vector[i];           
      Vector[i] = myPrefixSum + localPrefixSum;   
    }

    pthread_barrier_wait(&syncBarrier);

    if (myId == 0)
      return NULL;
    }

    return NULL;
}

void parallelPrefixSumPth(volatile TYPE *Vec, long numTotalElmts, int numThreads) {
  static int initialized = 0;
  pthread_t Thread[MAX_THREADS];
  int myThreadId[MAX_THREADS];

  nTotalElements = numTotalElmts;
  nThreads = numThreads;
  Vector = Vec;

  if (!initialized) {
    pthread_barrier_init(&syncBarrier, NULL, nThreads);
    
    myThreadId[0] = 0;
    for (int i = 1; i < nThreads; i++) {
      myThreadId[i] = i;
      pthread_create(&Thread[i], NULL, prefixSumBody, &myThreadId[i]);
    }
    
    initialized = 1;
  }

  prefixSumBody(&myThreadId[0]);
}

int main(int argc, char *argv[]) {
	long i;

	if (argc != 3) {
		printf("usage: %s <nTotalElements> <nThreads>\n", argv[0]);
		return 0;
	}	else {
		nThreads = atoi(argv[2]);
		if (nThreads == 0) {
			printf("usage: %s <nTotalElements> <nThreads>\n", argv[0]);
			printf("<nThreads> can't be 0\n");
			return 0;
		}

		if (nThreads > MAX_THREADS) {
			printf("usage: %s <nTotalElements> <nThreads>\n", argv[0]);
			printf("<nThreads> must be less than %d\n", MAX_THREADS);
			return 0;
		}

		nTotalElements = atoi(argv[1]);
		if (nTotalElements > MAX_TOTAL_ELEMENTS) {
			printf("usage: %s <nTotalElements> <nThreads>\n", argv[0]);
			printf("<nTotalElements> must be up to %d\n", MAX_TOTAL_ELEMENTS);
			return 0;
		}
	}

  // allocate the GLOBAL Vector that will by processed by the threads  
  Vector = (TYPE *) malloc(nTotalElements*sizeof(TYPE));
  if (Vector == NULL)
    printf("Error allocating working Vector of %d elements (size=%ld Bytes)\n", nTotalElements, nTotalElements*sizeof(TYPE));
    
  // allocate space for the initial vector 
  TYPE *InitVector = (TYPE *) malloc(nTotalElements*sizeof(TYPE));
  if (InitVector == NULL)
    printf("Error allocating initVector of %d elements (size=%ld Bytes)\n", nTotalElements, nTotalElements*sizeof(TYPE));

  // Print INFOS about the prefix sum algorithm
  printf("Using PREFIX SUM of TYPE %s\n", TYPE_NAME);
        
  int r;
	for (long i = 0; i < nTotalElements; i++) {
	  r = rand(); 
		InitVector[i] = (r % 10);
	}

	printf("\n\nwill use %d threads to calculate prefix-sum of %d total elements\n", nThreads, nTotalElements);

  chronoReset(&memcpyTime);      
	chronoReset(&parallelPrefixSumTime);
	chronoStart(&parallelPrefixSumTime);

  #define NTIMES 1000
  for (int i = 0; i<NTIMES; i++) {
    // make a copy, measure time taken
    chronoStart( &memcpyTime );
    memcpy( (void *)Vector, (void *)InitVector, nTotalElements * sizeof(TYPE) );
    chronoStop( &memcpyTime );
                
    #ifdef SEQUENTIAL_VERSION
      sequentialPrefixSum( Vector, nTotalElements, nThreads );
    #else
      // run your parallelPrefixSumPth algorithm (with thread pool)
      parallelPrefixSumPth( Vector, nTotalElements, nThreads );
    #endif   
  }     
      
	chronoStop(&parallelPrefixSumTime);
	// DESCONTAR o tempo das memcpys no cronometro ...
	//   ... pois só queremos saber o tempo do algoritmo de prefixSum
  chronoDecrease(&parallelPrefixSumTime, chronoGetTotal(&memcpyTime));

  // reportar o tempo após o desconto dos memcpys
	chronoReportTime(&parallelPrefixSumTime, "parallelPrefixSumTime");
        
	// calcular e imprimir a VAZAO (numero de operacoes/s)
	// descontar o tempo das memcpys pois só queremos saber o tempo do algoritmo de prefixSum
	double totalTimeInSeconds = (double)chronoGetTotal(&parallelPrefixSumTime) / ((double)1000 * 1000 * 1000);
								   
	printf("totalTimeInSeconds: %lf s for %d prefix-sum ops\n", totalTimeInSeconds, NTIMES);

	double OPS = ((long)nTotalElements * NTIMES) / totalTimeInSeconds;
	printf("Throughput: %lf OP/s\n", OPS);

  verifyPrefixSum(InitVector, Vector, nTotalElements);
        
  chronoReportTime(&memcpyTime, "memcpyTime");
    
  #if DEBUG >= 2 
    // Print InputVector
    printf("In: ");
    for (int i = 0; i < nTotalElements; i++) {
	  	printf("%lld ", InitVector[i]);
    }
	  printf("\n");

    // Print the result of the prefix Sum
    printf("Out: ");
	  for (int i = 0; i < nTotalElements; i++){
		  printf("%lld ", Vector[i]);
	  }
	  printf("\n");
  #endif
	
  return 0;
}