/*
 * Transparently fetch and push data from/to MZD to/from USB device.
 * 
 * When run the program connects to a micro controller connected to
 * the USB bus, fetches data and stores the result in a file in 
 * /tmp/mnt/data_persist/dev/bin
 * 
 * The filename is the action name converted to lower case with 
 * extension ".out".
 * 
 * 
 * Command line options
 * ====================
 * 
 * usage: usbget [options] [command ... ]
 * 
 *	 Options:
 *     -d device_name             Specify device to use.
 *     -v                         Verbose. Enable debug output.
 *     -?                         Print usage
 * 
 *   Commands:
 *     -l                         List supported actions
 *     -q action [-p param ... ]  Query action
 *     -s action [-p param ... ]  Set action
 * 
 *   In case no command is specified all supported actions are queried.
 * 
 * 
 * Line Protocol:
 * ==============
 * 
 * The protocol supports the following functions:
 * 
 *   + Query action list
 *       The micro controller supports a list of actions to be 
 *       performed on behalf of the MZD. The list may vary depending
 *       on the software release running on the micro controller.
 *       This function queries the list of supported actions.
 * 
 *   + Query action
 *       Execute an action on the micro controller and query the
 *       result state.
 * 
 *   + Set variable
 *       Modify the value of a variable on the micro controler or
 *       perform an action without fetching any results.
 * 
 * The protocol is character based (No binary data).
 * Every command starts with a single command character.
 * Every command is terminated by a newline character.
 * 
 * Supported command characters:
 * =============================
 *	 L     Query action list
 *   Q     Execute and query action
 *   S     Set variable
 *   +     Additional data
 *   .     End of transfer
 *   /     NACK or error response
 *   <nl>  Newline
 *
 * Examples
 * ========
 * 
 *      MZD         Transfer direction        Micro controller
 * ----------------------------------------------------------------
 *
 * Query action list
 * -----------------
 * L                         =>
 * .                         =>
 *                          <=             +action1
 *                          <=             +action2
 *                          <=             .
 *
 * Execute and query action
 * ------------------------
 * Qaction1                  =>
 * .                         =>
 *                          <=             +result line 1
 *                          <=             +result line 2
 *                          <=             .
 *
 * Execute and query action with optional parameter
 * ------------------------------------------------
 * Qaction1                  =>
 * +variable1=value1         =>
 * .                         =>
 *                          <=             +result line 1
 *                          <=             +result line 2
 *                          <=             .
 *
 * Variablen setzen
 * ----------------
 * Saction1                  =>
 * +variable1=value1         =>
 * .                         =>
 *                          <=             .
 * 
 * In case of an error
 * -------------------
 * Qblabla                   =>
 * .                         =>
 *                          <=             /Unknown function.
 * 
 * 
 * Credits
 * =======
 *   usbOpen() code based on mazda_tpms.c from Mazdaracerdude.
 * 
 * 
 * TODOs
 * =====
 * 
 * 
 * File History
 * ============
 *   wolfix      25-Jun-2019  Fix getopt hang (signed/unsiged mismatch)
 *                            3 sec command timeout.
 *   wolfix                   Obey lock file
 *   wolfix                   Support set command
 *   wolfix                   Support command line options
 *   wolfix      13-Jun-2019  Minor fixes and code cleanup
 *   wolfix      12-Jun-2019  Initial version
 * 
 */

#include <support.h>
 
#include <unistd.h>

#include <usb.h>
#include <protocol.h>

#define MAX_DEVICENAME_LEN  20
static char deviceName[MAX_DEVICENAME_LEN];
static usbDevice *device = NULL;

/* @TODO current limit of 10 actions */
#define MAX_ACTIONS           10
#define MAX_ACTION_NAME_LEN   20
static char *actions[MAX_ACTIONS];
static int actionCount;

/* @TODO current limit of 10 parameters per action */
#define MAX_PARAMETERS 10
static char *parameters[MAX_PARAMETERS];
static int parameterCount;

