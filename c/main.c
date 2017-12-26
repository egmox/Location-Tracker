/*
 * GsmGprsGpsATmega16.c
 *
 * Created: 11/14/2016 3:40:38 PM
 * Author : HBLS05
 */ 

#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "alcd.h"
#include "usart.h"
#include "virtual_uart.h"

#define TIMEOUT		1500
#define CALL_RUN	59
#define RETRY_COUNT 3                       // MAXIMUM RETRY COUNT
#define CALL_DELAY  300                     // Duration of every call

#ifndef ClearBit
#define ClearBit(reg, bit)    reg &= ~(1<<bit) //clear bit
#endif

#ifndef SetBit
#define SetBit(reg, bit)    reg |= (1<<bit) //set bit
#endif

#ifndef ReadBit
#define ReadBit(reg, bit)    (reg & (1<<bit)) //read bit
#endif

uint8_t receivedData=0;
// UART3 Global Registers
char u3_buffer ; // serial buffer
uint8_t u3_bit_nbr; // bit counter
char u3_status ; // status register
// UART3 Receiver buffer
char rx_buffer3[RX_BUFFER_SIZE3];
uint8_t rx_wr_index3, rx_rd_index3, rx_counter3;
// This flag is set on UART3 Receiver buffer overflow
uint8_t rx_buffer_overflow3,rx=0;

void softUartInit(void) ;
void softUartPutchar(char c);
char softUartGetchar(void);
void softUartPuts(char *data);

char str[200];
// char lo[20]="7731.8718,E,";
// char la[20]="2825.2649,N,";
//char str[100] = {"AT+HTTPPARA=\"URL\",\"162.144.48.118/gps/s.php?f=$0003$%200004814303$7731.8718,E,$2825.2649,N,$0004$\""};

void flushString(char *str, uint8_t len);
uint8_t send_cmd(PGM_P cmd, char *resp, unsigned char retry_count);
void getRespons(char *data , uint8_t len, uint16_t timeout);
void initModem(void);
void initGprs(void);
uint8_t txGprs(char *data);

//PGM_P const str_table[] PROGMEM =
//{
	//"1234","1235","1236","1237"
//};
char *str_table[]  =
{
	"%200000000001","%200000000002","%200000000003","%200000000004"
};

char lon[] =" LONGITUDE: ";
char lat[] =" LATITUDE: ";
char c[200],lo[20],la[20];
uint8_t j,k,counter=0;

// External Interrupt 0 service routine (used to initiate UART3 receive)
ISR(INT2_vect)
{
	if (counter<3)	
		counter++;
	else
		counter=0;
	while(!(PINB & (1<<PB3)));			
}



