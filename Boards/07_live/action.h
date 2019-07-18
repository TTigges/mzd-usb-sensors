/*
 * action.h
 * 
 * Superclass of all actions.
 * 
 */

/* Disable simulation to get real data */
boolean actionSimulate = false;

#define MAX_ACTIONS 10

/* This is the superclass of all actions.
 * Every action has to implement below 6 methods.
 * 
 * Only getName() and setup() are mandatory.
 * The other methody may be optional depending on the requirements.
 */
class Action {

  public: 
    /* Returns the actions name.
     * This is the name by which the action is invoked 
     * via Q command.
     */
    virtual const char *getName() = 0;

    /* Called once in the global setup() function */
    virtual size_t setup(unsigned int eepromLocation) = 0;

    /* Called to read attached sensors data before sendData() is called */
    virtual void getData() = 0;
    
    /* Called to send the actions sensor data */
    virtual void sendData() = 0;
    
    /* Called to send the actions configuration data */
    virtual void sendConfig() = 0;
    
    /* Called to set the actions configuration */
    virtual void setConfig() = 0;
};