/* Run options selected via command line parameters */
enum RunOption {
    QUIT = 0,
    ERROR,
	LIST,
	QUERY,
	SET
};

#define MAX_OPTION_ACTION_SIZE 20
static char optionAction[MAX_OPTION_ACTION_SIZE];

/******************* static forward declarations *******************/

static void parseOptions( int argc, char **argv);
static enum RunOption parseArguments( int argc, char **argv);
static void usage();

static void listActions();
static void printActions();
static void queryAction( const char *action);
static void setAction( const char *action);

/*******************************************************************/

int main( int argc, char **argv)
{
	boolean nothingToDo = TRUE;
	enum RunOption runOption;

	parseOptions( argc, argv);
		
	if( strlen( deviceName) == 0) {
		printfLog( "No device specified. "
			       "Use -d option with supported device argument.\n");
		exit(-1);
	}
	
	/* Make sure only one instance accesses the USB port.
	 * Multiple transfers in parallel would fail because the 
	 * port can only be opened by a single process.
	 */
	if( acquireLock() != RC_OK) {
		printfLog( "Failed to acquire lock.\n");
		exit(-1);
	}
	
	device = usbOpen( deviceName);
	if( !device) {
		releaseLock();
		exit(-1);
	}

	usbDrainInput( device);

	while( (runOption = parseArguments( argc, argv)))
	{
		usbResetBuffers( device);

		if( runOption == LIST) {
			nothingToDo = FALSE;
			listActions();
			printActions();

		} else if( runOption == QUERY) {
			nothingToDo = FALSE;
			queryAction( optionAction);

		} else if( runOption == SET) {
			nothingToDo = FALSE;
			setAction( optionAction);
			
		} else if( runOption == QUIT || runOption == ERROR) {
			break;
		}
		
		parameterCount = 0;
	}

	if( runOption == ERROR) {
		printfDebug( "Exit with error.\n");

	} else if( nothingToDo) {
		printfDebug( "Nothing to do, Querying all actions.\n");
		
		listActions();
		for( int action=0; action < actionCount; action++) {
			queryAction( actions[action]);
		}		
	}

	usbClose( &device);
	
	releaseLock();
}

static void parseOptions( int argc, char **argv)
{
	int opt;
	
	deviceName[0] = '\0';
	
#define ALL_GETOPTS "lq:s:p:d:v?"

	while((opt = getopt(argc, argv, ALL_GETOPTS)) != -1) {
		if( (char)opt ==  'v') {
			setDebugStream( stdout);

	    } else if( (char)opt == 'd') {
			strncpy( deviceName, optarg, MAX_DEVICENAME_LEN);

	    } else if( (char)opt == '?') {
			usage();
			exit(0);
        }
	}
	
	optind = 1;
}

static enum RunOption parseArguments( int argc, char **argv)
{
	int state = 0;
	
#define CHECK_STATE( b) \
	if( state == 1) {   \
		optind -= b;    \
		break;          \
	}                   \
	state++
	
	int opt;
	enum RunOption runOption = QUIT;
	
	while((opt = getopt(argc, argv, ALL_GETOPTS)) != -1) {  

		/* This cannot be a switch() because the CHECK_STATE macro
		 * calls break to leave the loop.
		 */
        if((char)opt == 'l') {
			CHECK_STATE( 1);
			runOption = LIST;

		} else if( (char)opt == 'q') {
			CHECK_STATE(2);
			runOption = QUERY;
			strncpy( optionAction, optarg, MAX_OPTION_ACTION_SIZE);
			
		} else if( (char)opt == 's') {
			CHECK_STATE(2);
			runOption = SET;
			strncpy( optionAction, optarg, MAX_OPTION_ACTION_SIZE);

		} else if( (char)opt == 'p') {
			if( state == 0) {
				printfLog( "No action for parameter: %s\n", optarg);
				runOption = ERROR;
				break;
			}

			if( parameterCount >= MAX_PARAMETERS-1) {
				printfLog( "To many parameters: %s\n", optarg);
				continue;
			}
			
			parameters[parameterCount++] = optarg;
		}
    }

