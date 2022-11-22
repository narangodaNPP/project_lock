/*
 * project_lock.c
 *
 * Created: 06/11/2022 02:49:11 PM
 * Author : Pasan
 */ 

#define F_CPU 16000000UL
#define BAUD 57600
#define UBRR_VALUE ((F_CPU)/(BAUD * 16UL) - 1)

#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// buttons to check the functions
#define non		0
#define upBtn		1
#define downBtn		2
#define okBtn		3
#define cancelBtn	4

// operations for fps 
#define FP_connect	1 // connect to fps
#define FP_find		2 // find fp
#define FP_im1		3 // capture image
#define FP_im2		4 // capture image again
#define FP_cretModl	5 // create template
#define FP_store	6 // store template
#define FP_delete	7 // delete one
#define FP_empty	8 // delete all 
#define FP_search	9 // search fp

// operation status for fps
#define success		0
#define fail		1
#define retry		2

// ports for lcd
#define LcdDataBus PORTA
#define LcdControlBus PORTA

#define LcdDataBusDirnReg DDRA

#define LCD_RS 0
#define LCD_RW 1
#define LCD_EN 2

// ports for solenoid
#define SOLENOID PC1

unsigned char u8_data;

volatile char cont;
volatile char rcvData[15]; // ack package format

// fingerprint scanner 
char sendCmd2Fps(char operation);
char getID();
void enroll();
void search();

// Buttons
void initButtons();
char buttonOperations();

// Solenoid
void initSolenoid();

// LCD
void cmdWriteLCD(char cmd);
void dataWriteLCD(char dat);
void initLCD(void);
void clearDisplay();
void stringLCD(char *str);
void firstLine();
void secondLine();
void xyLCD(int x, int y);

// USART
void usartSendString(char string[16]);
void usartInit();
void usartArray(const char *data, int size);
char usartReceiveChar();
void clearArray(unsigned char *str);
void usartReceiveString(char *str);
void usartSendChar(uint8_t data);

// common sets for commands and acks
const char Header[]		= {0xEF, 0x01};
const char Address[]		= {0xFF, 0xFF, 0xFF, 0xFF};
const char Command[]		= {0x01, 0x00};

//commands Package formats
const char PassVfy[]		= {0x07, 0x13, 0xFF, 0xFF, 0xFF, 0xFF, 0x04, 0x17};
const char fp_detect[]		= {0x3, 0x1, 0x0, 0x5};
const char fp_imz2ch1[]		= {0x4, 0x2, 0x1, 0x0, 0x8};
const char fp_imz2ch2[]		= {0x4, 0x2, 0x2, 0x0, 0x9};
const char fp_createModel[]	= {0x3, 0x5, 0x0, 0x9};
char fp_storeModel[]		= {0x6, 0x6, 0x1, 0x0, 0x1, 0x0, 0xE};
const char fp_search[]		= {0x8, 0x4, 0x1, 0x0, 0x0, 0x0, 0xA3, 0x0, 0xB1};
char fp_delete[]		= {0x7, 0xC, 0x0, 0x0, 0x0, 0x1, 0x0, 0x15};
char fp_empty[]			= {0x3, 0xD, 0x0, 0x11};


