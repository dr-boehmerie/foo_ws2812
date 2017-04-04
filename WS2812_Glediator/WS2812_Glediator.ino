
//##############################################################################
//##############################################################################
//                                                                             #
// Glediator to WS2812 pixel converter                                         #
// by R. Heller                                                                #
// V 1.0 - 07.01.2014                                                          #            
// wwww.SolderLab.de                                                           #
//                                                                             #
// Receives serial data in Glediator protocol format @ 1 MBit/s                #
// and distributes it to a connectect chain of WS2812 pixels                   #
//                                                                             #
// Adjust the correct DATA PIN and the correct NUMBER OF PIXELS you are using  # 
// in the definitions section below before uploading this sketch to your       #
// Arduino device.                                                             #
//                                                                             #
// Maxiumum number of supported pixels is 512 !!!                              #
//                                                                             #
// In the Glediator software set output mode to "Glediator_Protocol",          #
// color order to "GRB" and baud rate to "1000000"                             #
// pixel order to HL_TL (Horizontal linear, Top left?)                         #
//                                                                             #
//##############################################################################
//##############################################################################


//##############################################################################
//                                                                             #
// Definitions --> Make changes ONLY HERE                                      #
//                                                                             #
// To find out the correct port, ddr and pin name when you just know the       #
// Arduino's digital pin number just google for "Arduino pin mapping".         #
// In the present example digital Pin 6 is used which corresponds to "PORTD",  #
// "DDRD" and "6", respectively.                                               #
//                                                                             #
//##############################################################################

#define USE_SERIAL_LIB     1
#define USE_TM1829         1

// Arduino Micro D5: PC6
#define DATA_PORT          PORTC
#define DATA_DDR           DDRC
#define DATA_PIN           6

#if USE_TM1829
// Renkforce TM1829 5m Digital LED Stripe
#define NUMBER_OF_PIXELS   50
#else
// WS2812 5m LED Stripe
#define NUMBER_OF_PIXELS   240
#endif

// Arduino Micro LED IO13: PC7
// indicates active output
#define LED_PORT            PORTC
#define LED_DDR             DDRC
#define LED_PIN             7              

//##############################################################################
//                                                                             #
// Variables                                                                   #
//                                                                             #
//##############################################################################

uint8_t display_buffer[NUMBER_OF_PIXELS * 3];
static uint8_t *ptr = display_buffer;
static uint16_t pos = 0;

volatile uint8_t go = 0;




//##############################################################################
//                                                                             #
// Setup                                                                       #
//                                                                             #
//##############################################################################

void setup()
{
  // Set data pin as output
  DATA_DDR |= (1 << DATA_PIN);

#if defined LED_PIN
  // Set LED pin as output
  LED_DDR |= (1 << LED_PIN);
#endif

  // clear LEDs
  cli();
#if USE_TM1829
  // Reset pulse => output high for typ. 500us
  DATA_PORT |= (1 << DATA_PIN);
  _delay_us(500);
  tm1829_sendarray(display_buffer, NUMBER_OF_PIXELS * 3);
#else
  ws2812_sendarray(display_buffer, NUMBER_OF_PIXELS * 3);
#endif
  sei();
  
#if USE_SERIAL_LIB
  // Initialize UART
  Serial.begin(115200);
  while (!Serial); // wait for serial attach
#else
  // Initialize UART
  UCSR0A |= (1<<U2X0);                                
  UCSR0B |= (1<<RXEN0)  | (1<<TXEN0) | (1<<RXCIE0);   
  UCSR0C |= (1<<UCSZ01) | (1<<UCSZ00)             ; 
  UBRR0H = 0;
  UBRR0L = 1; //Baud Rate 1 MBit (at F_CPU = 16MHz)
#endif
}


//##############################################################################
//                                                                             #
// Main loop                                                                   #
//                                                                             #
//##############################################################################

void loop()
{
#if USE_SERIAL_LIB
  if (Serial.available()) {
    uint8_t b = Serial.read();
    
    if (b == 1) {
	    // start of frame
      pos = 0;
      ptr = display_buffer;
    } else {
	    // RGB bytes, must not contain the value 1, for this is used as the start of frame signal!
      if (pos < (NUMBER_OF_PIXELS * 3)) {
        *ptr++ = b;
        pos++;
      }
      if (pos == (NUMBER_OF_PIXELS * 3)) {
        pos++;
        go = 1;
      }
    }
  }
#endif // USE_SERIAL_LIB

  if (go != 0) {
#if defined LED_PIN
    LED_PORT |= (1 << LED_PIN);
#endif

    cli();
#if USE_TM1829
    tm1829_sendarray(display_buffer, NUMBER_OF_PIXELS * 3);
#else
    ws2812_sendarray(display_buffer, NUMBER_OF_PIXELS * 3);
#endif
    sei();
    go = 0;
    
#if defined LED_PIN
    LED_PORT &= ~(1 << LED_PIN);
#endif
  }
}

#if !USE_SERIAL_LIB
//##############################################################################
//                                                                             #
// UART-Interrupt-Prozedur (called every time one byte is compeltely received) #
//                                                                             #
//##############################################################################

ISR(USART_RX_vect) 
{
  unsigned char b;
  b=UDR0;
  
  if (b == 1)  {pos=0; ptr=display_buffer; return;}    
  if (pos == (NUMBER_OF_PIXELS*3)) {} else {*ptr=b; ptr++; pos++;}  
  if (pos == ((NUMBER_OF_PIXELS*3)-1)) {go=1;}
}
#endif  // !USE_SERIAL_LIB

