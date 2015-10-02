#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/setbaud.h>
#include <stdlib.h>
#include <math.h>

// Run to set fuse bits
// avrdude -c avrispmkII -p m324a -P usb -U lfuse:w:0xe0:m

#define CHANNELS_PORT   PORTA
#define DIP_SWITCH      PINC
#define LISTEN_SWITCH   PC0 

#define LED_AVR      PD7 // OC2A
#define LED_ERROR    PD6 // OC2B
#define LED_RECEIVE  PD5 // OC1A
#define LED_TRANSMIT PD4 // OC1B
#define LED_LISTEN   PB3 // OC0A

#define LEVELS_OF_BRIGHTNESS 30

volatile uint8_t phaseValues[LEVELS_OF_BRIGHTNESS];

volatile uint8_t deviceAddress   = 8;   // UNIQUE VALUE FOR EACH FIRMWARE

volatile uint8_t positionInPhase = 0;   // Counts from 0-30
volatile uint8_t listening       = 0;   // Determines if we are listening or not
volatile uint16_t currentChannel = 0;   // The current channel we are receiving data for via RXD
volatile uint8_t lastIntReceived = 0;

volatile int tog = 0;
void setLED( int led, int onOrOff )
{
    if( led == LED_LISTEN )
    {
        if( onOrOff == 0 )
        {
            PORTB |= (1 << LED_LISTEN);
        }
        else 
        {
            PORTB &= ~(1 << LED_LISTEN);
        }
    }
    else 
    {
        if( onOrOff == 0 )
        {
            PORTD |= (1 << led);
        }
        else 
        {
            PORTD &= ~(1 << led);
        }
    }
}

void setAllLEDs( int onOrOff )
{
    setLED( LED_AVR,      onOrOff );
    setLED( LED_ERROR,    onOrOff );
    setLED( LED_RECEIVE,  onOrOff );
    setLED( LED_TRANSMIT, onOrOff );
    setLED( LED_LISTEN,   onOrOff );
}

/**
 * Set a channel to a specific level of brightness
 */
void updateDeviceAddress(void)
{
    // A clear bit, means the switch is pulling to ground.  Clear=On
    if( bit_is_clear(DIP_SWITCH, LISTEN_SWITCH) )
    {
        listening = 1;
        setLED(LED_LISTEN, 1);
    }
    else
    {
        listening = 0;
        setLED(LED_LISTEN, 0);
    }

    // deviceAddress = 0;
    // deviceAddress += ( bit_is_clear(DIP_SWITCH, PC7) ) ? 1  : 0;
    // deviceAddress += ( bit_is_clear(DIP_SWITCH, PC6) ) ? 2  : 0;
    // deviceAddress += ( bit_is_clear(DIP_SWITCH, PC5) ) ? 4  : 0;
    // deviceAddress += ( bit_is_clear(DIP_SWITCH, PC4) ) ? 8  : 0;
    // deviceAddress += ( bit_is_clear(DIP_SWITCH, PC3) ) ? 16 : 0;
    // deviceAddress += ( bit_is_clear(DIP_SWITCH, PC2) ) ? 32 : 0;
    // deviceAddress += ( bit_is_clear(DIP_SWITCH, PC1) ) ? 64 : 0;

}

/**
 * Set a channel to a specific level of brightness
 */
void setChannelValue( int channel, int brightnessValue )
{
    // Invert our brightness value
    brightnessValue = LEVELS_OF_BRIGHTNESS - brightnessValue;

    for( int i = 0; i < LEVELS_OF_BRIGHTNESS; i++ )
    {
        if( i >= brightnessValue )
        {
            phaseValues[i] &= ~(1 << channel);
        }
        else 
        {
            phaseValues[i] |= (1 << channel);
        }
    }
}

void setAllChannels( int brightnessValue )
{
    for( int i = 0; i < 8; i++ )
    {
        setChannelValue( i, brightnessValue );
    }
}

/**
 * |-----------------------------------------------------------------
 * | DIP Switch Change Interupt
 * |-----------------------------------------------------------------
 * |
 * | This interupt is triggered when any pin on the DIP switch changes.
 * | All we need to do is update our address, and see if we should
 * | still be listening or not.
 * |
 */
ISR( PCINT2_vect )
{
    updateDeviceAddress();
}

/**
 * |-----------------------------------------------------------------
 * | Zero Cross Detection Interupt
 * |-----------------------------------------------------------------
 * |
 * | This interupt is triggered via the zero cross pin
 * | when the AC signal crosses 0 volts.  This interupt is called on
 * | a rising voltage.
 * |
 */
