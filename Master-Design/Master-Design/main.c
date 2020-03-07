
/*
 * Master-Design.c
 *
 * Created: 2020-03-05 1:27:22 PM
 * Author : Rowan Darlison
 */ 

#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <stdio.h>
#include <string.h>

/* UART Code */
int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);
FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE mystdin = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);
/******************************************************************************
******************************************************************************/
int uart_putchar(char c, FILE *stream)
{
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
    return 0;
}
/******************************************************************************
******************************************************************************/
int uart_getchar(FILE *stream)
{
    /* Wait until data exists. */
    loop_until_bit_is_set(UCSR0A, RXC0);
    return UDR0;
}
/******************************************************************************
******************************************************************************/
void init_uart(void)
{
    UCSR0B = (1<<RXEN0) | (1<<TXEN0);
    UBRR0 = 103;  //With 16 MHz Crystal, 9600 baud = 103
    stdout = &mystdout;
    stdin = &mystdin;
}


void init_hardware(void)
{
    //Configure pins you need as OUTPUT. You'll have to look at where you plug
    //the keypad in. For example if you were using PORTD6, PORTD5, and PORTB0 as outputs
    //you would do:
    //PB4, 3, 2, 1, 0
    //PD7, 6, 5
    DDRB |=  (1<<PORTB1) | (1<<PORTB2) | (1<<PORTB3) | (1<<PORTB4); 
    //Next, you might want to use built-in pull-up resistors. For example if we
    //decide PINDB2 and PINB3 are inp
    //uts, you could enable pull-ups with:
    PORTD |= (1<<PORTD7) | (1<<PORTD6);
    PORTB |= (1<<PORTB0);
}

void set_row_low(unsigned int row)
{

    PORTB |= (1<<PORTB1) | (1<<PORTB2) | (1<<PORTB3) | (1<<PORTB4);
    
    if(row == 0){
        PORTB &= ~(1<<PORTB4);
        
        }else if(row == 1){
        PORTB &= ~(1<<PORTB3);
        
        }else if(row == 2){
        PORTB &= ~(1<<PORTB2);
        
        }else if(row == 3){
        PORTB &= ~(1<<PORTB1);
    }
}

int col_pushed(void)
{
    if ((PINB & (1<<PINB0)) == 0){
        return 1;
    }
    if((PIND & (1<<PIND7)) == 0){
        return 2;
    }
    if((PIND & (1<<PIND6)) == 0){
        return 3;
    }
    return 0;
}

//An easy way to map the XY location is a lookup table.
//You'll need to fill this in - you might need to figure out
//if mirrored etc or something funky.
char buttons[4][3] = {{'1', '2', '3'},
{'4', '5', '6'},
{'7', '8', '9'},
{'*', '0', '#'}};

char get_button(void)
{
    
    for(int row = 0; row < 4; row++){
        set_row_low(row);
        _delay_ms(20);
        
        int col = col_pushed();
        
        if(col){
            return buttons[row][col-1];
        }
    }
}

char get_new_button(void)
{
    static char last_button;
    char b = get_button();
    
    if(b == last_button) return 0;
    
    last_button = b;
    
    return b;
}

int main(void){
    init_hardware();
    init_uart(); // initialization
    printf("System Booted, built %s on %s\n", __TIME__, __DATE__);
    char pin[10];
    int i = 0, j = 0;
    
    
    while(1){
        char b = get_new_button();
        
        //Do something special with "#", for example clear partial entry
        if(b == '#'){
            i = 0;
            continue;
        }
        DDRC |= (1<<PORTC2);
        DDRD |= (1<<PORTD3) | (1<<PORTD4);
        if(b){
            pin[i++] = b;
        }
        
        if(i >= 4){
            pin[4] = 0;
            printf("Entered PIN: %s\n", pin);
            if((pin[0] == '1') && (pin[1] == '2') && (pin[2] == '3') && (pin[3] == '4')){ //0 = "48", 1 ="50" ... 9 = "57", * = "42"
                PORTD |= (1<<PORTD3); //disarmed
            }else if((pin[0] == '4') && (pin[1] == '3') && (pin[2] == '2') && (pin[3] == '1')){
            PORTD |= (1<<PORTD3) | (1<<PORTD4); //silent alarm
            }else{
                PORTC |= (1<<PORTC2); // wrong password/alarm
            }
            while(1){
                char r = get_new_button();
                if(r){
                    pin[j++]= r;
                }
                if(j >= 1){
                    pin[1]=0;
                    if(pin[0] == '*'){
                        break;
                    }
                }
            }
            PORTD &= ~(1<<PORTD3);
            PORTC &= ~(1<<PORTC2);
            PORTD &= ~(1<<PORTD4);
            i = 0;
            j = 0;
        }
    }
}
/* Notes
Need to:
When doing Admin code, delete previous password by setting array to 0.
scan function for new password code

Curator code --> make new code array, inside the function we disable alarm.
Silent alarm code --> make new code array, inside function, enable silent alarm.
*/

