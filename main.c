/*
* main.c    ATmega88P    F_CPU = 8000000 Hz
*
* Created on: 2018-11-12
*     Author: admin
*/
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include "lcd44780.h"

volatile int16_t timer = 0;
volatile int16_t timer2 = 0;
volatile int16_t timer_buzzer = 0;
volatile int16_t delay = 0;   //czas przerwy w brzeczenu buzzera w ms
volatile int16_t coeff = 0;
volatile double distance = 0.0;   //dystans w cm
int flaga = 0;
int flag_buzzer = 0;  //flaga przelaczajaca poziom sygnalu buzzera

void trigger() {
    _delay_ms( 50 );

    //trigger do czujnika, PDR4 - TRIGGER
    PORTD &= ~( 1 << PD4 );  //PD4 - LOW
    _delay_us( 2 );
    PORTD |= ( 1 << PD4 );   //PD4 - HIGH
    _delay_us( 10 );
    PORTD &= ~( 1 << PD4 );  //PD4 - LOW
}

void buzzer_led(){
      if ( distance < 15 )
            delay = 100;
        else if ( distance < 20 )
            delay = 200;
        else if ( distance < 25 )
            delay = 400;
        else if ( distance < 30 )
            delay = 700;
        else if ( distance < 35 )
            delay = 1000;
        else if ( distance < 40 )
            delay = 1400;
        else if ( distance < 45 )
            delay = 1700;
        else if ( distance < 50 )
            delay = 2000;

        if ( distance > 50 ) {
            TIMSK2 &= ~( 1 << OCIE2A ); //wylaczenie timera
            PORTB &= ~( 1 << PB1 );    //wylaczenie buzzera
            PORTB |= ( 1 << PB0 );     //wylaczenie diody
            timer_buzzer = 0;
            flag_buzzer = 0;
        } else if ( distance <= 50 && distance >= 10 ) {
            TIMSK2 |= ( 1 << OCIE2A );    //wlaczenie timera
            //stan gdy buzzer ma sie wlaczyc
            if ( flag_buzzer == 0 ) {
                if ( timer_buzzer < 400 ) {
                    PORTB |= ( 1 << PB1 );     //wlaczenie buzzera na 400ms
                    PORTB &= ~( 1 << PB0 );    //wlaczenie diody na 400ms
                } else {
                    flag_buzzer = 1;
                    timer_buzzer = 0;
                }
            }
            //stan gdy buzzer ma sie wylaczyc
            if ( flag_buzzer == 1 ) {
                if ( timer_buzzer < delay ) {
                    PORTB &= ~( 1 << PB1 );     //wylaczenie buzzera
                    PORTB = ( 1 << PB0 );       //wylaczenie diody
                } else {
                    flag_buzzer = 0;
                    timer_buzzer = 0;
                }
            }
        }     //stan gdy buzzer przechodzi w sygnal ciagly
        else if ( distance < 10 ) {
            TIMSK2 &= ~( 1 << OCIE2A );  // wylaczenie timera
            PORTB |= ( 1 << PB1 );   //wlaczenie buzzera
            PORTB &= ~( 1 << PB0 );   //wlaczenie diody
            timer_buzzer = 0;
            flag_buzzer = 0;
        }
}

int main( void ) {

    DDRD |= ( 1 << PD4 );   //PD4 - OUT, trigger
    PORTD &= ~( 1 << PD2 );   //PD2 - IN, echo
    DDRB |= ( 1 << PB1 );    //PB1 - OUT - BUZZER
    DDRB |= ( 1 << PB0 );   //PB0 - OUT - LED
    lcd_init();

    //rejestry przerwan
    EIMSK |= ( 1 << INT0 );    //zezwolenie na przerwanie INT0
    EICRA |= ( 1 << ISC00 );  //INT0- zezwolenie na przerwanie przy zmianie stanu

    //rejestry timera
    TCCR0A |= ( 1 << WGM01 );   //tryb operacji timera CTC, Clear Timer on Compare Match
    OCR0A = 56;    //wartosc porownywana z licznikem TCNTO
    TCCR2A |= ( 1 << WGM21 );   //tryb operacji CTC
    OCR2A = 124;    //wartosc porownywana z licznikem TCNT2

    sei();    //globalne wlaczenie przerwan

    TCCR0B |= ( 1 << CS01 );  //prescaler - clk/8
    TCCR2B |= ( 1 << CS22 );   //prescaler - clk/64

    while ( 1 ) {

        trigger();   //wyslanie sygnalu o dlugosci 10ms do czujnika

        distance = timer2 / 2;  //odleglosc w cm

        buzzer_led();

        lcd_locate( 0, 0 );
        lcd_str( "Parking" );
        lcd_locate( 0, 9 );
        lcd_str( "Sensor" );
        lcd_locate( 1, 0 );
        lcd_str( "Distance: " );
        lcd_locate( 1, 9 );
        lcd_str( "     " );
        lcd_locate( 1, 10 );
        if ( distance < 50 ) {
            lcd_int( distance );
            lcd_locate( 1, 14 );
            lcd_str( "cm" );
        } else {
            lcd_locate( 1, 9 );
            lcd_str( "too far" );
        }
    }
}
//procerdura obslugi przerwania na skutek przepelnienia timera obslugujacego buzzer
ISR( TIMER2_COMPA_vect ) {
    timer_buzzer++;
}
//procerdura obslugi przerwania na skutek przepelnienia timera zliczajacego 29us
ISR( TIMER0_COMPA_vect ) {
    timer++;
}
//procedura obs³ugi przerwañ
ISR( INT0_vect ) {
    if ( flaga == 0 ) {
        TIMSK0 |= ( 1 << OCIE0A );   //wlaczenie timera, przerwanie wywolane po przepelnieniu timera
        flaga = 1;
    } else if ( flaga == 1 ) {
        TIMSK0 &= ~( 1 << OCIE0A );    //wylaczenie timera
        timer2 = timer;
        timer = 0;
        flaga = 0;
    }
}