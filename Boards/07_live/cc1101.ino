/*
 * cc1101.ino
 * 
 * 433 Mhz transmitter/receiver support for tpms_433
 * 
 */

#ifdef CC1101_SUPPORT

SPIClass spi;

/*
 * Receive data from CC1101.
 * This version of the code is interrupt driven.
 * 
 * 
 * Receive state machine:
 * ======================
 * 
 *     +------------ CDintr ---------+
 *     |         (carrier lost)      |
 *     V                             |
 *   IDLE >------ CDintr ----> CARRIER_DETECTED
 *     ^    (carrier detected)       |
 *     |                             |
 *     |                             |
 *   loop()                       EdgeIntr
 *  processing                       |
 *     |                             |
 *     |                             V
 *   DATA <------ CDintr ------< RECEIVING >--+
 * AVAILABLE   (carrier lost)        ^        |
 *                                   |     EdgeIntr
 *                                   |        |
 *                                   +--------+
 * 
 */
#define STATE_IDLE               0
#define STATE_CARRIER_DETECTED   1
#define STATE_RECEIVING          2
#define STATE_DATA_AVAILABLE     3

static volatile byte receiver_state;

static volatile bool first_edge_state = LOW;

#define CC1101_MAX_TIMINGS   255
volatile byte timings[CC1101_MAX_TIMINGS];
volatile byte timings_len = 0;

volatile static unsigned long LastEdgeTime_us = 0;

/* Range of valid carrier length */
#define CARRIER_MIN_LEN_usec    9600
#define CARRIER_MAX_LEN_usec   10500
unsigned long carrier_len_usec;

void init_buffer()
{
  timings_len = 0;
}

/* **********************************  interrupt handler   ******************************* */

void edge_interrupt()
{
  unsigned long ts = micros();
  unsigned long bit_width;

  statistics.data_interrupts++;
  
  switch( receiver_state)
  {
    case STATE_IDLE:
      /* Do nothing */
      break;

    case STATE_CARRIER_DETECTED:
    
      first_edge_state = digitalRead(CC1101_RXPin);
      receiver_state = STATE_RECEIVING;
      /* Fall throught */

    case STATE_RECEIVING:

      if (timings_len >= CC1101_MAX_TIMINGS)
      {//buffer full - don't accpet anymore
        return;
      }

      bit_width = ts - LastEdgeTime_us;
      LastEdgeTime_us = ts;

      if (bit_width <= 10)  //ignore glitches
      {
        return;
      }
  
      if (bit_width > 255)
        bit_width = 255;

      timings[timings_len++] = (byte)bit_width;
      
      break;

    case STATE_DATA_AVAILABLE:
      /* DO nothing */
      break;      
  }
}

void carrier_sense_interrupt()
{
  unsigned long ts = micros();
  byte carrier = digitalRead(CC1101_CDPin);
  
  statistics.cs_interrupts++;

  switch( receiver_state)
  {
    case STATE_IDLE:
      if( carrier == HIGH) {
        carrier_len_usec = LastEdgeTime_us = ts;
        receiver_state = STATE_CARRIER_DETECTED;
        statistics.carrier_detected++;
      }
      break;

    case STATE_CARRIER_DETECTED:
      if( carrier == LOW) {
        carrier_len_usec = ts - carrier_len_usec;
        if( carrier_len_usec > statistics.carrier_len) {
          statistics.carrier_len = carrier_len_usec;
        }
        receiver_state = STATE_IDLE;
        timings_len = 0;
      }
      break;

    case STATE_RECEIVING:
      if( carrier == LOW) {
        carrier_len_usec = ts - carrier_len_usec;
        
        if( carrier_len_usec > statistics.carrier_len) {
          statistics.carrier_len = carrier_len_usec;
        }
        
        if ((carrier_len_usec >= CARRIER_MIN_LEN_usec) && (carrier_len_usec <= CARRIER_MAX_LEN_usec)) {
          receiver_state = STATE_DATA_AVAILABLE;
          statistics.data_available++; 
        } else {
          receiver_state = STATE_IDLE;
          timings_len = 0;
        }
      }
      break;

    case STATE_DATA_AVAILABLE:
      /* DO nothing */
      break;      
  }
}

ISR( PCINT0_vect) {
  carrier_sense_interrupt();
}

