#include "serial.h"
#include <stdio.h>
#include "serial.h"
#include <stdint.h>    
#include <FreeRTOS.h>
#include <string.h>
#include <task.h>
#include "processor.h"
#include "interrupt.h"
#include "stdio.h"
#include "semphr.h"
#include "timers.h"
#include <math.h>

uint32_t freert_cpus_lock(void);
void freert_cpus_unlock(uint32_t sr_level);
#define mathSTACK_SIZE          ( ( unsigned short ) 1024 )                        
#define mathNUMBER_OF_TASKS  	( 8 )                                                
                                                                                  
/* Four tasks, each of which performs a different floating point calculation.     
Each of the four is created twice. */                                             
static void vCompetingMathTask1( void *pvParameters );                            
static void vCompetingMathTask2( void *pvParameters );                            
static void vCompetingMathTask3( void *pvParameters );                            
static void vCompetingMathTask4( void *pvParameters );                            
                                                                                  
/* These variables are used to check that all the tasks are still running.  If a  
task gets a calculation wrong it will                                             
stop incrementing its check variable. */                                          
static volatile unsigned short usTaskCheck[ mathNUMBER_OF_TASKS ] = { ( unsigned short ) 0 };

static void vCompetingMathTask1( void *pvParameters )
{       
portDOUBLE d1, d2, d3, d4;
volatile unsigned short *pusTaskCheckVariable;
const portDOUBLE dAnswer = ( 123.4567 + 2345.6789 ) * -918.222;
short sError = pdFALSE;
        
        /* Queue a message for printing to say the task has started. */
        /* The variable this task increments to show it is still running is passed in 
        as the parameter. */
        pusTaskCheckVariable = ( unsigned short * ) pvParameters;
        
        /* Keep performing a calculation and checking the result against a constant. */
        for(;;)
        {       
                d1 = 123.4567;
                d2 = 2345.6789;
                d3 = -918.222;
                d4 = ( d1 + d2 ) * d3;
                taskYIELD();
                /* If the calculation does not match the expected constant, stop the 
                increment of the check variable. */
                if( fabs( d4 - dAnswer ) > 0.001 )
                {    
			SMP_DBG("cal error.\n");
			soft_break();
                }    
        
                if( sError == pdFALSE )
                {    
                        /* If the calculation has always been correct, increment the check 
                        variable so we know this task is still running okay. */
                        ( *pusTaskCheckVariable )++;
                }
        
                if( (*pusTaskCheckVariable ) % 256 == 0)
		{
		    	uint32_t cpu_sr;
		    	cpu_sr = freert_cpus_lock();
			SMP_DBG("success.\n");
			freert_cpus_unlock(cpu_sr);
		}
                taskYIELD();
        }

	vTaskDelete(NULL);
   	return;
}    

static void vCompetingMathTask2( void *pvParameters )
{               
portDOUBLE d1, d2, d3, d4;
volatile unsigned short *pusTaskCheckVariable;
const portDOUBLE dAnswer = ( -389.38 / 32498.2 ) * -2.0001;
short sError = pdFALSE;
                
        /* Queue a message for printing to say the task has started. */
        /* The variable this task increments to show it is still running is passed in 
        as the parameter. */
        pusTaskCheckVariable = ( unsigned short * ) pvParameters;
                
        /* Keep performing a calculation and checking the result against a constant. */
        for( ;; )    
        {       
                d1 = -389.38;
                d2 = 32498.2;
                d3 = -2.0001;
                
                d4 = ( d1 / d2 ) * d3;
                
                taskYIELD();
                
                /* If the calculation does not match the expected constant, stop the 
                increment of the check variable. */
                if( fabs( d4 - dAnswer ) > 0.001 )
                {    
			SMP_DBG("cal error.\n");
			soft_break();
                }    
                
                if( sError == pdFALSE )
                {    
                        /* If the calculation has always been correct, increment the check 
                        variable so we know
                        this task is still running okay. */
                        ( *pusTaskCheckVariable )++;
                }
                     
                if( (*pusTaskCheckVariable ) % 256 == 0)
		{
		    	uint32_t cpu_sr;
		    	cpu_sr = freert_cpus_lock();
			SMP_DBG("success.\n");
			freert_cpus_unlock(cpu_sr);
		}
                taskYIELD();
        }

	vTaskDelete(NULL);
   	return;
}