// ECHOLOCATOR


/*
 * Main.c
 *
 * Author : Mark LeBlanc, Dalhousie University
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <stdio.h>


volatile unsigned char MIP;
volatile unsigned int ECHOHigh, ECHOLow, ECHOHighPipe;
volatile unsigned int TimeOutCnt,Tick;
volatile unsigned int distance;


/***************************************************************************************
When the echo length is longer than the counter duration, we use an additional byte to 
indicate how many times we reach the maximum value. 
***************************************************************************************/
ISR (TIMER1_OVF_vect) { // For long ECHO's
    if(ECHOHighPipe >= 2) {
        TIMSK1 = 0; // No further interrupts.
        TCCR1B = 0; // Stop Clock
        MIP = 0xFF; // End Measurement
    }
    
    ECHOHighPipe++; // Add 1 to High byte.
}

/***************************************************************************************
Interrupt service routine called when the input capture pin state is changed
***************************************************************************************/
ISR (TIMER1_CAPT_vect) {    // Start and Stop ECHO measurement;
    if((TCCR1B & (1<<ICES1)) != 0) { // a rising edge has been detected
        TCCR1B |= (1<<CS01) | (1<<CS00);    // Start counting with ck/8;
        TCCR1B &= ~(1<<ICES1);  // Configure Negative Edge Capture for end of echo pulse.
    }
    
    else {                      // a falling edge has been detected
        ECHOLow = TCNT1;
        ECHOHigh = ECHOHighPipe;
        TIMSK1 = (1<<OCIE1B);   // Enables the Compare B interrupt for POST Trigger Delay: Approx 10mS
        TCNT1 = 0;
    }
}


/***************************************************************************************
Interrupt service routine called when the counter 1 has reached the compare value
***************************************************************************************/
ISR (TIMER1_COMPB_vect) {   // Compare B: Post ECHO delay 10mS



    MIP = 0;    // End Measurement
}

/***************************************************************************************
Interrupt service routine called when the counter 1 has reached the compare value
***************************************************************************************/
ISR (TIMER1_COMPA_vect) {   // Compare A : End of Trigger Pulse
    PORTB &= ~(1<<PB1);
    TIMSK1 = (1<<ICIE1)|(1<<TOIE1); // enables the T/C1 Overflow and Capture interrupt;
    TCCR1B = (1<<ICES1);            // Set Positive edge for capture but Don't count yet


}

/******************************************************************************
******************************************************************************/
void Trigger( void ) {      // Config Timer 1 for 10 to 15uS pulse.
    if(MIP == 0) {  // Don't allow re-trigger.
        MIP = 1;                // Set Measurement in progress FLAG
        DDRB |= (1<<PB1);       // PB1 as Output for Trigger pulse.
        DDRB &= ~(1<<PB0);      // PB0 as Input for Input Capture (ECHO).
        
        TCNT1 = 0;              // Clear last Echo times.
        ECHOHighPipe = 0;
        
        OCR1B = 10100;          // 10 mS Post echo Delay
        OCR1A = 12;             // 10 us Trigger length.

        PORTB |= (1<<PB1);      // Start Pulse.

        TIFR1 = 0xFF;           //  Clear all timer interrupt flags
        TCCR1A = 0;   // Timer mode with Clear Output on Match
        TCCR1B = (1<<WGM12) | (1<<CS00) | (1<<CS01);  // Counting with CKio/8 CTC Mode enabled
        TIMSK1 = (1<<OCIE1A);   // enables the T/C1 Overflow, Compare A, and Capture interrupt;
    }
    
}

/******************************************************************************
******************************************************************************/
int main(void){
    uint8_t key;
    
    init_uart();

    sei();
    
    printf("Hello\n\n");

    while (1 == 1) {
    
    key = getchar();
    
    switch(key) {
        case 13:
            printf("\n");
            break;

        case 'T':
        case 't':
            Trigger();
            while (MIP == 1){};
            distance =((((1.0/16000000.0)*64.0*65535.0/58.0*1000000.0))*ECHOHigh) + (4520.0*ECHOLow/65535.0) ; // distance of 1 echohigh in cm
            printf("Echo is %d %d\n\n", ECHOHigh, ECHOLow);
            printf("distance in cm is %d \n", distance);
            break;
        
        }
    }
}
