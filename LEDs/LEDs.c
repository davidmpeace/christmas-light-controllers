#include <avr/io.h>                        /* Defines pins, ports, etc */
#include <util/delay.h>                     /* Functions to waste time */
#include <stdlib.h>
#include <math.h>

// avrdude -c avrispmkII -p atmega328p -P usb -U lfuse:w:0xe0:m

void oneOut(void)
{
    PORTC = 0b11111111;
    _delay_us(0.7);
    PORTC = 0b00000000;
    _delay_us(0.6);
}

void zeroOut(void)
{
    PORTC = 0b11111111;
    _delay_us(0.35);
    PORTC = 0b00000000;
    _delay_us(0.8);
}

void resetOut(void)
{
    PORTC = 0b00000000;
    _delay_us(51);
}


void numberOut( int myNumber )
{
    for( int i = 128; i >= 1; i = i / 2)
    {
        if( myNumber & i )
            oneOut();
        else
            zeroOut();
    }
}

void rgb( int red, int green, int blue )
{
    numberOut(green);
    numberOut(red);
    numberOut(blue);
}

void all( int red, int green, int blue )
{
    for( int i = 0; i<=150; i++ )
    {
        rgb( red,green,blue );
    }
    resetOut();
}

void allOn(void)
{
    all(255,255,255);
}

void allOff(void)
{
    all(0,0,0);
}

int main(void) 
{
    DDRC = 0b11111111;
    resetOut();
    
    int r,g,b;

    while (1) 
    {

        all(0,0,0);

for( int i = 0; i<=150; i++ )
        {

        for( int j = 0; j<=150; j++ )
        {
            if( j==i+2 )
            {
                rgb( 255,0,0 );
            }
            else if( j==i+1 )
            {
                rgb( 255,0,0 );
            }
            else if( j==i )
            {
                rgb( 255,0,0 );
            }
            else if(j==i-1 )
            {
                rgb( 50,0,0 );
            }
            else if(j==i-2 )
            {
                rgb( 10,0,0 );
            }
            else 
            {
                rgb( 0,0,0 );
            }
            
        }
            resetOut();
            // _delay_ms(10);
         }


for( int i = 150; i>=0; i-- )
        {

        for( int j = 0; j<=150; j++ )
        {
            if( j==i-2 )
            {
                rgb( 255,0,0 );
            }
            else if( j==i-1 )
            {
                rgb( 0,255,0 );
            }
            else if( j==i )
            {
                rgb( 0,0,255 );
            }
            else if(j==i+1 )
            {
                rgb( 0,0,50 );
            }
            else if(j==i+2 )
            {
                rgb( 0,0,10 );
            }
            else 
            {
                rgb( 0,0,0 );
            }
            
        }
            resetOut();
            _delay_ms(50);
         }
        
    }

    return (0);
}