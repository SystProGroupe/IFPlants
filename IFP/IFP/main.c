/*
 * IFP.c
 *
 * Created: 2016-05-23 10:06:45
 * Author : Mattis
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <string.h>		// Declaration for strlen()
 
// to use boolean
typedef int bool;
#define true 1
#define false 0

#define MOISTURE_PIN	0
#define WATERLEVEL_PIN	1
#define F_CPU	16000000
#define BAUD	9600
#define BRC		((F_CPU/16/BAUD) - 1)
#define  TX_BUFFER_SIZE		128 // For buffer TX
#define RX_BUFFER_SIZE		128	// For Buffer RX
#define BUFFER				128

char *strcat(char *dest, const char *src);			// Declaration for strcat()
char * itoa ( int value, char * str, int base );	// Declaration for itao()
void init_WDT();
void init_ADC();
void init_Wifi();
void wakeUp_Measure();
char ReadADC(uint8_t channelno);
void changeADCinput();
void checkMoisture(char value, char level);
void waterThePlant();
void checkWaterLevel(char value);

int count = 405; // so it will start measure when it wakes up
//const int countInterval = 405;	// How long for watchdog to count to. 3600 / 8,9 = 405 -> 1 timme
const int countInterval = 2;
char moistureValue;
char waterLevelValue;
char moistureLeveltoWater;
int plantSize;
bool useDatabase = false;
int ID = 1;


void loginWifi(char SSID[], char Password[]);
void inputToServer(int ID, char moisture, char waterlevel);
char CRLF[] = "\r\n";
char txBuffer[TX_BUFFER_SIZE];
uint8_t txReadPos = 0;
uint8_t txWritePos = 0;

void appendSerial(char c);
void serialWrite(char c[]);

char rxBuffer[RX_BUFFER_SIZE];
uint8_t rxReadPos = 0;
uint8_t rxWritePos = 0;

char getChar(void);
char peekChar(void);

int main(void)
{
	DDRB = 0b00000111;	// Output for LEDs
	DDRD = 0b11110000;	// Output for:
	// Waterpump
	// Wi-fi
	// Water Level Sensor
	// Moisture Sensor
	
	if (useDatabase == true)
		init_Wifi();
	
	moistureLeveltoWater = 100;	// Standard
	plantSize = 3;
	// Sizes of plants:
	// 1 - Extra Small
	// 2 - Small
	// 3 - Medium
	// 4 - Large
	// 5 - Extra Large
	
	init_ADC();
    init_WDT();			//initialize watchdog
    
	while (1) 
    {
    }
}

ISR(WDT_vect)	//Watchdog timeout ISR
{
	sleep_disable();	// Wake up from sleep
	count++;
	
	if (count >= countInterval)
	{
		count = 0;
		
		//PORTB = 0b00000111;
		//_delay_ms(1000);
		//PORTB = 0b00000000;
		
		wakeUp_Measure();
	}
	
	else
		sleep_cpu();
}

void wakeUp_Measure()
{
	// make sure reads from ADC0
	ADMUX = (ADMUX & 0xF8) | (MOISTURE_PIN); // 0xF8 = 0b11111000
	
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	// Measure the moisture
	PORTD = 0b00010000;	// Turn on power to this sensor
	_delay_ms(100);		// Let it wait for 100 ms so the voltage can be noticed by sensor
	moistureValue = ReadADC(MOISTURE_PIN);
	changeADCinput();	// change ADC input
	PORTD = 0b00000000;	// Turn off power to this sensor
	
	_delay_ms(1000);
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	// Measure the water level
	PORTD = 0b00100000;	// Turn on power to this sensor
	//_delay_ms(50);		// Let it wait for 50 ms so the voltage can be noticed by sensor
	waterLevelValue = ReadADC(WATERLEVEL_PIN);
	changeADCinput();	// change ADC input
	PORTD = 0b00000000;	// Turn off power to this sensor
	
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	// Send measured data to server
	// The server will send a message back, if it received the data
	// but also what level of moisture value to water and size of plant
	if (useDatabase == true)
		inputToServer(ID, moistureValue, waterLevelValue);
	
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	// Check how much water is in the tank.
	// GND on the Arduino have a cable to the tank
	// to remove possible charge
	checkWaterLevel(waterLevelValue);
	
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	// Check if the plant needs to be watered
	// Make sure Trigger value is set!!
	checkMoisture(moistureValue, moistureLeveltoWater);
	
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	_delay_ms(1000);
	// Enter sleep mode
	PORTB = 0b0000000;
	PORTD = 0b0000000;
	sleep_cpu();
}


void init_WDT()
{
	MCUSR &= ~(1<<WDRF);
	
	//set up WDT interrupt
	//WDTCSR = (1<<WDCE)|(1<<WDE);
	WDTCSR = 0b00011000;
	
	//Start watchdog timer with 8s prescaller
	//WDTCSR = (1<<WDIE)|(1<<WDP3)|(1<<WDP0);
	WDTCSR = 0b01100001;
	
	//SMCR = 0b00000100;	// Power-down mode
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);	// Set sleep mode
	sleep_enable();
		
	//Enable global interrupts
	sei();
}

void init_ADC()
{
	// configure the ADC module
	ADMUX = 0b01100000;
	// Bit 7:6
	// REFS1:0 = 01 -> AVCC as reference
	// 
	// Bit 5
	// ADLAR = 1 -> Left adjust
	// 
	// Bit 4
	// Reserved = 0
	// 
	// Bit 3:0
	// MUX4:0 = 0000 -> ADC0 as input
	// MUX4:0 = 0001 -> ADC1 as input
	
	ADCSRA = 0b10000111;
	// Bit 7
	// ADEN = 1: enable ADC
	// 
	// Bit 6
	// ADSC = 0: don't start conversion yet
	// ADSC = start conversion
	// 
	// Bit 5
	// ADATE = 0: disable auto trigger
	//
	// Bit 4
	// ADIF = 0: interrupt if it's enabled, see ADIE
	// 
	// Bit 3
	// ADIE = 0: disable ADC interrupt
	//
	// Bit 2:0
	// ASPS2:0 = 111: prescaler = 128
}

char ReadADC(uint8_t channelno)	// Function to read Analog input
{
	ADMUX = (ADMUX & 0xF8) | (channelno); // 0xF8 = 0b11111000
	
	// Start conversion by setting flag ADSC
	// "write this bit to one to start the first conversion"
	ADCSRA |= (1<<ADSC);
	
	// wait until ADC conversion is complete
	while(ADCSRA & (1<<ADSC)) ;
		return ADCH;		// return only higher byte
	
	// The ADC is initialized to put the total 10-bit value in
	// two bytes, placed left-most. as:
	// ADCH: XXXXXXXX
	// ADCL: XX000000
	// Total: XXXXXXXXXX000000
	// Where X is either 1 or 0.
	// But this function only returns the highest 8 bits.
	// We don't need to be more precise than that.
}

void changeADCinput()
{
	// https://sites.google.com/site/qeewiki/books/avr-guide/analog-input
	
	uint8_t tmp;            // temp register for storage of misc data

	tmp = ADMUX;            // read the value of ADMUX register
	tmp &= 0x0F;            // AND the first 4 bits (value of ADC pin being used) ((the 4 leftmost bits))
	
	if (tmp == 0)
		ADMUX++;            // add 1 to ADMUX to go to the next sensor
		
	else if (tmp == 1)
		ADMUX &= 0xF8;      // clear the last 4 bits to reset the MUX to ADC0

}

void checkMoisture(char value, char level)	// First value for measured value, second value is for what level to water
{
	// 255		in air
	// 200~210	in very dry soil
	// 20		just watered
	
	if (value >= level)		// if measured moisture is same or higher (more dry) than the breaking point to water the plant
		waterThePlant(plantSize);
}

void waterThePlant(int size)
{
	//if (useDatabase == true)
		// Post on server
	
	// start the pump, for a set time
	int i;	
	
	int time = size;
	PORTD = 0b10000000;	// Turn on power to the pump

	for (i = 0; i < time; i++)
		_delay_ms(5000);
		
	PORTD = 0b00000000;
}

void checkWaterLevel(char value)
{
	// 0 - 115		0 - 5 mm of water
	// 115 - 130	5 - 20 mm of water
	// 130 <		20 + mm of water
	
	if (value > 130)						// at least 20 mm of water
	PORTB = 0b00000001;	// Green
	
	else if (value <= 130 && value > 115)	// 5-20 mm of water
	PORTB = 0b00000010;	// Yellow

	else if (value <= 115)					// 5 mm or less of water
	PORTB = 0b00000100;	// Red

	else
		PORTB = 0b00000111;
	
	//if (useDatabase == true)
		// Post on server
	
	waterLevelValue = 0;
}


void init_Wifi()
{
	// Finish setting BAUD rate
	UBRR0H = (BRC >> 8);
	UBRR0L = BRC;

	// Setup Transmitter (TX) and Receiver (RX)
	UCSR0B = (1 << TXEN0) | (1 << TXCIE0) | (1 << RXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);	// 8-bit dataframe

	loginWifi("tomas's iPhone", "2az1ftt9rlng");
}

void loginWifi(char SSID[], char Password[])
{
		PORTD = 0b01000000;	// Turn on power to the wi-fi module
		// Then have to wait 10-15 sec for it to start up and connect to network (previous settings)
		// Then we wait for 15 sec
		for(int i = 0; i < 15; i++)
			_delay_ms(1000);

		// Enable the module's mode
		serialWrite("AT+CWMODE=3\r\n");
		_delay_ms(1000)	;
		/*
		// Have to manually enter SSID and Password
		serialWrite("AT+CWJAP=\"Fjallet\",\"0702794209\"\r\n");
		*/
		
		char str1[] = "AT+CWJAP=\"";
		char str2[] = "\",\"";
		char string[] = {0};
		strcat(string, str1);
		strcat(string, SSID);
		strcat(string, str2);
		strcat(string, Password);
		strcat(string, CRLF);
		serialWrite(string);
		
		// Wait for reply:
		// WIFI DISCONNECT
		// WIFI CONNECTED
		// WIFI GOT IP
		//
		// OK
		
		// But can't successfully find more than letter 'W', so have to assume it successfully connected to the network
		
		// Then again have to wait 15 sec for it to connect to network
		// Then we wait for 18 sec
		for(int i = 0; i < 18; i++)
			_delay_ms(1000);
			
		PORTD = 0b00000000;	// Turn on power to the wi-fi module
}