ISR( INT0_vect )
{
    CHANNELS_PORT   = 255;    // Turn all off
    positionInPhase = 0;    // Reset phase position
    TCNT1           = 0;              // Reset counter
}

/**
 * |-----------------------------------------------------------------
 * | Clock Cycle Interupt
 * |-----------------------------------------------------------------
 * |
 * | This method is triggered when a specific number of clock cycles
 * | have taken place.  Specifically, the number stored in OCR0A.
 * | It's job is to adjust the output pins X number of times between zero crosses
 * |
 */
ISR( TIMER1_COMPA_vect )
{
    CHANNELS_PORT = phaseValues[positionInPhase];
    positionInPhase++;
}

/**
 * |-----------------------------------------------------------------
 * | Byte Received Interupt
 * |-----------------------------------------------------------------
 * |
 * | This method is triggered whenever a byte is received over the USART.
 * |
 */
ISR(USART0_RX_vect)
{
    char ReceivedByte = UDR0;
    uint16_t receivedInt = UDR0;

    if( receivedInt == 65 && lastIntReceived == 117 )   //uA
    {
       currentChannel = 0;
    }
    else 
    {
        uint16_t channelNumber = (currentChannel - deviceAddress);
        
        if( channelNumber >= 0 && channelNumber <= 7 )
        {
            setChannelValue( channelNumber, round(receivedInt / 8.5) );
        }
        
        currentChannel++;
   }

   lastIntReceived = receivedInt;

   while ( !(UCSR0A & (1<<UDRE0)) )
   {
       
   }

   UDR0 = ReceivedByte;
}

/**
 * |------------------------------------------
 * | Main Application Loop
 * |------------------------------------------
 * |
 * | 
 * |
 * |
 */
int main(void) 
{

    // // ---------------------------------------------------------
    // // Configure all our input & output pins.  0=input, 1=output
    // // ---------------------------------------------------------
    DDRA = 0b11111111;  // Set the CHANNEL OUTPUT PORT all to output
    DDRB = 0b00001000;  // Listen LED to output
    DDRC = 0b00000000;  // Set the DIP SWITCH PORT all to input
    DDRD = 0b11110000;  // ALl other LEDs set to output

    // ---------------------------------------------------------
    // Internal pull-up resistors. 1=on, 0=off
    // ---------------------------------------------------------
    PINC = 0b11111111;  // Set the internal pull-up resistors for the DIP SWITCH

    // ---------------------------------------------------------
    // Configure our zero cross interupt
    // ---------------------------------------------------------
    EICRA |= (1 << ISC01) | (1 << ISC00); // External Interrupt Control Register A, setting both values means we will interupt on a rising edge
    EIMSK |= (1 << INT0);                 // Mask the interupt to watch the zero cross pin only (INT0)

    // ---------------------------------------------------------
    // Configure Phase Control timer
    // ---------------------------------------------------------
    
    TCCR1B |= (1 << WGM12); // Turn on time compare 
    OCR1A = (F_CPU / 120UL) / LEVELS_OF_BRIGHTNESS;  // With the timer set to interupt every 5120 clock cycles, we should interupt 30 times between zero crosses

    TCCR1B |= (1 << CS10);   // Set up timer for 1 tick per clock cycle
    TIMSK1 |= (1 << OCIE1A); // Enable CTC interrupt (when compare is found), will call the interupt when OCR1A is met

    // ---------------------------------------------------------
    // Interupt on any DIP SWITCH change
    // ---------------------------------------------------------

    PCICR = (1 << PCIE2);  // The Pin Change Interupt 2 turns on interupts for PCINT16 - PCINT23
    PCMSK2 = 0b11111111;   // Mask on interupts for the DIP SWITCH pins PCINT16 - PCINT23.

    // ---------------------------------------------------------
    // Interupt on byte received on USART
    // ---------------------------------------------------------

    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

    UCSR0B = (1 << RXEN0) | (1 << TXEN0);  // Receive enable, transmit enable

    UCSR0B |= (1 << RXCIE0); // Receive byte interupts

    // ---------------------------------------------------------
    // Global interupt flag (set enable interupt)
    // ---------------------------------------------------------

    sei();

    updateDeviceAddress();
    setAllLEDs(0);         // Turn all LEDs OFF
    setLED( LED_AVR, 1 );  // Power LED On
    setAllChannels(0);     // Turn all channels OFF

    while (1) 
    {
        
    }

    return (0);
}
