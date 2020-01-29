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
 *   Options:
 *     -d device_name             Specify device to use.
 *     -v                         Verbose. Enable debug output.
 *     -?                         Print usage
 *
 *   Commands:
 *     -u                         List USB devices
 *     -i request                 Query infos
 *     -i ALL                     Query all infos
 *     -l                         List supported actions
 *     -q action [-p param ... ]  Query action
 *     -s action [-p param ... ]  Set action
 *     -c action                  Query action config
 *
 *   In case no command is specified all supported actions are queried.
 *
 * Examples
 * ========
 *
 *   $ usbget -d redbear_duo -l
 *   TPMS
 *   OIL
 *   WS2801
 *
 *   $ usbget -d redbear_duo -q TPMS -q OLI
 *
 *   $ usbget -d redbear_duo -c TPMS
 *   FL=80EACA100326
 *   FR=81EACA2002C0
 *   RL=82EACA300633
 *   RR=83EACA400619
 *   csum=1696
 *
 *   $ usbget -d redbear_duo -s WS2801 -p R=10 -p G=10 -p B=10
 *
 *   $ usbget -d redbear_duo -i ALL
 *   VERSION=0.1.1
 *
 *
 * NOTES
 * =====
 *   For a description of the line protocol see protocol.h
 *
 *
 * TODOs
 * =====
 *
 *
 * File History
 * ============
 *   wolfix      26-Jan-2020  (0.1.2) CH240 support
 *   wolfix      21-Jul-2019  USB device open code
 *   wolfix      04-Jul-2019  Code refactoring
 *   wolfix      25-Jun-2019  Fix getopt hang (signed/unsiged mismatch)
 *                            3 sec command timeout.
 *   wolfix                   Obey lock file
 *   wolfix                   Support set command
 *   wolfix                   Support command line options
 *   wolfix      13-Jun-2019  Minor fixes and code cleanup
 *   wolfix      12-Jun-2019  Initial version
 *
 */

#include "support.h"
#include "usb.h"
#include "protocol.h"

#include <unistd.h>


static const char* VERSION = "0.1.2";


#define MAX_DEVICENAME_LEN  20
static char deviceName[MAX_DEVICENAME_LEN];
static usbDevice *device = NULL;

/* Baud rate for Micros that are attached via FTDI or similar chip */
#define USB_SPEED             ((uint32_t)19200)

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
typedef enum RunOption {
    QUIT = 0,
    ERROR,
    INFO,
    LIST,
    QUERY,
    SET,
    CONFIG
} RunOption;

#define MAX_OPTION_ACTION_LEN 20
static char optionAction[MAX_OPTION_ACTION_LEN];


/******************* static forward declarations *******************/

static void parseOptions( int argc, char **argv);
static RunOption parseArguments( int argc, char **argv);
static void usage();

static void queryActionList();
static void queryInfo( const char *action);
static void queryAction( const char *action);
static void setAction( const char *action);
static void queryConfig( const char *action);

static void runCommand( ProtocolChar cmd,
                        const char *action,
                        const char *debugComment,
                        boolean print);

/*******************************************************************/

int main( int argc, char **argv)
{
    boolean nothingToDo = TRUE;
    RunOption runOption;

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

    device = usbOpen( deviceName, USB_SPEED);
    if( !device) {
        releaseLock();
        exit(-1);
    }

    usbDrainInput( device);

    while( (runOption = parseArguments( argc, argv)))
    {
        usbResetBuffers( device);

        if( runOption == INFO) {
            nothingToDo = FALSE;
            queryInfo( optionAction);

        } else if( runOption == LIST) {
            nothingToDo = FALSE;
            queryActionList();
            for( int action=0; action < actionCount; action++) {
                printf("%s\n",actions[action]);
            }

        } else if( runOption == QUERY) {
            nothingToDo = FALSE;
            queryAction( optionAction);

        } else if( runOption == SET) {
            nothingToDo = FALSE;
            setAction( optionAction);

        } else if( runOption == CONFIG) {
            nothingToDo = FALSE;
            queryConfig( optionAction);

        } else if( runOption == QUIT || runOption == ERROR) {
            break;
        }
    }

    if( runOption == ERROR) {
        printfDebug( "Exit with error.\n");

    } else if( nothingToDo) {
        printfDebug( "Nothing to do, Querying all actions.\n");

        queryActionList();
        for( int action=0; action < actionCount; action++) {
            queryAction( actions[action]);
        }
    }

    usbClose( &device);

    releaseLock();
}

/* Parse options.
 * This function only honors option parameters like -d -v -?.
 * Action parameters are parsed and processed later by
 * parseArguments().
 */
static void parseOptions( int argc, char **argv)
{
    int opt;

    deviceName[0] = '\0';

#define ALL_GETOPTS "vd:ulc:i:q:s:p:?"

    while((opt = getopt(argc, argv, ALL_GETOPTS)) != -1) {
        if( (char)opt ==  'v') {
            setDebugStream( stdout);

        } else if( (char)opt == 'd') {
            SAFE_STRNCPY( deviceName, optarg, MAX_DEVICENAME_LEN);

        } else if( (char)opt == 'u') {
            usbList();
            exit(0);

        } else if( (char)opt == '?') {
            usage();
            exit(0);
        }
    }

    optind = 1;
}