int main(void)
{    
	uint8_t retry_count=0,ret,i;
	uint16_t j=0;
	char buffer[30];
	DDRB = (0<<PB2);
	PORTB = (1<<PB2);
	GICR |= (1<<INT2);
	MCUCSR=(0<<ISC2);
	GIFR |= (1<<INTF2);	
	lcd_init();
	usart_init(9600);
	
	
	
	lcd_puts_P(PSTR("     DEVICE     "));
	lcd_gotoxy(0,1);
	lcd_puts_P(PSTR("  INITIALIZING  "));
	_delay_ms(20000);
	initModem();
	lcd_clear();
	lcd_puts_P(PSTR("     DEVICE     "));
	lcd_gotoxy(0,1);
	lcd_puts_P(PSTR("   INITIALIZED  "));
	_delay_ms(1000);
	
	softUartInit();
	sei();
	
    while (1) 
    {
		rx_wr_index3 = 0;	
//		rx_rd_index3 = 0;	
//		rx_counter3=0;
		for(i=0;i<200;i++)
		{
			c[i]=0x00; // just send it back !
			rx_buffer3[i]=0x00;
		}
		softUartInit();
	    sei();
		j=0;
		lcd_gotoxy(0,0);
		lcd_puts_P(PSTR("    *WAIT*     "));
		lcd_gotoxy(0,1);
		lcd_puts_P(PSTR("  *NEW DATA*   "));
		_delay_ms(10000);
		
		for(i=0;i<200;i++)
		{
			c[i]=rx_buffer3[i]; // just send it back !			
		}
		
		for(i=0;i<200;i++)
		{
			if(c[i+0]=='G' && c[i+1]=='P' && c[i+2]=='G' && c[i+3]=='G'  && c[i+4]=='A')
			{
				if(c[i+21]=='.' && c[i+26]==',' && c[i+27]=='N' && c[i+28]==',')
				{
					lcd_clear();
					lcd_puts(lat);
					_delay_ms(50);
					lcd_gotoxy(0,1);
					_delay_ms(50);
					
					k=0;
					for(j=17;j<29;j++)
					{
						la[k]=c[i+j];
						lcd_putchar(c[i+j]);
						k++;
					}
					la[k]='\0';
					_delay_ms(1500);
					
					lcd_clear();
					lcd_gotoxy(0,0);
					lcd_puts(lon);
					_delay_ms(50);
					lcd_gotoxy(0,1);
					_delay_ms(50);
					
					k=0;
					for(j=30;j<42;j++)
					{
						lo[k]=c[i+j];
						lcd_putchar(c[i+j]);
						k++;
					}
					lo[k]='\0';
					
					retry:
					strcpy_P(str,PSTR("AT+HTTPPARA=\"URL\",\"162.144.48.118/gps/s.php?f=$0003$"));
					strcat(str,(str_table[counter]));
					strcat_P(str,PSTR("$"));
					strcat(str,la);
					strcat_P(str,PSTR("$"));
					strcat(str,lo);
					strcat_P(str,PSTR("$0004$"));
					strcat_P(str,PSTR("\""));
					
					i=send_cmd(PSTR("AT+SAPBR=2,1"),",1",RETRY_COUNT);    // Check the registration of gprs with network
					_delay_ms(100);
					if(i == 0)
					initGprs();
					_delay_us(1000);
					usart_puts(str);
					_delay_ms(10);
					usart_putc(0x0D);
					_delay_ms(1);
					getRespons(buffer,50,5000);
					previous:
					retry_count++;
					_delay_ms(100);                             // Receiving Response
					for(i=0;i<100;i++){                          //command to receive data
						if(buffer[i]=='O' && buffer[i+1]=='K'){
							goto next;
						}
					}
					if(retry_count<50)
					goto previous;
					
					next:
					if(send_cmd(PSTR("AT+HTTPACTION=0"),"OK",RETRY_COUNT)==1)
					{
						previous1:
						retry_count++;
						getRespons(buffer,30,5000);				// Receiving Response
						for(i=0;i<30;i++){                      //command to receive data
							if(buffer[i]=='+' && buffer[i+1]=='H' && buffer[i+2]=='T' && buffer[i+10]=='N'  && buffer[i+14]=='2'){
								goto next1;
							}
						}
						if(retry_count<50)
						goto previous1;
					}
					next1:
					ret= send_cmd(PSTR("AT+HTTPREAD"),"In",RETRY_COUNT);
					_delay_ms(100);
					retry_count++;
					if(ret==1)
					{
						lcd_clear();
						lcd_gotoxy(0,0);
						lcd_puts_P(PSTR("  *DATA SENT*   "));
						lcd_gotoxy(0,1);
						lcd_puts_P(PSTR("  SUCESSFULLY   "));
						retry_count=0;
						_delay_ms(5000);
					}
					else
					{
						lcd_clear();
						lcd_puts_P(PSTR("  *DATA SENT*   "));
						lcd_gotoxy(0,1);
						lcd_puts_P(PSTR("     FAILED     "));
						_delay_ms(2000);
					}
				}
				else
				{
					lcd_clear();
					lcd_gotoxy(0,0);
					lcd_puts_P(PSTR("  *GPS DATA*   "));
					lcd_gotoxy(0,1);
					lcd_puts_P(PSTR("  NOT SYNC OK   "));
					_delay_ms(2000);
				}
										
			}			
		}		
    }
}

// External Interrupt 0 service routine (used to initiate UART3 receive)
//interrupt [EXT_INT0] void external_INT0(void)
ISR(INT0_vect)
{
	TCNT0 = 0x00;
	OCR0  = START_BIT_LENGTH;       // set timer reload value (to 1.5 bit length)	
	TIFR  |= (1<<OCF0);             // clear timer compare flag	
	TIMSK |= (1<<OCIE0);            // enable timer compare interrupt
	/*****************************************************************************/	
	GICR &= ~(1<<INT0);				// disable external interrupt INT0 INT0
	/*****************************************************************************/
	u3_status  = (1<<RX_BUSY);		// set RX busy flag (clear all others)
	u3_buffer  = 0x00;				// clear UART3 buffer
	u3_bit_nbr = 0xFF;				// erase bit counter (set all bits)	
	rx=0;
}

