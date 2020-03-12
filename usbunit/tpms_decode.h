/*
 * Abarth 124 TPMS Sensor decoding
 *
 */

/********************************************************/

typedef byte bitLength_t;
typedef byte byteLength_t;

/* Max number of bits supported in bitArray_t */
#define MAX_BITS   200
#define MAX_BITS_ARRAY ((MAX_BITS + 7) / 8)

/* Max number of bytes supported in byteArray_t */
#define MAX_BYTES   10

/*
 * Both structures MUST be cleared before usage!
 *
 * Call:
 *   void clear_bit_array( bitArray_t *data);
 *   void clear_byte_array( byteArray_t *data);
 */

typedef struct bitArray_t {
    bitLength_t capacity;
    bitLength_t length;
    byte bits[MAX_BITS_ARRAY];    
} bitArray_t;

typedef struct byteArray_t {
    byteLength_t capacity;
    byteLength_t length;
    byte bytes[MAX_BYTES];
} byteArray_t;


/****************** PARAMETERS ***************************/

#define MANCHESTER_DECODING_MASK  0b1010101010101010

/* Pulse range in micro seconds */
#define MIN_SHORT_usec   ((byte) 20)
#define MIN_LONG_usec    ((byte) 80)

/* Pulse type */
#define INVALID ((byte)0)
#define SHORT   ((byte)1)
#define LONG    ((byte)2)


/***************** forward defines **********************/

bool decode_tpms( volatile byte timing_array[], unsigned int timing_count, bool start_edge, byteArray_t *data);

void clear_bit_array( bitArray_t *bits);
bool get_bit( bitArray_t *bits, bitLength_t bitno);
void set_bit( bitArray_t *bits, bitLength_t bitno, bool value);
void append_bit( bitArray_t *bits, bool value);

void clear_byte_array( byteArray_t *data);
byte get_byte( byteArray_t *data, byteLength_t byteno);
void append_byte( byteArray_t *data, byte value);

byte pulse_type( byte time);
void bit_decode( volatile byte timing[], unsigned int count, bool start_value, bitArray_t *data);

bitLength_t find_preamble( bitArray_t *data, bitArray_t *preamble);

void manchester_decode( bitArray_t *bits, bitLength_t start, byteArray_t *data);

bool check_checksum( byteArray_t *data);
byte checksum_xor( byteArray_t *data, byte bytes);

/********************************************************/

/*
 * Returns true if decoding was successful.
 */
bool decode_tpms( volatile byte timing_array[], unsigned int timing_count, bool start_edge, byteArray_t *data)
{
    bitArray_t preamble;
    bitArray_t decoded_bits;   // Decoded timing bits

    bitLength_t data_start;    // Data start after preamble
    
    byte a_byte;
    int i;

    clear_byte_array( data);
    
    /* Create a bit array that contains the preamble. 
     * It is easier to handle.
     */
    clear_bit_array( &preamble);
    preamble.bits[0] = 0xAA;
    preamble.bits[1] = 0xA9;
    preamble.length = 16;

    if( timing_count > statistics.max_timings) {
      statistics.max_timings = timing_count;
    }

    bit_decode( timing_array, timing_count, start_edge, &decoded_bits);

    data_start = find_preamble( &decoded_bits, &preamble);
   
    if( data_start > 0) {
      statistics.preamble_found++;
       
      manchester_decode( &decoded_bits, data_start, data);

      if( data->length > 9) {
        data->length = 9;
      }

      if( check_checksum( data)) {
        statistics.checksum_ok++;
        return true;

      } else {
        statistics.checksum_fails++;
      }
    }

    return false;
}

/********************************************************/

void clear_bit_array( bitArray_t *bits)
{
    bitLength_t i;

    bits->length = 0;
    bits->capacity = MAX_BITS;

    for( i = 0; i < MAX_BITS_ARRAY; i++) {
        bits->bits[i] = 0;
    }
}

/* 
 * Bitno starts at 0.
 */
bool get_bit( bitArray_t *bits, bitLength_t bitno)
{
    if( bitno < bits->capacity) {
        return (bits->bits[bitno/8] & (1 << (7-(bitno % 8)))) ? true : false;
    } else {

#ifdef SHOWDEBUGINFO
        Serial.println( F("ERROR IN get_bit(): bitno >= capacity"));
#endif
        
        return false;
    }
}

/* 
 * Bitno starts at 0.
 * This funktion also adjusts bits->length.
 */
void set_bit( bitArray_t *bits, bitLength_t bitno, bool value)
{
    if( bitno < bits->capacity) {
        if( value) {
            bits->bits[bitno/8] |= (byte)(1 << (7-(bitno % 8)));
        } else {
            bits->bits[bitno/8] &= ~((byte)(1 << (7-(bitno % 8))));
        }
        if( bitno >= bits->length) {
            bits->length = bitno +1;
        }
    } else {

#ifdef SHOWDEBUGINFO
        Serial.println( F("ERROR IN set_bit(): bitno >= capacity"));
#endif

    }
}

void append_bit( bitArray_t *bits, bool value)
{
  set_bit( bits, bits->length, value);
}

/********************************************************/