/* *************************  end of interrupt handler   *************************** */

#define wait_Miso()              delay(3)
#define readStatusReg(regAddr)   readReg(regAddr, CC1101_STATUS_REGISTER)
#define readConfigReg(regAddr)   readReg(regAddr, CC1101_CONFIG_REGISTER)

/* *********** PUBLIC *********** */

/*
 * 
 */
void CC1101::setIdleState()
{
  cmdStrobe(CC1101_SIDLE);
}

/*
 * 
 */
void CC1101::setRxState()
{
  cmdStrobe(CC1101_SRX);
}

/*
 * 
 */
void CC1101::setTxState()
{
  cmdStrobe(CC1101_STX);
}

/* 
 * Reset CC1101
 */
void CC1101::reset() 
{
  pinMode(CC1101_CS, OUTPUT);
  digitalWrite(CC1101_CS, HIGH);

  pinMode(CC1101_RXPin, INPUT);
  pinMode(CC1101_CDPin, INPUT);

  SPI.begin();

  deselect();
  delayMicroseconds(5);
  select();
  delayMicroseconds(10);
  deselect();
  delayMicroseconds(41);
  select();

  wait_Miso();
  spi.transfer(CC1101_SRES);
  wait_Miso();

  deselect();

  configureRegisters();

  delay(2000);

  setIdleState();

  init_buffer();
  clear_statistics();
  
  receiver_state = STATE_IDLE;

  attachInterrupt( digitalPinToInterrupt(CC1101_RXPin), edge_interrupt, CHANGE);
  // attachInterrupt( digitalPinToInterrupt(CC1101_CDPin), carrier_sense_interrupt, CHANGE);
  
  cli();
  PCMSK0 |= _BV(PCINT1);
  PCICR |= _BV(PCIE0);
  sei();

  setRxState();
}

/*
 * 
 */
bool CC1101::getCarrierStatus()
{
  byte stat = readStatusReg(CC1101_PKTSTATUS);
  
  return ((stat & 0x40) != 0);
}

/*
 * Read status info into public variables.
 */
void CC1101::getStatusInfo()
{
  freqOffset = readStatusReg(CC1101_FREQEST);
  demodLinkQuality = readStatusReg(CC1101_LQI);
  rssiValue = readStatusReg(CC1101_RSSI);
}

/* ************** PRIVATE ***************** */

/*
 * 
 */
void CC1101::select()
{
  spi.beginTransaction(SPISettings(5000000,MSBFIRST,SPI_MODE0));
  digitalWrite(CC1101_CS, LOW);
}

/*
 * 
 */
void CC1101::deselect()
{
  digitalWrite(CC1101_CS, HIGH);
  spi.endTransaction();
}

/**
 * cmdStrobe
 * 
 * Send command strobe to the CC1101 IC via SPI
 * 
 * 'cmd'  Command strobe
 */     
void CC1101::cmdStrobe(byte cmd) 
{
  select();
  spi.transfer(cmd);
  deselect();
}

/**
 * writeReg
 * 
 * Write single register into the CC1101 IC via SPI
 * 
 * 'regAddr'  Register address
 * 'value'  Value to be writen
 */
void CC1101::writeReg(byte regAddr, byte value) 
{
  select();
  //wait_Miso();
  spi.transfer(regAddr);
  spi.transfer(value);
  deselect();
}

/**
 * readReg
 * 
 * Read CC1101 register via SPI
 * 
 * 'regAddr'  Register address
 * 'regType'  Type of register: CC1101_CONFIG_REGISTER or CC1101_STATUS_REGISTER
 * 
 * Return:
 *  Data byte returned by the CC1101 IC
 */
byte CC1101::readReg(byte regAddr, byte regType)
{
  byte addr, val;

  addr = regAddr | regType;
  select();
  //wait_Miso();
  spi.transfer(addr);
  val = spi.transfer(0x00);
  deselect();

  return val;
}

/*
 * wakeUp
 * 
 * Wake up CC1101 from Power Down state
 */
void CC1101::wakeUp(void)
{
  select();
  wait_Miso();
  deselect();
}

/*
 * Configure CC1101 registers
 */