// Timer 0 compare interrupt service routine (sends & receives the UART3 bits)
ISR(TIMER0_COMP_vect)
{	
	TCNT0 = 0x00;
	OCR0  = DATA_BIT_LENGTH;
	u3_bit_nbr++;
	
	/*** check what are we doing: send or receive ? ***/
	if(u3_status & (1<<TX_BUSY))     // transmit process
	{
		if(u3_bit_nbr < 8)              // data bits (bit 0...7)
		{
			if( ReadBit(u3_buffer, 0) )
				SetBit(PORTD, TX3_PIN);
			else
				ClearBit(PORTD, TX3_PIN);			
			u3_buffer >>= 1;             // next bit, please !
		}
		else                           // stop bit (bit #8)
		{
			SetBit(PORTD, TX3_PIN);			
			if(u3_bit_nbr == UART3_STOP) // ready! stop bit(s) sent
			{
				TIMSK &= ~(1<<OCIE0);	// disable timer 0 interrupt
				u3_status = 0x00;       // clear UART3 status register
				GICR |= (1<<INT0);      // enable external interrupt
			}
		}
	}
	/***********************************************************/
	else // receive process (1<<RX_BUSY)	
	{
		if(u3_bit_nbr < 8)            // data bits (bit 0...7)
		{					
			if(PIND & (1<<RX3_PIN))			
				u3_buffer |= (1<<(u3_bit_nbr)); // sample here !
			else
				u3_buffer |= (0<<(u3_bit_nbr)); // sample here !								
		}
		else                             // ready! it's the stop bit
		{			
			//usart_putc(u3_buffer);
			TIMSK &= ~(1<<OCIE0);		// disable timer 0 interrupt
			u3_status = 0x00;          // clear UART3 status register													
			// test the stop bit: (RX3_PIN == HIGH)			
			if( ReadBit(PIND, RX3_PIN) )	// if no frame error...		
			{
				rx_buffer3[rx_wr_index3] = u3_buffer;				
				if(++rx_wr_index3 == RX_BUFFER_SIZE3){
					rx_wr_index3 = 0;					
				}
				if(++rx_counter3 > RX_BUFFER_SIZE3)
				rx_buffer_overflow3 = TRUE;
			}
					        
			GICR |= (1<<INT0);			// enable external interrupt						
		}
	}
}

// get a character stored in the UART3 RX-buffer
char softUartGetchar(void)
{
	char data;
// 	while(u3_bit_nbr < 8)
//  	while(rx_counter3 == 0)
//  	wdogtrig();
 	//while(rx == 0)
 	//{
 		////usart_putc('a');	
 	//}	
	data = rx_buffer3[rx_rd_index3];
	if(++rx_rd_index3 == RX_BUFFER_SIZE3)
	rx_rd_index3 = 0;
	cli();
	--rx_counter3;
	sei();
	return data;	
}

// put a character DIRECTLY to the UART3 buffer (it will CANCEL the RX flow)
void softUartPutchar(char c)
{
	while(u3_status & (1<<TX_BUSY)); // wait while UART3 is busy with sending
	/************************/	
	GICR &= ~(1<<INT0);					// disable external interrupt
	/************************/
	ClearBit(PORTD, TX3_PIN);			// clear output (start bit)	
	/************************/
	TCNT0 = 0x00;
	OCR0  = DATA_BIT_LENGTH;			// set timer reload value (1 bit)
	TIFR  |= (1<<OCF0);                 // clear timer compare flag
	TIMSK |= (1<<OCIE0);                // enable timer compare interrupt
	/************************/
	u3_status  = (1<<TX_BUSY);          // set TX busy flag (clear all others)
	u3_buffer  = c;                     // copy data to buffer
	u3_bit_nbr = 0xFF;                  // erase bit counter (set all bits)
}

