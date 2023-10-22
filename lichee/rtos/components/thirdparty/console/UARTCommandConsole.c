/* Standard includes. */
#include "string.h"
#include "stdio.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


/* Example includes. */
#include "FreeRTOS_CLI.h"
#include "UARTCommandConsole.h"
#include <hal_uart.h>

#include <console.h>
#include <cli_console.h>

/* Dimensions the buffer into which input characters are placed. */
#define cmdMAX_INPUT_SIZE		SH_MAX_CMD_LEN

/* The maximum time in ticks to wait for the UART access mutex. */
#define cmdMAX_MUTEX_WAIT		( 200 / portTICK_PERIOD_MS )

/* Characters are only ever received slowly on the CLI so it is ok to pass
received characters from the UART interrupt to the task on a queue.  This sets
the length of the queue used for that purpose. */
#define cmdRXED_CHARS_QUEUE_LENGTH			( 10 )

/*-----------------------------------------------------------*/

extern void vRegisterSampleCLICommands(void);
/*
 * The task that implements the command console processing.
 */
void prvUARTCommandConsoleTask( void *pvParameters );

/*
 * Ensure a previous interrupt driven Tx has completed before sending the next
 * data block to the UART.
 */
static void prvSendBuffer( const char * pcBuffer, size_t xBufferLength );

/*
 * A UART is used for printf() output and CLI input and output.  Configure the
 * UART and register prvUARTRxNotificationHandler() to handle UART Rx events.
 */
static void prvConfigureUART( cli_console * console );
//static void prvUARTRxNotificationHandler( mss_uart_instance_t * this_uart );

/*-----------------------------------------------------------*/

/* Const messages output by the command console. */
static const char * const pcWelcomeMessage = "\r\n\r\nFreeRTOS command server.\r\nType Help to view a list of registered commands.\r\n\r\n>";
/* static const char * const pcEndOfOutputMessage_cpu0 = "\r\n[Press ENTER to execute the previous command again]\r\ncpu0>";*/
/* static const char * const pcEndOfOutputMessage_cpu1 = "\r\n[Press ENTER to execute the previous command again]\r\ncpu1>";*/
#if defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_ARCH_ARM_CORTEX_M33)
static const char * const pcEndOfOutputMessage_cpu0 = "\ncm33>";
#elif defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_ARCH_RISCV_C906)
static const char * const pcEndOfOutputMessage_cpu0 = "\nc906>";
#else
static const char * const pcEndOfOutputMessage_cpu0 = "\ncpu0>";
static const char * const pcEndOfOutputMessage_cpu1 = "\ncpu1>";
#endif

static const char * const pcNewLine = "\r\n";

/* Because characters are received slowly (at the speed somebody can type) then
it is ok to pass received characters from the Rx interrupt to the task on a
queue.  This is the queue used for that purpose. */

/*-----------------------------------------------------------*/

void vCommandConsoleStart( uint16_t usStackSize, portBASE_TYPE uxPriority, void * console)
{
#if !defined(CONFIG_DISABLE_ALL_UART_LOG)
	/* A UART is used for printf() output and CLI input and output.  Note there
	is no mutual exclusion on the UART, but the demo as it stands does not
	require mutual exclusion. */
    TaskHandle_t cli_task = NULL;

	prvConfigureUART(console);

	vRegisterSampleCLICommands();
	portBASE_TYPE ret;
	/* Create that task that handles the console itself. */
	ret = xTaskCreate(prvUARTCommandConsoleTask,	/* The task that implements the command console. */
					"CLI",				/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
					usStackSize,				/* The size of the stack allocated to the task. */
					NULL,						/* The parameter is not used, so NULL is passed. */
					uxPriority,					/* The priority allocated to the task. */
					&cli_task);						/* A handle is not required, so just pass NULL. */
	if (ret != pdPASS) {
	    printf("Error creating console task, status was %d\n", ret);
	}

#ifdef CONFIG_COMPONENTS_MULTI_CONSOLE
    cli_task_set_console(cli_task, console);
#endif
#endif
}

/*-----------------------------------------------------------*/