int main(void) {
	
	initLCD(); // initialize LCD
	initSolenoid(); // initialize solenoid
	usartInit(); // initialize UART
	initButtons(); // initialize buttons
	
	clearDisplay(); // clear LCD display
	firstLine(); // goto first row of LCD
	stringLCD("WELCOME"); // display welcome message
	secondLine(); // goto second row of LCD
	stringLCD("USER"); // display message
	_delay_ms(2000);
	
	sendCmd2Fps(FP_connect); // password verification command
	_delay_ms(500); // 500ms delay
	
	unsigned char optionsCounter = 0;
	
	clearDisplay(); // clear LCD display
	
	while(1) {
		firstLine();
		stringLCD("Options: ");
		
		if(buttonOperations() == upBtn) {
			optionsCounter++;
		}else if(buttonOperations() == downBtn) {
			optionsCounter--;
		}else if(buttonOperations() == okBtn) {
			if(optionsCounter == 0){
				search(); // search fingerprint
			}else if(optionsCounter == 1) {
				enroll(); // enroll fingerprint
			}else if(optionsCounter == 2) {
				sendCmd2Fps(FP_delete); // send delete command
			}else if(optionsCounter == 3) {
				sendCmd2Fps(FP_empty); // send delete all command
			}
		}
		if(optionsCounter > 3) {
			optionsCounter = 0;
		}

		secondLine();
		
		if(optionsCounter == 0) {
			stringLCD("Scan");
		}else if(optionsCounter == 1) {
			stringLCD("Enroll");
		}else if(optionsCounter == 2) {
			stringLCD("Delete ID");
		}else if(optionsCounter == 3) {
			stringLCD("Erase all IDs");
		}
		
		_delay_ms(100);
	}
}
/*initialize buttons*/
void initButtons() {
	// add ports for buttons
}

/* initialize solenoid */
void initSolenoid() {
	DDRC |= 0x01; // configure as output
}

/* initialize buttons*/
void initButtons(){
	DDRE = 0x0F; //configure as input
	PORTE = 0x0F; //set the pins to internal pull up resistance
}

/*button functions*/
char buttonOperations() {
 	if (PINE & (1<<0) == 0) {
 		return upBtn;
 	}
 	else if(PINE & (1<<1) == 0) {
 		return downBtn;
 	}
 	else if(PINE & (1<<2) == 0) {
 		return okBtn;
 	}
 
 	else if(PINE & (1<<3) == 0) {
 		return cancelBtn;
 	}
 	
 	else {
 		return non;
 	}	
	return okBtn;
}

