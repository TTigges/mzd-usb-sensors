/*
 * action.ino
 * 
 * The superclass of all actions.
 * 
 */

Action *actionList[MAX_ACTIONS];

unsigned int actionIdx = 0;

/* Call this once for every action to add from Arduino setup().
 */
void addAction( Action *action) {

  if( actionIdx < MAX_ACTIONS) {
    actionList[actionIdx++] = action;
  }
}

/* Call this once from Arduino setup() after all actions have been added.
 * This will initialize all action instances.
 */
void setupActions()
{
  unsigned int i;
  unsigned int eepromLocation = 0;

  for( i=0; i<actionIdx; i++) {
    /* We pass the EEPROM location where the action instance may save
     * its config data.
     */
    eepromLocation += actionList[i]->setup( eepromLocation);
  }
}

void runTimeout()
{
  unsigned int i;

  for( i=0; i<actionIdx; i++) {
    actionList[i]->timeout();
  }
}

/* Sends a list of supported actions.
 */
void listFunctions()
{
  unsigned int i;

  for( i=0; i<actionIdx; i++) {
    sendMoreData(actionList[i]->getName());
  }

  sendEOT();
}

/* Send the configuration data of a named action.
 */
void queryConfig(char aName[])
{
  Action *a;
  
  a = mapToFunction( aName);

  if( a) {
    a->sendConfig();
  }
  else {
    flagError(ERROR_UNKNOWN_ACTION);
  }

  sendEOT();
}

/* Set a variable or execute a named action.
 */
void setFunction(char aName[])
{
  Action *a;
  
  a = mapToFunction( aName);

  if( a) {
    a->setConfig();
  }
  else {
    flagError(ERROR_UNKNOWN_ACTION);
  }

  sendEOT();
}

/* Query action data.
 * This also calls getData() to fetch sensor data.
 */
void queryFunction(char aName[])
{
  Action *a;
  
  a = mapToFunction( aName);

  if( a) {
    a->getData();
    a->sendData();
  }
  else {
    flagError(ERROR_UNKNOWN_ACTION);
  }

  sendEOT();
}

/* Map the action name to an action instance.
 * If there is no such action return NULL.
 */
Action *mapToFunction( char aName[])
{
  Action *a = NULL;
  unsigned int i;

  for( i=0; i<actionIdx; i++) {
    if( strcmp( aName, actionList[i]->getName()) == 0) {
      a = actionList[i];
      break;
    }
  }

  return a;
}