    return runOption;
}

static void usage()
{	
	unsigned int idx = 0;
	const char *name;
	
	printf("\nusage: usbget [options] [command ... ] \n\n");
	printf("  Options:\n");	
	printf("    -d device_name             USB device type\n");
	
	printf("       Valid device names:\n");
	while( (name = usbEnumDeviceNames( &idx))) {
		printf("         %s\n", name);
	}
	printf("    -v                         Enable debug output\n");
	printf("    -?                         Print usage\n\n");
	printf("  Commands:\n");
	printf("    -l                         List supported actions\n");
	printf("    -q action [-p param ... ]  Query action\n");
	printf("    -s action [-p param ... ]  Set action\n\n");
	printf("  In case no command is specified all supported actions"
	        " are queried.\n\n");
}

/* Query the device for supported actions.
 */
static void listActions()
{
	char *line;
	char commandChar;
	
	printfDebug( "ListActions()\n");

	actionCount = 0;
	for( int i=0; i<MAX_ACTIONS; i++) {
		actions[i] = NULL;
	}
	
	sendCommand( device, LIST_ACTIONS, NULL);
	for( int i=0; i<parameterCount; i++) {
		sendMoreData( device, parameters[i]);
	}
	sendEOT( device);
	
	line = receiveLine( device, &commandChar);
	
	while( !isNoCommand( commandChar)) {
		
		if( isEOT( commandChar)) {
			 break;
		}
		else if( isMoreData( commandChar)) {
			
			if( actionCount >= MAX_ACTIONS) { continue; }
			
			if( strlen(line) < MAX_ACTION_NAME_LEN) {
				actions[actionCount] = (char*)calloc( 1, strlen(line)+1);
				strcpy( actions[actionCount], line);
				actionCount++;
			} else {
				printfLog( "Action name exceeds length limit of %d: %s\n",
					MAX_ACTION_NAME_LEN, line);
			}
		}
		else if( isNACK( commandChar)) {
			printfLog( "Error from USB device: %s\n", line);
			break;
		}

		line = receiveLine( device, &commandChar);
	}
}

static void printActions()
{
	for( int action=0; action < actionCount; action++) {
		printf("%s\n",actions[action]);
	}	
}

/* Run actions and collect the result.
 */
static void queryAction( const char *action)
{
	char *line;
	char commandChar;
	FILE *fp = NULL;

	printfDebug( "QueryAction()\n");

	sendCommand( device, QUERY_ACTION, action);
	for( int i=0; i<parameterCount; i++) {
		sendMoreData( device, parameters[i]);
	}
	sendEOT( device);
	
	line = receiveLine( device, &commandChar);
	
	while( !isNoCommand( commandChar)) {
		
		if( isEOT( commandChar)) {
			 break;
		}
		else if( isMoreData( commandChar)) {
			if( fp == NULL) {
				fp = openFileForWrite( action, OUTPUT_EXT);
			}
			
			if (fp != NULL)	{
				fprintf(fp, "%s\n", line);
			}
		}
		else if( isNACK( commandChar)) {
			printfLog( "Error from USB device: %s\n", line);
			break;
		}
		
		line = receiveLine( device, &commandChar);
	}
	
	if( fp != NULL) {
		fclose( fp);
	}
}

/* Run actions but do not expect a result.
 */
static void setAction( const char *action)
{
	char *line;
	char commandChar;

	printfDebug( "SetAction()\n");

	sendCommand( device, SET_ACTION, action);
	for( int i=0; i<parameterCount; i++) {
		sendMoreData( device, parameters[i]);
	}
	sendEOT( device);
	
	line = receiveLine( device, &commandChar);
	
	while( !isNoCommand( commandChar)) {
		
		if( isEOT( commandChar)) {
			 break;
		}
		else if( isNACK( commandChar)) {
			printfLog( "Error from USB device: %s\n", line);
			break;
		}
		
		line = receiveLine( device, &commandChar);
	}
}

/*******************************************************************/