/* send commands to fps */
char sendCmd2Fps(char operation) {
	unsigned char successed = retry;
	while (successed == retry) {
		
		/*send commands to the FPS*/
		clearDisplay();	
		switch (operation) {
			case FP_connect:
			stringLCD("Searching for ");
			secondLine();
			stringLCD("fingerprint");
			break;
			
			case FP_find:
			stringLCD("Place your ");
			secondLine();
			stringLCD("Finger");
			break;
			
			case FP_im1:
			stringLCD("Processing..");
			break;
			
			case FP_im2:
			stringLCD("Processing..");
			break;
			
			case FP_cretModl:
			stringLCD("Creating Model");
			break;
			
			case FP_empty:
			stringLCD("ID is Erasing");
			break;
			
			case FP_search:
			stringLCD("Searching...");
			break;
			
			case FP_store:
			stringLCD("Storing Model");
			break;
			
			case FP_delete:
			stringLCD("Delete Finger");
			break;
		}
		
		/* send commands to fps */
		// these bits are common 
		usartArray(&Header,2);
		usartArray(&Address,4);
		usartArray(&Command,2);
		
		int arr_size;
		unsigned char ID;
		
		/* Commands to fingerprint module */
		switch (operation) {
			case FP_connect:
				arr_size = (sizeof(PassVfy)) / sizeof((PassVfy)[0]);
				usartArray(&PassVfy,arr_size);
				break;
			
			case FP_find:
				arr_size = (sizeof(fp_detect)) / sizeof((fp_detect)[0]);
				usartArray(&fp_detect,arr_size);
				break;
			
			case FP_im1:
				arr_size = (sizeof(fp_imz2ch1)) / sizeof((fp_imz2ch1)[0]);
				usartArray(&fp_imz2ch1,arr_size);
				break;
			
			case FP_im2:
				arr_size = (sizeof(fp_imz2ch2)) / sizeof((fp_imz2ch2)[0]);
				usartArray(&fp_imz2ch2,arr_size);
				break;
			
			case FP_cretModl:
				arr_size = (sizeof(fp_createModel)) / sizeof((fp_createModel)[0]);
				usartArray(&fp_createModel,arr_size);
				break;
			
			case FP_empty:
				arr_size = (sizeof(fp_empty)) / sizeof((fp_empty)[0]);
				usartArray(&fp_empty,arr_size);
				break;
			
			case FP_search:
				arr_size = (sizeof(fp_search)) / sizeof((fp_search)[0]);
				usartArray(&fp_search,arr_size);
				break;
			
			case FP_store:
				ID = getID();
				fp_storeModel[4] = ID;
				fp_storeModel[6] = (0xE + ID);
				arr_size = (sizeof(fp_storeModel)) / sizeof((fp_storeModel)[0]);
				usartArray(&Header ,2);
				usartArray(&Address,4);
				usartArray(&Command,2);
				usartArray(&fp_storeModel,arr_size);
				break;
			
			case FP_delete:
				ID = getID();
				fp_delete[3] = ID;
				fp_delete[7] = (0x15 + ID);
				arr_size = (sizeof(fp_delete)) / sizeof((fp_delete)[0]);
				usartArray(&Header,2);
				usartArray(&Address,4);
				usartArray(&Command,2);
				usartArray(&fp_delete,arr_size);
				break;
		}
		_delay_ms(1000);
		
		/*feedback messages from FPS (Ack packages)*/
		if(cont>1) {
			if(rcvData[6] == 0x07 && (rcvData[8] == 0x03 || rcvData[8] == 0x07)) {
				secondLine();
			
				// 00h: commad execution complete;
				if(rcvData[9] == 0x00) {
					successed = success;
				}
				else {
					successed = fail;
					
					// 01h: error when receiving data package;
					if(rcvData[9] == 0x01) {
						stringLCD("Packet error")
					;}
					
					// 04h: fail to generate character file due to the over-disorderly fingerprint image;
					else if(rcvData[9] == 0x04) {
						stringLCD("failed!");
					}
					
					// 05h: fail to generate character file due to the over-wet fingerprint image;
					else if(rcvData[9] == 0x05) {
						stringLCD("Failed!");
					}
					
					// 06h: ail to generate character file due to the over-disorderly fingerprint image;
					else if(rcvData[9] == 0x06) {
						stringLCD("Failed!");
					}
					
					// 07h: fail to generate character file due to lackness of character point or over-smallness of fingerprint image
					else if(rcvData[9] == 0x07) {
						stringLCD("Failed!");
					}
					
					// 09h: fail to find the matching finger;
					else if(rcvData[9] == 0x09) {
						stringLCD("ID not Found!");
					}
					
					// 0Bh: addressing PageID is beyond the finger library;
					else if(rcvData[9] == 0x0B) {
						stringLCD("ID overload!");
					}
					
					// 18h: error when writing flash;
					else if(rcvData[9] == 0x18) {
						stringLCD("Flash error!");
					}
					
					// 0Ah: fail to combine the character files;
					else if(rcvData[9] == 0x0A) {
						stringLCD("Failed!");
					}
					
					// 13h: wrong password!
					else if(rcvData[9] == 0x13) {
						stringLCD("Incorrect PW!");
					}
					
					// 21h: password not verified					
					else if(rcvData[9] == 0x21) {
						stringLCD("Verify PW!");
					}
					
					else {
						// 02h: no finger on the sensor;
						if (rcvData[9] == 0x02) {
							stringLCD("No Finger!");
						}
						
						// 03h: fail to enroll the finger;
						else if(rcvData[9] == 0x03) {
							stringLCD("Enroll Fail!");
						}
						
						// if no defined acks
						else {
							stringLCD("Failed!!");
						}
						successed = retry;
					}
				}
			}
			/* if no acks received */
			else {
				clearDisplay();
				stringLCD("FP module");
				secondLine();
				stringLCD("connection Error");
			}
		}
		/* if no command were send to fps */
		else {
			clearDisplay();
			stringLCD("FP not Found or");
			secondLine();
			stringLCD("Not Responding ");
			_delay_ms(1000);
		}
		
		/* give feedback massage to LDC display */
		if(successed == success) {
			unsigned char String[20];
			
			switch (operation) {
				case FP_connect:
					stringLCD("Module Found");
					break;
				
				case FP_find:
					stringLCD("Finger Captured");
					break;
				
				case FP_im1:
					stringLCD("Done");
					break;
				
				case FP_im2:
					stringLCD("Done");
					break;
				
				case FP_cretModl:
					stringLCD("Model Created");
					break;
				
				case FP_empty:
					stringLCD("Memory Erased");
					break;
				
				case FP_search:
					stringLCD("Found in ID: ");
					sprintf(String," %d  ",rcvData[11]);
					stringLCD(&String);
					/* issue signal for solenoid */
					if (PINC & 0x01) {
						PORTC &= ~(1<<SOLENOID); // close
					}else{
						PORTC |= (1<<SOLENOID); // open
					}
					break;
				
				case FP_store:
					stringLCD("Saved Successful");
					_delay_ms(700);
					break;
				
				case FP_delete:
					sprintf(String,"ID: %d Deleted",ID);
					stringLCD(&String);
					_delay_ms(700);
					break;
			}
		}
		clearArray(&rcvData);
		cont = 0;
		_delay_ms(700);
	}
	return successed;
}