void CC1101::configureRegisters(void) 
{
  writeReg(CC1101_IOCFG2,  CC1101_DEFVAL_IOCFG2);
  writeReg(CC1101_IOCFG1,  CC1101_DEFVAL_IOCFG1);
  writeReg(CC1101_IOCFG0,  CC1101_DEFVAL_IOCFG0);
  writeReg(CC1101_FIFOTHR,  CC1101_DEFVAL_FIFOTHR);
  writeReg(CC1101_PKTLEN,  CC1101_DEFVAL_PKTLEN);
  writeReg(CC1101_PKTCTRL1,  CC1101_DEFVAL_PKTCTRL1);
  writeReg(CC1101_PKTCTRL0,  CC1101_DEFVAL_PKTCTRL0);

  // Set default synchronization word
  //setSyncWord(syncWord);

  // Set default device address
  //setDevAddress(devAddress);

  // Set default frequency channel
  //setChannel(channel);
  
  writeReg(CC1101_FSCTRL1,  CC1101_DEFVAL_FSCTRL1);
  writeReg(CC1101_FSCTRL0,  CC1101_DEFVAL_FSCTRL0);

//  // Set default carrier frequency = 868 MHz
//  writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_868);
//  writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_868);
//  writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_868);

  // Set default carrier frequency = 433 MHz
  writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_433);
  writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_433);
  writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_433);

  writeReg(CC1101_MDMCFG4,  CC1101_DEFVAL_MDMCFG4);
  writeReg(CC1101_MDMCFG3,  CC1101_DEFVAL_MDMCFG3);
  writeReg(CC1101_MDMCFG2,  CC1101_DEFVAL_MDMCFG2);
  writeReg(CC1101_MDMCFG1,  CC1101_DEFVAL_MDMCFG1);
  writeReg(CC1101_MDMCFG0,  CC1101_DEFVAL_MDMCFG0);
  writeReg(CC1101_DEVIATN,  CC1101_DEFVAL_DEVIATN);
  writeReg(CC1101_MCSM2,  CC1101_DEFVAL_MCSM2);
  writeReg(CC1101_MCSM1,  CC1101_DEFVAL_MCSM1);
  writeReg(CC1101_MCSM0,  CC1101_DEFVAL_MCSM0);
  writeReg(CC1101_FOCCFG,  CC1101_DEFVAL_FOCCFG);
  writeReg(CC1101_BSCFG,  CC1101_DEFVAL_BSCFG);
  writeReg(CC1101_AGCCTRL2,  CC1101_DEFVAL_AGCCTRL2);
  writeReg(CC1101_AGCCTRL1,  CC1101_DEFVAL_AGCCTRL1);
  writeReg(CC1101_AGCCTRL0,  CC1101_DEFVAL_AGCCTRL0);
  writeReg(CC1101_WOREVT1,  CC1101_DEFVAL_WOREVT1);
  writeReg(CC1101_WOREVT0,  CC1101_DEFVAL_WOREVT0);
  writeReg(CC1101_WORCTRL,  CC1101_DEFVAL_WORCTRL);
  writeReg(CC1101_FREND1,  CC1101_DEFVAL_FREND1);
  writeReg(CC1101_FREND0,  CC1101_DEFVAL_FREND0);
  writeReg(CC1101_FSCAL3,  CC1101_DEFVAL_FSCAL3);
  writeReg(CC1101_FSCAL2,  CC1101_DEFVAL_FSCAL2);
  writeReg(CC1101_FSCAL1,  CC1101_DEFVAL_FSCAL1);
  writeReg(CC1101_FSCAL0,  CC1101_DEFVAL_FSCAL0);
  writeReg(CC1101_RCCTRL1,  CC1101_DEFVAL_RCCTRL1);
  writeReg(CC1101_RCCTRL0,  CC1101_DEFVAL_RCCTRL0);
  writeReg(CC1101_FSTEST,  CC1101_DEFVAL_FSTEST);
  writeReg(CC1101_PTEST,  CC1101_DEFVAL_PTEST);
  writeReg(CC1101_AGCTEST,  CC1101_DEFVAL_AGCTEST);
  writeReg(CC1101_TEST2,  CC1101_DEFVAL_TEST2);
  writeReg(CC1101_TEST1,  CC1101_DEFVAL_TEST1);
  writeReg(CC1101_TEST0,  CC1101_DEFVAL_TEST0);
}

#endif