static void vCompetingMathTask3( void *pvParameters )
{                    
portDOUBLE *pdArray, dTotal1, dTotal2, dDifference;
volatile unsigned short *pusTaskCheckVariable;
const unsigned short usArraySize = 250;
unsigned short usPosition;
short sError = pdFALSE;
                     
        /* Queue a message for printing to say the task has started. */
                     
        /* The variable this task increments to show it is still running is passed in 
        as the parameter. */
        pusTaskCheckVariable = ( unsigned short * ) pvParameters;
                     
        pdArray = ( portDOUBLE * ) pvPortMalloc( ( size_t ) 250 * sizeof( portDOUBLE ) );
                     
        /* Keep filling an array, keeping a running total of the values placed in the 
        array.  Then run through the array adding up all the values.  If the two totals 
        do not match, stop the check variable from incrementing. */
        for( ;; )    
        {            
                dTotal1 = 0.0;
                dTotal2 = 0.0;
                     
                for( usPosition = 0; usPosition < usArraySize; usPosition++ )
                {    
                        pdArray[ usPosition ] = ( portDOUBLE ) usPosition + 5.5;
                        dTotal1 += ( portDOUBLE ) usPosition + 5.5;     
                }    
                     
                taskYIELD();
                     
                for( usPosition = 0; usPosition < usArraySize; usPosition++ )
                {    
                        dTotal2 += pdArray[ usPosition ];
                }    
                     
                dDifference = dTotal1 - dTotal2;
                if( fabs( dDifference ) > 0.001 )
                {     
			SMP_DBG("cal error.\n");
			soft_break();
                }     
                      
                if( sError == pdFALSE )
                {     
                        /* If the calculation has always been correct, increment the check 
                        variable so we know     this task is still running okay. */
                        ( *pusTaskCheckVariable )++;
                }     

                if( (*pusTaskCheckVariable ) % 256 == 0)
		{
		    	uint32_t cpu_sr;
		    	cpu_sr = freert_cpus_lock();
			SMP_DBG("success.\n");
		    	freert_cpus_unlock(cpu_sr);
		}
                taskYIELD();
        }

	vTaskDelete(NULL);
   	return;
}

static void vCompetingMathTask4( void *pvParameters )
{                     
portDOUBLE *pdArray, dTotal1, dTotal2, dDifference;
volatile unsigned short *pusTaskCheckVariable;
const unsigned short usArraySize = 250;                                                                                                                                                        
unsigned short usPosition;
short sError = pdFALSE;
                      
        /* Queue a message for printing to say the task has started. */
        /* The variable this task increments to show it is still running is passed in 
        as the parameter. */
        pusTaskCheckVariable = ( unsigned short * ) pvParameters;
                      
        pdArray = ( portDOUBLE * ) pvPortMalloc( ( size_t ) 250 * sizeof( portDOUBLE ) );
                      
        /* Keep filling an array, keeping a running total of the values placed in the 
        array.  Then run through the array adding up all the values.  If the two totals 
        do not match, stop the check variable from incrementing. */
        for( ;; )     
        {             
                dTotal1 = 0.0;
                dTotal2 = 0.0;
                      
                for( usPosition = 0; usPosition < usArraySize; usPosition++ )
                {     
                        pdArray[ usPosition ] = ( portDOUBLE ) usPosition * 12.123;
                        dTotal1 += ( portDOUBLE ) usPosition * 12.123;  
                }     
                      
                taskYIELD();
                      
                for( usPosition = 0; usPosition < usArraySize; usPosition++ )
                {     
                        dTotal2 += pdArray[ usPosition ];
                }     
                      
                dDifference = dTotal1 - dTotal2;
                if( fabs( dDifference ) > 0.001 )
                {    
			SMP_DBG("cal error.\n");
			soft_break();
                }    
         
                if( sError == pdFALSE )
                {    
                        /* If the calculation has always been correct, increment the check 
                        variable so we know     this task is still running okay. */
                        ( *pusTaskCheckVariable )++;
                }

                if( (*pusTaskCheckVariable ) % 256 == 0)
		{
		    	uint32_t cpu_sr;
		    	cpu_sr = freert_cpus_lock();
			SMP_DBG("success.\n");
		    	freert_cpus_unlock(cpu_sr);
		}
                taskYIELD();
        }

	vTaskDelete(NULL);
   	return;
} 


void cpu0_app_main(void *para)
{
	portBASE_TYPE ret;
	uint32_t uxPriority = 3;

	xTaskCreateOnCore( vCompetingMathTask1, "Math1", mathSTACK_SIZE, ( void * ) &( usTaskCheck[ 0 ] ), uxPriority, NULL, 1);
	xTaskCreateOnCore( vCompetingMathTask2, "Math2", mathSTACK_SIZE, ( void * ) &( usTaskCheck[ 1 ] ), uxPriority, NULL, 1);
	xTaskCreateOnCore( vCompetingMathTask3, "Math3", mathSTACK_SIZE, ( void * ) &( usTaskCheck[ 2 ] ), uxPriority, NULL, 1);
	xTaskCreateOnCore( vCompetingMathTask4, "Math4", mathSTACK_SIZE, ( void * ) &( usTaskCheck[ 3 ] ), uxPriority, NULL, 1);
	xTaskCreateOnCore( vCompetingMathTask1, "Math5", mathSTACK_SIZE, ( void * ) &( usTaskCheck[ 4 ] ), uxPriority, NULL, 0);
	xTaskCreateOnCore( vCompetingMathTask2, "Math6", mathSTACK_SIZE, ( void * ) &( usTaskCheck[ 5 ] ), uxPriority, NULL, 0);
	xTaskCreateOnCore( vCompetingMathTask3, "Math7", mathSTACK_SIZE, ( void * ) &( usTaskCheck[ 6 ] ), uxPriority, NULL, 0);
	xTaskCreateOnCore( vCompetingMathTask4, "Math8", mathSTACK_SIZE, ( void * ) &( usTaskCheck[ 7 ] ), uxPriority, NULL, 0);

	vTaskDelete(NULL);
   	return;
}