/* USARt */
void usartInit() {
	/*
			  U2Xn = 0		   U2Xn = 1
			UBRR	Error	UBRR	Error
	57.6K	16		2.1%	 34		-0.8%
	
	*/
	UBRR1H = (UBRR_VALUE>>8); // UBRR1H = 0
	UBRR1L = UBRR_VALUE; // UBRR1L = 16
	UCSR1B = (1<<RXEN1)|(1<<TXEN1)|(1<<RXCIE1); // receiver and transmitter enable and rx complete interrupt enable
	UCSR1C |= (1<<UCSZ10)|(1<<UCSZ11); // UCSZ10 = UCSZ11 = 1 to set 8 bit
	UCSR1B &= ~(1<<UCSZ12); // UCSZ02 = 0 to set 8 bit mode
	
	sei(); // globally initialize interrupt
}

/* receive interrupt vector */
ISR(USART1_RX_vect) {
	rcvData[cont++] = UDR1;
	UCSR1A |= (1<<RXCIE1);
}

/* receive character */
char usartReceiveChar() {
	while (!(UCSR1A & (1<<RXC1))); /// checked if received, if rx_complete = 1
	return UDR1;
}

/* receiving string of characters */
void usartReceiveString(char *str) {
	int i = 0;
	while (str[i] != "\r") {
		str[i] = usartReceiveChar();
		i++;
	}
	str[i] = "\0";
}

/* send character */
void usartSendChar(uint8_t data) {
	while (!(UCSR1A & (1 << UDRE1))); // wait for empty transmit buffer
	UDR1 = data; // put data into buffer, sends the data
}

/* send string of characters */
void usartSendString(char *str) {
	unsigned char i;
	while (str[i] != "\0" ) {
		usartSendChar(str[i]);
		i++;
	}
}

/* array with array size for send commands*/
void usartArray(const char *data, int size) {
	for(int i = 0; i<size; i++) {
		usartSendChar(*data);
		*data++;
	}
}

/* clear command array */
void clearArray(unsigned char *str) {
	while(*str) {
		*str = 0;
		*str++;
	}
}

/* get id */
char getID() {
	clearDisplay();
	unsigned char ids = 1;
	char ok = 0;
	stringLCD("Enter ID:");
	while(ok == 0) {
		if(buttonOperations() == upBtn) {
			ids++;
		}
		
		else if(buttonOperations() == downBtn) {
			ids--;
		}
		
		else if(buttonOperations() == okBtn) {
			ok = 1;
		}
		
		if(ids >= 128) {
			ids = 1;
		}
		
		unsigned char String[20];
		
		xyLCD(1,10);
		sprintf(String," %d  ",ids);
		stringLCD(&String);
		_delay_ms(200);
	}
	return ids;
}