/* Parse action parameters.
 *
 * This function can be called multiple times and returns
 * whenever there is enough information to execute the next action.
 */
static RunOption parseArguments( int argc, char **argv)
{
    int state = 0;
    int opt;
    RunOption runOption = QUIT;

#define CHECK_STATE( b, o) \
    if( state == 1) {      \
        optind -= b;       \
        break;             \
    }                      \
    state++;               \
    runOption = o

    parameterCount = 0;

    while((opt = getopt(argc, argv, ALL_GETOPTS)) != -1) {

        /* This cannot be a switch() because the CHECK_STATE macro
         * calls break to leave the loop.
         */
        if((char)opt == 'l') {
            CHECK_STATE( 1, LIST);

        } else if((char)opt == 'i') {
            CHECK_STATE( 1, INFO);
            SAFE_STRNCPY( optionAction, optarg, MAX_OPTION_ACTION_LEN);

        } else if( (char)opt == 'q') {
            CHECK_STATE(2, QUERY);
            SAFE_STRNCPY( optionAction, optarg, MAX_OPTION_ACTION_LEN);

        } else if( (char)opt == 's') {
            CHECK_STATE(2, SET);
            SAFE_STRNCPY( optionAction, optarg, MAX_OPTION_ACTION_LEN);

        } else if( (char)opt == 'c') {
            CHECK_STATE(2, CONFIG);
            SAFE_STRNCPY( optionAction, optarg, MAX_OPTION_ACTION_LEN);

        } else if( (char)opt == 'p') {
            if( state == 0) {
                printfLog( "No action for parameter: %s\n", optarg);
                runOption = ERROR;
                break;
            }

            if( parameterCount >= MAX_PARAMETERS) {
                printfLog( "To many parameters: %s\n", optarg);
                runOption = ERROR;
                break;
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

    printf("\nusbget %s\n\n", VERSION);
    printf(" usage: usbget [options] [command ... ] \n\n");
    printf("   Options:\n");
    printf("     -d device_name             USB device type\n");
    printf("        Valid device names:\n");
    while( (name = usbEnumDeviceNames( &idx))) {
        printf("          %s\n", name);
    }
    printf("     -v                         Enable debug output\n");
    printf("     -?                         Print usage\n\n");
    printf("   Commands:\n");
    printf("     -u                         List USB devices\n");
    printf("     -i request                 Query infos\n");
    printf("     -i ALL                     Query all infos\n");
    printf("     -l                         List supported actions\n");
    printf("     -q action [-p param ... ]  Query action\n");
    printf("     -s action [-p param ... ]  Set action\n");
    printf("     -c action                  Query action config\n\n");
    printf("   In case no command is specified all supported actions"
           " are queried.\n\n");
}

/* Query the device for supported actions.
 */
static void queryActionList()
{
    runCommand( LIST_ACTIONS, NULL, "queryActionList()\n", FALSE);
}

/* Run actions and collect the result.
 */
static void queryAction( const char *action)
{
    runCommand( QUERY_ACTION, action, "queryAction()\n", FALSE);
}

/* setAction is like query but do not expect (print) a result.
 */
static void setAction( const char *action)
{
    runCommand( SET_ACTION, action, "setAction()\n", FALSE);
}

/* Query device for version etc...
 */
static void queryInfo( const char *action)
{
    runCommand( INFO_COMMAND, action, "queryInfo()\n", TRUE);
}

/* Query device for action configuration.
 */
static void queryConfig( const char *action)
{
    runCommand( QUERY_CONFIG, action, "queryConfig()\n", TRUE);
}

/* Run any command and optionally collect the response.
 *
 * @TODO should return an error.
 */
static void runCommand( ProtocolChar cmd,
                        const char *action,
                        const char *debugComment,
                        boolean print)
{
    char *line;
    ProtocolChar commandChar;
    FILE *fp = NULL;

    printfDebug( debugComment);

    if( cmd == LIST_ACTIONS) {
        actionCount = 0;
        for( int i=0; i<MAX_ACTIONS; i++) {
            actions[i] = NULL;
        }
    }

    sendCommand( device, cmd, action);
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
        else if( isMoreData( commandChar)) {

            if( cmd == LIST_ACTIONS) {

                if( actionCount >= MAX_ACTIONS) {
                    printfLog( "Too many Actions: skipping %s\n", line);
                    continue;
                }

                if( strlen(line) < MAX_ACTION_NAME_LEN) {
                    actions[actionCount] = (char*)calloc( 1, strlen(line)+1);
                    strcpy( actions[actionCount], line);
                    actionCount++;
                } else {
                    printfLog( "Action name exceeds length limit of %d: %s\n",
                        MAX_ACTION_NAME_LEN, line);
                }

            }
            else if( cmd == QUERY_ACTION) {

                if( fp == NULL) {
                    fp = openFile( action, OUTPUT_EXT, "w");
                }

                if (fp != NULL) {
                    fprintf(fp, "%s\n", line);
                }

            }
            else {
                if( print) {
                    printf("%s\n", line);
                }
            }
        }

        line = receiveLine( device, &commandChar);
    }

    if( fp != NULL) {
        fclose( fp);
    }
}

/*******************************************************************/