//##############################################################################
//                                                                             #
// WS2812 output routine                                                       #
// Extracted from a light weight WS2812 lib by Tim (cpldcpu@gmail.com)         #
// Found on wwww.microcontroller.net                                           #
// Requires F_CPU = 16MHz                                                      #
//                                                                             #
//##############################################################################

void ws2812_sendarray(uint8_t *data,uint16_t datlen)
{
  uint8_t curbyte,ctr,masklo;
  uint8_t maskhi = _BV(DATA_PIN);
  masklo =~ maskhi & DATA_PORT;
  maskhi |= DATA_PORT;

  while (datlen--) 
  {
    curbyte = *data++;

    // 1CK = 62.5ns
    // 0 = 375n (6CK)  H, 875n (14CK) L
    // 1 = 625n (10CK) H, 625n (10CK) L
    asm volatile
    (
      "		ldi %0,8	\n\t"		// 0
      "loop%=:out %2, %3	\n\t"		// 1
      "lsl	%1		\n\t"		// 2
      "dec	%0		\n\t"		// 3
      "		rjmp .+0	\n\t"		// 5
      "		brcs .+2	\n\t"		// 6l / 7h
      "		out %2,%4	\n\t"		// 7l / -
      "		rjmp .+0	\n\t"		// 9
      "		nop		\n\t"		// 10
      "		out %2,%4	\n\t"		// 11
      "		breq end%=	\n\t"		// 12      nt. 13 taken
      "		rjmp .+0	\n\t"		// 14
      "		rjmp .+0	\n\t"		// 16
      "		rjmp .+0	\n\t"		// 18
      "		rjmp loop%=	\n\t"		// 20
      "end%=:			\n\t" 
      :	"=&d" (ctr)
      :	"r" (curbyte), "I" (_SFR_IO_ADDR(DATA_PORT)), "r" (maskhi), "r" (masklo)
    );
  }

}


//##############################################################################
//                                                                             #
// RenkForce RGB output routine = WS2812 output routine modified by MBC        #
// source see above                                                            #
// Requires F_CPU = 16MHz                                                      #
//                                                                             #
//##############################################################################

void tm1829_sendarray(uint8_t *data, uint16_t datlen)
{
  uint8_t curbyte, ctr;
  uint8_t maskhi, masklo;
  
  masklo = ~_BV(DATA_PIN) & DATA_PORT;
  maskhi =  _BV(DATA_PIN) | DATA_PORT;

  // output pin is idle high
  
  while (datlen--)      // ?CK
  {
    curbyte = *data++;  // ?CK
    
    // 1CK = 62.5ns
    // 0 = 1875n (30CK) H, 250n (4CK)  L
    // 1 = 1625n (26CK) H, 750n (12CK) L
    asm volatile
    (
      "        ldi  %0,8        \n"   // ldi ctr,8 (1CK)
      "loop%=: out  %2, %3      \n"   // out DATA_PORT, maskhi (1CK)
      "        lsl  %1          \n"   // lsl curbyte (shifts Bit7 into C) (1CK)
      "        dec  %0          \n"   // dec ctr (1CK)
      "        rjmp .+0         \n"   // relative jump to next instruction (2CK)
      "        rjmp .+0         \n"   // (2CK)
      "        rjmp .+0         \n"   // (2CK)
      "        rjmp .+0         \n"   // (2CK)
      "        rjmp .+0         \n"   // (2CK)
      "        rjmp .+0         \n"   // (2CK) 14CK
//      "        rjmp .+0         \n"   // (2CK) 16CK
//      "        rjmp .+0         \n"   // (2CK) 18CK
//      "        rjmp .+0         \n"   // (2CK) 20CK
//      "        rjmp .+0         \n"   // (2CK) 22CK
//      "        rjmp .+0         \n"   // (2CK) 24CK
      "        brcc msb0%=      \n"   // branch to msb0 if carry is cleared (1/2CK)
      "msb1%=: out  %2,%4       \n"   // out DATA_PORT, masklo (1CK) 26CK if C == 1
      "        rjmp .+0         \n"   // (2CK)
      "        rjmp .+0         \n"   // (2CK)
      "        nop              \n"   // (1CK)
      "msb0%=: rjmp .+0         \n"   // (2CK)
      "        nop              \n"   // (1CK)
      "        out  %2,%4       \n"   // out DATA_PORT, masklo (1CK) 30CK if C == 0
      "        nop              \n"   // (1CK)
      "        rjmp .+0         \n"   // (2CK)
      "        out  %2,%3       \n"   // out DATA_PORT, maskhi (1CK) 4CK if C == 0, 12CK if CK == 1
      "        breq end%=       \n"   // branch to end if equal (z == 1 -> ctr == 0) (1/2CK)
//      "        rjmp .+0         \n"   // (2CK)
//      "        rjmp .+0         \n"   // (2CK)
//      "        nop              \n"   // (1CK)
      "        rjmp .+0         \n"   // (2CK)
      "        rjmp .+0         \n"   // (2CK)
      "        rjmp .+0         \n"   // (2CK)
      "        rjmp loop%=      \n"   // back to loop (2CK)
      "end%=:                   \n" 
      : "=&d" (ctr)
      : "r" (curbyte), "I" (_SFR_IO_ADDR(DATA_PORT)), "r" (maskhi), "r" (masklo)
    );
  }
}


//##############################################################################
//                                                                             #
// End of program                                                              #
//                                                                             #
//##############################################################################