void clear_byte_array( byteArray_t *data)
{
    byteLength_t i;

    data->length = 0;
    data->capacity = MAX_BYTES;
    
    for( i = 0; i < MAX_BYTES; i++) {
        data->bytes[i] = 0;
    }
}

/* 
 * Byteno starts at 0.
 */
byte get_byte( byteArray_t *data, byteLength_t byteno)
{
    if( byteno < data->capacity) {
        return data->bytes[byteno];
    } else {

#ifdef SHOWDEBUGINFO
        Serial.println(F("ERROR IN get_byte(): byteno >= capacity"));
#endif

        return 0;
    }
}

/* 
 * Byteno starts at 0.
 * This funktion also adjusts data->length.
 */
void append_byte( byteArray_t *data, byte value)
{
    if( data->length < data->capacity) {
        data->bytes[data->length] = value;
        data->length++;
    } else {

#ifdef SHOWDEBUGINFO
        Serial.println(F("ERROR IN append_byte(): length >= capacity"));
#endif

    }
}

/********************************************************/

byte pulse_type( byte time_usec)
{
    if( time_usec < MIN_SHORT_usec) {
        return INVALID;
    }

    if( time_usec < MIN_LONG_usec) {
        return SHORT;
    }

    return LONG;
}

/********************************************************/

/* Konvertiert timings zu bits.
 *
 * Parameter:
 *   const byte timing[]    - Timing array
 *   int count              - Anzahl der bytes in timing array
 *   data                   - Rückgabe der Bit Werte
 *
 * Return:
 *   nix
 */
void bit_decode( volatile byte timing[], unsigned int count, bool start_value,  bitArray_t *bits)
{
    unsigned int timing_idx = 0;
    unsigned int timing_len_usec = 0;
    bitLength_t bit_count = 0;
    bool level = start_value;

    clear_bit_array( bits);

    for( timing_idx = 0; timing_idx < count; timing_idx++) {
        
        timing_len_usec += timing[timing_idx];
        
        switch( pulse_type( timing[timing_idx] ) ) {
        case LONG: /* 2 pulses */
            set_bit( bits, bit_count++, level);
            /* Fall through */

        case SHORT: /* 1 pulse */
            set_bit( bits, bit_count++, level);
            break;
        }

        level = !level;
    }
}

/********************************************************/

/* Findet die Präambel im bit array.
 * Gibt den index des ersten bits nach der Präambel zurück.
 *
 * Parameter:
 *
 * Return: > 0    - Start Index nach der Präambel
 *         0      - Keine Präambel gefunden
 */
bitLength_t find_preamble( bitArray_t *bits, bitArray_t *preamble)
{
    bitLength_t bit_idx = 0;
    bitLength_t pre_idx = 0;
    bitLength_t saved_start = 0;
    bitLength_t data_start = 0;

    if( bits->length <= preamble->length) {
      /* Not enough bits to detect preamble - bail out.*/
      return 0;
    }

    while( bit_idx < (bits->length - preamble->length)) {

        if( pre_idx == 0) { /* Remember start for restart on match failure */
            saved_start = bit_idx;
        }
        
        if( get_bit( bits, bit_idx) == get_bit( preamble, pre_idx) ) { 
            /* Match, advance preamble and bit index */
            pre_idx++;
            bit_idx++;
            
            if( pre_idx >= preamble->length) { /* All preamble bits found, done */
                data_start = bit_idx;
                break;
            }
        } else { /* fail, restart with next bit */
            pre_idx = 0;
            bit_idx = saved_start + 1;
        }
    }

    return data_start;
}

/********************************************************/

/* Manchester decode geht am einfachsten über ein XOR verknüpfung
 * mit dem Clock Signal  ( 1010101010101.... )
 */
void manchester_decode( bitArray_t *bits, bitLength_t start, byteArray_t *data)
{
    bitLength_t bit_idx;
    bitLength_t bit_count = 0;

    unsigned int an_int = 0;
    byte a_byte = 0;
    byte n;

    clear_byte_array( data);
    
    for( bit_idx = start; bit_idx < bits->length; bit_idx++) {

        an_int <<= 1;
        an_int |= get_bit( bits, bit_idx) ? 1 : 0;
        bit_count++;

        if( bit_count == 16) { /* Decode 16 bits via XOR with clock signal to one byte */
            an_int ^= MANCHESTER_DECODING_MASK;
            a_byte = 0;
            
            for( n = 0; n < 8; n++) { /* Convert 16 bits to one byte */
                a_byte <<= 1;
                a_byte |= ((an_int & 0xc000) ? 1 : 0);
                an_int <<= 2;
            }
            
            append_byte( data, a_byte);
            bit_count = 0;
            an_int = 0;
        }
    }
}

/* 
 *  Check XOR checksum.
 *  Compute XOR value of first 8 bytes, than compare with 9th byte.
 *  
 *  Returns true if checksum is ok.
 */
bool check_checksum( byteArray_t *data)
{
  byte csum = checksum_xor( data, 8);

  return csum == get_byte( data, 8);
}

/*
 * Compute XOR value of first 'count' bytes in data.
 */
byte checksum_xor( byteArray_t *data, byte count)
{
  byteLength_t i;
  byte checksum;
    
  checksum = 0;
  for( i = 0; i < count; i++) {
    checksum ^= get_byte( data, i);
  }

  return checksum;
}
