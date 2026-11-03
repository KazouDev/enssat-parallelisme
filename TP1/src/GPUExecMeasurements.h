#include <sys/time.h>

#define InitGPUTimer 			\
	cudaEvent_t startGPU, stopGPU; 	\
  	float elapsedTimeGPU;		\
 	cudaEventCreate(&startGPU);	\
 	cudaEventCreate(&stopGPU);	
 
#define StartGPUTimer	{	cudaEventRecord(startGPU,0);   }

#define StopGPUTimer	{		\
 	cudaEventRecord(stopGPU,0);	\
 	cudaEventSynchronize(stopGPU);	\
 	cudaEventElapsedTime(&elapsedTimeGPU, startGPU,stopGPU); }
 	
#define GPUExecTps (elapsedTimeGPU)


#define PrintGPUExecutionTime(texte, quoi) { 	\
	printf("%s ==> %f ms\n" ,texte, quoi);}