// UART3 initialization function (we MUST call this function BEFORE using UART3)
void softUartInit(void) // UART3 = software UART
{
	//RX	
	PORTD |= (1<<RX3_PIN);		// pull-up
	DDRD  |= (0<<RX3_PIN);		// input	
	//TX
	PORTD |= (1<<TX3_PIN);		// logic 1 (high level = STOP TX)
	DDRD  |= (1<<TX3_PIN);		// output	

	// Timer/Counter 0 initialization
	// Clock source: System Clock
	// Clock value: 500,000 kHz
	// Mode: CTC top = OCR0
	// OC0 output: Disconnected
	ASSR =0x00;
	TCCR0=0x0A;

	// External Interrupt 0 initialization (for RX)
	MCUCR |= (0<<ISC01) | (0<<ISC00);;
	GICR |= (1<<INT0);
	GIFR |= (1<<INTF0);
	
	// Timer 0 Output Compare Match Interrupt disable
	TIMSK &= ~(1<<OCIE0);		
}

void softUartPuts(char *data)
{
	unsigned int i=0;
	while(data[i]!='\0')
	{
		softUartPutchar(data[i]);
		_delay_ms(1);
		i++;
	}	
}

/*************************************************************************
Function: send_cmd()
Purpose:  send GSM AT commands and compare its response
**************************************************************************/
unsigned char send_cmd(PGM_P cmd, char *resp, unsigned char retry_count)
{
	uint8_t i,j=0;
	char buffer[30];
	retry:
	usart_puts_P(cmd);
	_delay_ms(10);
	usart_putc(0x0D);
	_delay_ms(1);
	getRespons(buffer,50,5000);
	for(i=0;i<50;i++)
	{
		if((buffer[i] == resp[0]) && (buffer[i+1] == resp[1]))
		{
			return 1;
		}
		else if((buffer[i]=='+') && (buffer[i+1]=='C') && (buffer[i+2]=='M') && (buffer[i+3]=='S') && (buffer[i+5]=='E') && (buffer[i+6]=='R'))
		{
			j++;
			if(j<retry_count)
			{
				_delay_ms(100);
				goto retry;
			}
			else
			return 0;
		}
		else if((buffer[i]=='+') && (buffer[i+1]=='C') && (buffer[i+2]=='M') && (buffer[i+3]=='E') && (buffer[i+5]=='E') && (buffer[i+6]=='R'))
		{
			j++;
			if(j<retry_count)
			{
				_delay_ms(100);
				goto retry;
			}
			else
			return 0;
		}
	}
	j++;
	if(j<retry_count){
		//_delay_ms(500);
		goto retry;
	}
	else
	return 0;
}

void getRespons(char *data , uint8_t len, uint16_t timeout)
{
	uint8_t i;
	uint16_t j;
	for(i=0;i<len;i++){                      // Command to receive data
		j=0;
		while ((UCSRA & (1<<RXC))==0){
			if(j>=timeout)
			goto out;
			_delay_us(50);                 // need to be adjusted in case communication fails with modem
			j++;
		}
		data[i]=UDR;
	}
	out:
	data[i]='\0';
	_delay_us(100);
}