void inputToServer(int ID, char moisture, char waterlevel)
{
	PORTD = 0b01000000;	// Turn on power to the wi-fi module
	// Then again have to wait 15 sec for it to start and connect to network
	// Then we wait for 18 sec
	for(int i = 0; i < 18; i++)
		_delay_ms(1000);
	
	// Enable multiple connections
	serialWrite("AT+CIPMUX=1\r\n");
	_delay_ms(1000);
	
	// Enter what connection, URL and port
	serialWrite("AT+CIPSTART=4,\"TCP\",\"ifp.hopto.org\",80\r\n");
	for(int i = 0; i < 3; i++)
	_delay_ms(1000);
	
	int bytesToSend = 40;
	
	// edit the size of bytes to send, depending of values
	if(moisture > 10)
		bytesToSend++;
	if(moisture > 100)
		bytesToSend++;
		
	if(waterlevel > 10)
		bytesToSend++;
	if(waterlevel > 100)
		bytesToSend++;
	
	// Send how many bytes to send
	char bytesString[2];
	itoa(bytesToSend, bytesString, 10);
	
	char strBytes[] = "AT+CIPSEND=4,";
	char stringBytes[] = {0};
	strcat(stringBytes, strBytes);
	strcat(stringBytes, bytesString);
	strcat(stringBytes, CRLF);
	serialWrite(stringBytes);
	
	_delay_ms(1000);
	_delay_ms(1000);
	
	// Send the message to php
	char IDString[2];
	itoa(ID, IDString, 10);
	
	char moistureString[] = {0};
	itoa((int)moisture, moistureString, 10);
	
	char waterlevelString[] = {0};
	itoa((int)waterlevel, waterlevelString, 10);
		
	char strSend1[] = "GET /testserver/dataIn.php/?id=";
	char strSend2[] = "c";
	char strSend3[] = "c";
	char stringSend[] = {0};
	strcat(stringSend, strSend1);
	strcat(stringSend, IDString);
	strcat(stringSend, strSend2);
	strcat(stringSend, moistureString);
	strcat(stringSend, strSend3);
	strcat(stringSend, waterlevelString);
	strcat(stringSend, CRLF);
	serialWrite(stringSend);
	
	_delay_ms(1000);
	
	// Send empty line
	serialWrite(CRLF);
	
	_delay_ms(1000);
	PORTD = 0b00000000;	// Turn on power to the wi-fi module
}