void prvUARTCommandConsoleTask( void *pvParameters )
{
char cRxedChar, *pcOutputString;
unsigned int cInputIndex = 0;
char cInputString[ cmdMAX_INPUT_SIZE ], cLastInputString[ cmdMAX_INPUT_SIZE ];
portBASE_TYPE xReturned;

	( void ) pvParameters;

	/* Obtain the address of the output buffer.  Note there is no mutual
	exclusion on this buffer as it is assumed only one command console
	interface will be used at any one time. */
	pcOutputString = FreeRTOS_CLIGetOutputBuffer();

	/* Send the welcome message. */
	prvSendBuffer( pcWelcomeMessage, strlen( pcWelcomeMessage ) );

	for( ;; )
	{
#ifdef CONFIG_COMPONENTS_MULTI_CONSOLE
		check_console_task_exit();
#endif
		/* Wait for the next character to arrive. */
		// if( xQueueReceive( xRxedChars, &cRxedChar, portMAX_DELAY ) == pdPASS )
#ifdef CONFIG_PANIC_CLI
		extern int g_cli_direct_read;
		if (g_cli_direct_read)
			hal_uart_receive_polling(CONFIG_CLI_UART_PORT, (uint8_t *)&cRxedChar, 1);
		else
			cRxedChar = getchar();
#else
		cRxedChar = getchar();
#endif
		{
			/* Echo the character back. */
			prvSendBuffer( &cRxedChar, sizeof( cRxedChar ) );

			/* Was it the end of the line? */
			if( cRxedChar == '\n' || cRxedChar == '\r' )
			{
				/* Just to space the output from the input. */
				prvSendBuffer( pcNewLine, strlen( pcNewLine ) );
				cInputString[cInputIndex] = '\0';

				/* See if the command is empty, indicating that the last command is
				to be executed again. */
				if( cInputIndex == 0 )
				{
					/* Copy the last command back into the input string. */
					/* strcpy( cInputString, cLastInputString ); */
					memset( cInputString, 0x00, cmdMAX_INPUT_SIZE );
#ifdef CONFIG_SMP
					int cur_cpu_id(void);
					if(cur_cpu_id() == 0)
						prvSendBuffer( pcEndOfOutputMessage_cpu0, strlen( pcEndOfOutputMessage_cpu0) );
					else if(cur_cpu_id() == 1)
						prvSendBuffer( pcEndOfOutputMessage_cpu1, strlen( pcEndOfOutputMessage_cpu1) );
#else
					prvSendBuffer( pcEndOfOutputMessage_cpu0, strlen( pcEndOfOutputMessage_cpu0) );
#endif
					continue;

				}

				/* Pass the received command to the command interpreter.  The
				command interpreter is called repeatedly until it returns pdFALSE
				(indicating there is no more output) as it might generate more than
				one string. */
				do
				{
					/* Get the next output string from the command interpreter. */
					xReturned = FreeRTOS_CLIProcessCommand( cInputString, pcOutputString, configCOMMAND_INT_MAX_OUTPUT_SIZE );

					/* Write the generated string to the UART. */
					prvSendBuffer( pcOutputString, strlen( pcOutputString ) );

				} while( xReturned != pdFALSE );

				/* All the strings generated by the input command have been sent.
				Clear the input	string ready to receive the next command.  Remember
				the command that was just processed first in case it is to be
				processed again. */
				strcpy( cLastInputString, cInputString );
				cInputIndex = 0;
				memset( cInputString, 0x00, cmdMAX_INPUT_SIZE );

				/*  do not need pcEndOfOutputMessage  */
#ifdef CONFIG_SMP
				int cur_cpu_id(void);
				if(cur_cpu_id() == 0)
					prvSendBuffer( pcEndOfOutputMessage_cpu0, strlen( pcEndOfOutputMessage_cpu0) );
				else if(cur_cpu_id() == 1)
					prvSendBuffer( pcEndOfOutputMessage_cpu1, strlen( pcEndOfOutputMessage_cpu1) );
#else
				prvSendBuffer( pcEndOfOutputMessage_cpu0, strlen( pcEndOfOutputMessage_cpu0) );
#endif
			}
			else
			{
				if( cRxedChar == '\r' )
				{
					/* Ignore the character. */
				}
				else if( cRxedChar == '\b' )
				{
					/* Backspace was pressed.  Erase the last character in the
					string - if any. */
					if( cInputIndex > 0 )
					{
						cInputIndex--;
						cInputString[ cInputIndex ] = '\0';
					}
				}
				else
				{
					/* A character was entered.  Add it to the string
					entered so far.  When a \n is entered the complete
					string will be passed to the command interpreter. */
					if( ( cRxedChar >= ' ' ) && ( cRxedChar <= '~' ) )
					{
						if( cInputIndex < cmdMAX_INPUT_SIZE - 1)
						{
							cInputString[ cInputIndex ] = cRxedChar;
							cInputIndex++;
						}
					}
				}
			}
		}
	}
}
/*-----------------------------------------------------------*/

static void prvSendBuffer( const char * pcBuffer, size_t xBufferLength )
{
#ifdef CONFIG_COMPONENTS_MULTI_CONSOLE
    cli_console_write(NULL, pcBuffer, xBufferLength);
#else
#if !defined(CONFIG_DISABLE_ALL_UART_LOG) && defined(CONFIG_DRIVERS_UART)
    char *b = (char *)pcBuffer;
    int i;
    for (i = 0; i < xBufferLength; i ++)
    {
        if (*(b + i) == '\n')
        {
            hal_uart_send(CONFIG_CLI_UART_PORT, "\r", 1);
        }
        hal_uart_send(CONFIG_CLI_UART_PORT, b + i, 1);
    }
#endif
#endif
}

static void prvConfigureUART(cli_console * console)
{
}