/* enroll new user */
void enroll() {
	loop:
	if(sendCmd2Fps(FP_find)) goto loop;
	if(sendCmd2Fps(FP_im1)) goto loop;
	if(sendCmd2Fps(FP_find)) goto loop;
	if(sendCmd2Fps(FP_im2)) goto loop;
	if(sendCmd2Fps(FP_cretModl)) goto loop;
	if(sendCmd2Fps(FP_store)) goto loop;
}

/* search id */
void search() {
	loop:
	if(sendCmd2Fps(FP_find)) goto loop;
	if(sendCmd2Fps(FP_im1)) goto loop;
	if(sendCmd2Fps(FP_search)) goto loop;
}


/* pass 1 byte of commands to lcd */
void cmdWriteLCD(char cmd) {
	LcdDataBus = (cmd & 0xF0); // Set upper 4 bits of the cmd
	LcdControlBus &= ~(1<<LCD_RS); // RS = 0
	LcdControlBus &= ~(1<<LCD_RW); // RW = 0 --> write operation
	LcdControlBus |= (1<<LCD_EN); // enable bit
	
	_delay_ms(10); // 10ms delay
	
	LcdControlBus &= ~(1<<LCD_EN); // finish write operation
	
	_delay_ms(10); // 10ms delay
	
	LcdDataBus = ((cmd<<4) & 0xF0); // Set lower 4 bits of the cmd
	LcdControlBus &= ~(1<<LCD_RS); // RS = 0
	LcdControlBus &= ~(1<<LCD_RW); // RW = 0 --> write operation
	LcdControlBus |= (1<<LCD_EN); // enable bit
	
	_delay_ms(10); // 10ms delay
	
	LcdControlBus &= ~(1<<LCD_EN); // finished write operation
	
	_delay_ms(10); // 10 ms delay
}

/*pass 1 byte of data into the data register*/
void dataWriteLCD(char dat) {
	LcdDataBus = (dat & 0xF0); // Set lower 4 bits of the data
	LcdControlBus |= (1<<LCD_RS); // RS = 1
	LcdControlBus &= ~(1<<LCD_RW); // RW = 0 --> write operation
	LcdControlBus |= (1<<LCD_EN); // enable bit
	
	_delay_ms(10); // 10ms delay
	
	LcdControlBus &= ~(1<<LCD_EN); // finished write operation
	
	_delay_ms(10); // 10ms delay
	
	LcdDataBus = ((dat<<4) & 0xF0); //Set lower 4 bits of the data
	LcdControlBus |= (1<<LCD_RS); // RS = 1
	LcdControlBus &= ~(1<<LCD_RW); // RW = 0 --> write operation
	LcdControlBus |= (1<<LCD_EN); // enable bit
	
	_delay_ms(10); // 10ms delay
	
	LcdControlBus &= ~(1<<LCD_EN); // finished write operation
	
	_delay_ms(10); // 10ms delay
}

/*display string in LCD*/
void stringLCD(char *str) {
	int length = strlen(str);
	for (int i=0; i<length; i++) {
		dataWriteLCD(str[i]);
	}
}

/*clear display*/
void clearDisplay(void) {
	cmdWriteLCD(0x01); // clear display screen
}

/*cursor move to line 1*/
void firstLine(void) {
	cmdWriteLCD(0x80); // 1st line
}

/*cursor move to line 2*/
void secondLine(void) {
	cmdWriteLCD(0xC0); // 2nd line
}

/*display specific place*/
void xyLCD(int x, int y) {
	y--;
	if (x==1) {
		cmdWriteLCD(0x80+y);
	}
	if (x==2) {
		cmdWriteLCD(0xC0+y);
	}
}

/*initialize LCD*/
void initLCD(void) {
	LcdDataBusDirnReg = 0xFF;
	cmdWriteLCD(0x02); // return home
	cmdWriteLCD(0x28); // 2 lines and 5x7 matrix
	cmdWriteLCD(0x0E); // Display on, cursor blinking
	cmdWriteLCD(0x01); // Clear Display
	cmdWriteLCD(0x80); // 1st line
}
