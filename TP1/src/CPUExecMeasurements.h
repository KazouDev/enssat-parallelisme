#include <time.h>


#define InitCPUTimer    struct timespec startCPU, stopCPU
#define StartCPUTimer   clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startCPU)
#define StopCPUTimer   clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stopCPU)
#define SECONDE  		1000000000L
#define MILLISECONDE 		1000000L
#define MICROSECONDE 		1000L
#define NANOSECONDE 		1L
#define CPUSecExecTps "%.9f s\n",(( stopCPU.tv_sec - startCPU.tv_sec )+ (stopCPU.tv_nsec - startCPU.tv_nsec )/(double)SECONDE) 
#define CPUMilliSecExecTps "%.6f ms\n",(( stopCPU.tv_sec - startCPU.tv_sec )+ (stopCPU.tv_nsec - startCPU.tv_nsec )/(double)MILLISECONDE) 
#define CPUMicroSecExecTps "%.3f us\n",(( stopCPU.tv_sec - startCPU.tv_sec )+ (stopCPU.tv_nsec - startCPU.tv_nsec )/(double)MICROSECONDE) 
#define CPUNanoSecExecTps "%.1f ns\n",(( stopCPU.tv_sec - startCPU.tv_sec )+ (stopCPU.tv_nsec - startCPU.tv_nsec )/(double)NANOSECONDE) 

#define DEBUG (true)
#define TPSCALCUL (true)

#define PrintCPUExecutionTime(texte, quoi) { 	\
	printf("%s ==> ", texte); \
	printf(quoi) ;}


