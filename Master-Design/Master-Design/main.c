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
	DDRB |= (1<< PORTB0) | (1<<PORTB1) | (1<<PORTB2) | (1<<PORTB3);
	//NOTE TO STUDENTS: THE ABOVE ARE NOT CORRECT PINS AND NOT ENOUGH PINS!!!!
	
	//Next, you might want to use built-in pull-up resistors. For example if we
	//decide PINDB2 and PINB3 are inputs, you could enable pull-ups with:
	PORTD = (1<< PORTD5) | (1<< PORTD6) | (1<<PORTD7);
	//NOTE TO STUDENTS: THE ABOVE ARE NOT CORRECT PINS AND NOT ENOUGH PINS!!!!
}

void set_row_low(unsigned int row)
{
	if(row == 0){
		PORTB = ~(1<<PORTB3);
		} else if (row == 1){
		PORTB = ~ (1<<PORTB2);
		} else if (row == 2){
		PORTB = ~ (1<<PORTB1);
		} else if (row == 3){
		PORTB = ~ (1<<PORTB0);
	}
}

int col_pushed(void)
{
	//This code should return what column was detected.
	//Remember the following:
	// 1) We need to mask off such we only get the bit we care about.
	// 2) We need to detect the LOW state (as we are setting the row to be LOW)
	
	//For example, if you wanted to check if PINB2 was low and return '1':
	//if ((PINB & (1<<PINB2)) == 0){
	//    return 1;
	//}
	
	if((PIND & (1<<PIND7)) == 0){
		return 1;
		} else if((PIND & (1<<PIND6)) == 0){
		return 2;
		} else if((PIND & (1<<PIND5)) == 0){
		return 3;
	}


	//If not column detected, return nothing
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
	return 0;
}

char get_new_button(void)
{
	static char last_button;
	char b = get_button();
	
	//Check if we held button down
	if(b == last_button) return 0;
	
	last_button = b;
	
	return b;
}


int main(void)
{
    init_hardware();    
    init_uart(); // initialization
    printf("System Booted hello world hey hey, built %s on %s\n", __TIME__, __DATE__);
    
    char pin[10];
    int i = 0;
    
    while(1){        
        char b = get_new_button();
        
        //Do something special with "#", for example clear partial entry
        if(b == '#'){
            i = 0;
            continue;
        }
        
        char pin[10];
        int i = 0;
        
        while(1){
	        char b = get_new_button();
	        
	        //Do something special with "#", for example clear partial entry
	        if(b == '#'){
		        i = 0;
		        continue;
	        }
	        
	        if(b){
		        pin[i++] = b;
	        }
	        
	        if(i >= 4){
		        pin[4] = 0;
		        printf("Entered PIN: %s\n", pin);
		        i = 0;
	        }
	        
        }
}