void appendSerial(char c)
{
	txBuffer[txWritePos] = c;
	txWritePos++;
	
	if(txWritePos >= TX_BUFFER_SIZE)
		txWritePos = 0;
}

void serialWrite(char c[])
{
	for(uint8_t i = 0; i < strlen(c); i++)
	{
		appendSerial(c[i]);
	}
	
	if (UCSR0A & (1 << UDRE0))
		UDR0 = 0;
}

ISR(USART_TX_vect)
{
	if(txReadPos != txWritePos)
	{
		UDR0 = txBuffer[txReadPos];
		txReadPos++;
		
		if(txReadPos >= TX_BUFFER_SIZE)
			txReadPos = 0;
	}
}

char peekChar(void)
{
	char ret = '\0';	// NULL char
	
	if(rxReadPos != rxWritePos)
		ret = rxBuffer[rxReadPos];
	
	return ret;
}

char getChar(void)
{
	char ret = '\0';	// NULL char
	
	if(rxReadPos != rxWritePos)
	{
		ret = rxBuffer[rxReadPos];
		
		rxReadPos++;
		
		if (rxReadPos >= RX_BUFFER_SIZE)
			rxReadPos = 0;
	}
	
	return ret;
}

ISR(USART_RX_vect)
{
	rxBuffer[rxWritePos] = UDR0;
	
	rxWritePos++;
	
	if(rxWritePos >= RX_BUFFER_SIZE)
		rxWritePos = 0;
}