void initModem(void)
{
	//uint8_t count=0;
	//restart:
	send_cmd(PSTR("AT"),"OK",RETRY_COUNT);			// sending AT  to chech connectivity
	_delay_ms(500);
	send_cmd(PSTR("ATE0"),"OK",RETRY_COUNT);			// sending ATE0  to turn off echo
	_delay_ms(500);
	send_cmd(PSTR("AT+IPR=9600"),"OK",RETRY_COUNT);		// Baud rate = 38400
	_delay_ms(500);
	send_cmd(PSTR("AT+CMGF=1"),"OK",RETRY_COUNT);		// sending AT+CMGF  to set the text mode
	_delay_ms(500);
	send_cmd(PSTR("AT+CPMS=\"SM\",\"SM\",\"SM\";&W"),"OK",RETRY_COUNT); //Enable preferred SMS Message Storage
	_delay_ms(500);
	send_cmd(PSTR("AT+CMGD=1,4"),"OK",RETRY_COUNT);			// sending AT+CMGD to delete SensorMsgs
	_delay_ms(500);
	
	send_cmd(PSTR("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""),"OK",RETRY_COUNT);  // sending AT+CIICR
	_delay_ms(500);
	send_cmd(PSTR("AT+SAPBR=3,1,\"APN\",\"internet\""),"OK",RETRY_COUNT);
	_delay_ms(500);
	
	send_cmd(PSTR("AT+SAPBR=1,1"),"OK",RETRY_COUNT);
	_delay_ms(1000);
	send_cmd(PSTR("AT+SAPBR=2,1"),",1",RETRY_COUNT);                  // sending AT+CIICR
	_delay_ms(500);
	send_cmd(PSTR("AT+HTTPINIT"),"OK",RETRY_COUNT);                   // sending AT+CIFSR
	_delay_ms(500);
	send_cmd(PSTR("AT+HTTPPARA=\"CID\",1"),"OK",RETRY_COUNT);         // sending AT+CIFSR
	_delay_ms(500);
}

void flushString(char *str, uint8_t len)
{
	uint8_t i;
	for (i=0;i<len;i++)
	{
		str[i]=0xFF;
	}
}

void initGprs(void)
{
	/*uint8_t i;*/
	send_cmd(PSTR("AT+CREG?"),",1",RETRY_COUNT);                        // sending AT+CGATT=1
	_delay_ms(500);
	send_cmd(PSTR("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""),"OK",RETRY_COUNT);
	_delay_ms(500);
	send_cmd(PSTR("AT+SAPBR=3,1,\"APN\",\"internet\""),"OK",RETRY_COUNT);
	_delay_ms(500);
	// 	strcpy_P(str,PSTR("AT+SAPBR=3,1,\"APN\",\""));
	// 	strcat_P(str,PSTR("internet"));                                                        //set apn here
	// 	strcat_P(str,PSTR("\""));
	// 	usart_puts(str);
	// 	usart_putc(0x0D);
	// 	getRespons(rx_buffer,10,100);
	// 	for (i=0;i<10;i++)
	// 	{
	// 		if(rx_buffer[i]=='O' && rx_buffer[i+1]=='K'){
	// 			break;
	// 		}
	// 	}
	_delay_ms(500);
	send_cmd(PSTR("AT+SAPBR=1,1"),"OK",RETRY_COUNT);                    // sending AT+CIFSR
	_delay_ms(500);
	send_cmd(PSTR("AT+SAPBR=2,1"),"OK",RETRY_COUNT);                    // sending AT+CIICR
	_delay_ms(500);
	send_cmd(PSTR("AT+HTTPINIT"),"OK",RETRY_COUNT);                     // sending AT+CIFSR
	_delay_ms(500);
	send_cmd(PSTR("AT+HTTPPARA=\"CID\",1"),"OK",RETRY_COUNT);           // sending AT+CIFSR
	_delay_ms(500);
}

uint8_t txGprs(char *dat)
{
	uint8_t i=0,retry_count=0;
	char buffer[30];
	
	i=send_cmd(PSTR("AT+SAPBR=2,1"),",1",RETRY_COUNT);                   // Check the registration of gprs with network
	_delay_ms(100);
	if(i == 0)
	initGprs();
	_delay_us(1000);
	usart_puts(str);
	_delay_ms(10);
	usart_putc(0x0D);
	_delay_ms(1);
	getRespons(buffer,50,5000);
	previous:
	retry_count++;
	_delay_ms(100);                             // Receiving Response
	for(i=0;i<100;i++){                          //command to receive data
		if(buffer[i]=='O' && buffer[i+1]=='K')
		{
			goto next;
		}
	}
	if(retry_count<50)
	goto previous;
	
	next:
	if(send_cmd(PSTR("AT+HTTPACTION=0"),"OK",RETRY_COUNT)==1)
	{
		previous1:
		retry_count++;
		getRespons(buffer,30,5000);				// Receiving Response
		for(i=0;i<30;i++){                      //command to receive data
			if(buffer[i]=='+' && buffer[i+1]=='H' && buffer[i+2]=='T' && buffer[i+10]=='N'  && buffer[i+14]=='2'){
				goto next1;
			}
		}
		if(retry_count<50)
		goto previous1;
	}
	next1:
	i= send_cmd(PSTR("AT+HTTPREAD"),"In",RETRY_COUNT);
	_delay_ms(500);
	return i;
}