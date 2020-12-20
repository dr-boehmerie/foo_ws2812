
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
//                                                                             #
// Modified by M. Böhme 2017-2020                                              #
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

// 600 LEDs split into 4 groups of 150 each
#define LED_NO            600
#define LED_STRIPE_NO     4
#define LED_RGBW          1

// Arduino Leonardo D5: PC6
#define DATA_PORT         PORTC
#define DATA_DDR          DDRC
#define DATA_PIN          6
// start of frame for this stripe
#define DATA_SOF          254

#if LED_STRIPE_NO > 1
// Arduino Leonardo D6: PD7
#define DATA2_PORT        PORTD
#define DATA2_DDR         DDRD
#define DATA2_PIN         7
// start of frame for this stripe
#define DATA2_SOF         253
#endif

#if LED_STRIPE_NO > 2
// Arduino Leonardo D7: PE6
#define DATA3_PORT        PORTE
#define DATA3_DDR         DDRE
#define DATA3_PIN         6
// start of frame for this stripe
#define DATA3_SOF         252
#endif

#if LED_STRIPE_NO > 3
// Arduino Leonardo IO8: PB4
#define DATA4_PORT        PORTB
#define DATA4_DDR         DDRB
#define DATA4_PIN         4
// start of frame for this stripe
#define DATA4_SOF         251
#endif

#if LED_STRIPE_NO > 4
// Arduino Leonardo IO9: PB5
#define DATA5_PORT        PORTB
#define DATA5_DDR         DDRB
#define DATA5_PIN         5
// start of frame for this stripe
#define DATA5_SOF         250
#endif

#if LED_STRIPE_NO > 5
// Arduino Leonardo IO10: PB6
#define DATA6_PORT        PORTB
#define DATA6_DDR         DDRB
#define DATA6_PIN         6
// start of frame for this stripe
#define DATA6_SOF         249
#endif

#if 1
// Arduino Leonardo IO12: PD6
// Enable for DC/DC converters for the LEDs
#define LED_SUPPLY_PORT   PORTD
#define LED_SUPPLY_DDR    DDRD
#define LED_SUPPLY_PIN    6
#endif

// Arduino Leonardo LED IO13: PC7
// indicates active output
#define LED_PORT            PORTC
#define LED_DDR             DDRC
#define LED_PIN             7

//##############################################################################
//                                                                             #
// Variables                                                                   #
//                                                                             #
//##############################################################################

#define NUMBER_OF_PIXELS   (LED_NO / LED_STRIPE_NO)

// number of bytes per pixel (3 for BRG, 4 for RGBW)
#if LED_RGBW
#define BYTE_PER_PIXEL     4
#else
#define BYTE_PER_PIXEL     3
#endif

#define BUFFER_SIZE       (NUMBER_OF_PIXELS * BYTE_PER_PIXEL)

#if BUFFER_SIZE < 255
#define CNT_TYPE  uint8_t
#else
#define CNT_TYPE  uint16_t
#endif

uint8_t  pixel_buffer[BUFFER_SIZE];
uint8_t  *ptr = pixel_buffer;
CNT_TYPE cnt = 0;
uint8_t  sof = 0;
uint8_t  active = 0;

uint8_t  go = 0;

const uint32_t timeout = 1000;
uint32_t last_ticks = 0;

//##############################################################################
//                                                                             #
// Inline functions                                                            #
//                                                                             #
//##############################################################################

// the DATA_PORTs are #defines, so this function can be inlined and reduced to the call of the appropriate port function
static inline void ws2812_sendarray(const uint8_t *data, const CNT_TYPE dataLen, const uint8_t dataPinMask, const void *dataPort)
{
#if defined PORTA
  if (dataPort == _SFR_IO_ADDR(PORTA))
  {
     ws2812_sendarray_port_a(data, dataLen, dataPinMask);
     return;
  }
#endif
#if defined PORTB
  if (dataPort == _SFR_IO_ADDR(PORTB))
  {
     ws2812_sendarray_port_b(data, dataLen, dataPinMask);
     return;
  }
#endif
#if defined PORTC
  if (dataPort == _SFR_IO_ADDR(PORTC))
  {
     ws2812_sendarray_port_c(data, dataLen, dataPinMask);
     return;
  }
#endif
#if defined PORTD
  if (dataPort == _SFR_IO_ADDR(PORTD))
  {
     ws2812_sendarray_port_d(data, dataLen, dataPinMask);
     return;
  }
#endif
#if defined PORTE
  if (dataPort == _SFR_IO_ADDR(PORTE))
  {
     ws2812_sendarray_port_e(data, dataLen, dataPinMask);
     return;
  }
#endif
}

//##############################################################################
//                                                                             #
// Setup                                                                       #
//                                                                             #
//##############################################################################

void setup()
{
  // Set data pin(s) as output
  DATA_DDR |= _BV(DATA_PIN);

#if defined DATA2_PIN
  DATA2_DDR |= _BV(DATA2_PIN);
#endif
#if defined DATA3_PIN
  DATA3_DDR |= _BV(DATA3_PIN);
#endif
#if defined DATA4_PIN
  DATA4_DDR |= _BV(DATA4_PIN);
#endif
#if defined DATA5_PIN
  DATA5_DDR |= _BV(DATA5_PIN);
#endif
#if defined DATA6_PIN
  DATA6_DDR |= _BV(DATA6_PIN);
#endif

#if defined LED_SUPPLY_PIN
  // Set LED supply enable pin as output
  LED_SUPPLY_DDR |= _BV(LED_SUPPLY_PIN);
#endif

#if defined LED_PIN
  // Set LED pin as output
  LED_DDR |= _BV(LED_PIN);
#endif

#if !defined LED_SUPPLY_PIN
  // clear LEDs
  clear_leds();
#endif

  // Initialize UART (for USB UART the baudrate is irrelevant)
  Serial.begin(115200);
  while (!Serial); // wait for USB attach

  active = 1;

#if defined LED_SUPPLY_PIN
  // Enable LED suppy
  LED_SUPPLY_PORT |= _BV(LED_SUPPLY_PIN);
  // wait for stable power supply
  delay(500);

  // clear LEDs
  clear_leds();
#endif // defined LED_SUPPLY_PIN

  last_ticks = millis();
}


//##############################################################################
//                                                                             #
// Main loop                                                                   #
//                                                                             #
//##############################################################################

void loop()
{
  // read data
  while (Serial.available())
  {
    uint8_t b = Serial.read();

    if (b == DATA_SOF
#if defined DATA2_SOF
        || b == DATA2_SOF
#endif
#if defined DATA3_SOF
        || b == DATA3_SOF
#endif
#if defined DATA4_SOF
        || b == DATA4_SOF
#endif
#if defined DATA5_SOF
        || b == DATA5_SOF
#endif
#if defined DATA6_SOF
        || b == DATA6_SOF
#endif
       )
    {
	    // start of frame
      sof = b;
      cnt = 0;
      ptr = pixel_buffer;
    }
    else
    {
	    // RGB(W) bytes, must not contain start of frame value!
      if (cnt < BUFFER_SIZE)
      {
        *ptr++ = b;
        cnt++;
      }
      if (cnt == BUFFER_SIZE)
      {
        // stop reception
        go = 1;
        break;
      }
    }
  }

  // send data to stripes
  if (go != 0)
  {
#if defined LED_PIN
    LED_PORT |= _BV(LED_PIN);
#endif

    // SOF value selects the stripe
    switch (sof)
    {
    case DATA_SOF:
      ws2812_sendarray(pixel_buffer, BUFFER_SIZE, _BV(DATA_PIN), _SFR_IO_ADDR(DATA_PORT));
      break;

#if defined DATA2_SOF
    case DATA2_SOF:
      ws2812_sendarray(pixel_buffer, BUFFER_SIZE, _BV(DATA2_PIN), _SFR_IO_ADDR(DATA2_PORT));
      break;
#endif
#if defined DATA3_SOF
    case DATA3_SOF:
      ws2812_sendarray(pixel_buffer, BUFFER_SIZE, _BV(DATA3_PIN), _SFR_IO_ADDR(DATA3_PORT));
      break;
#endif
#if defined DATA4_SOF
    case DATA4_SOF:
      ws2812_sendarray(pixel_buffer, BUFFER_SIZE, _BV(DATA4_PIN), _SFR_IO_ADDR(DATA4_PORT));
      break;
#endif
#if defined DATA5_SOF
    case DATA5_SOF:
      ws2812_sendarray(pixel_buffer, BUFFER_SIZE, _BV(DATA5_PIN), _SFR_IO_ADDR(DATA5_PORT));
      break;
#endif
#if defined DATA6_SOF
    case DATA6_SOF:
      ws2812_sendarray(pixel_buffer, BUFFER_SIZE, _BV(DATA6_PIN), _SFR_IO_ADDR(DATA6_PORT));
      break;
#endif

    default:
      break;
    }
    go = 0;

    last_ticks = millis();
    active = 1;

#if defined LED_PIN
    LED_PORT &= ~_BV(LED_PIN);
#endif
  }


  // disable LEDs if no data has been received for a certain amount of time
  if (active != 0)
  {
    uint32_t ticks = millis();

    if (ticks < last_ticks)
    {
      // overflow
      last_ticks = ticks;
    }
    else
    {
      uint32_t tmp = ticks - last_ticks;

      if (tmp > timeout)
      {
        // clear LEDs
        clear_leds();
        active = 0;
      }
    }
  }
}

void clear_leds(void)
{
  memset(pixel_buffer, 0, BUFFER_SIZE);

  ws2812_sendarray(pixel_buffer, BUFFER_SIZE, _BV(DATA_PIN), _SFR_IO_ADDR(DATA_PORT));

#if defined DATA2_PIN
  ws2812_sendarray(pixel_buffer, BUFFER_SIZE, _BV(DATA2_PIN), _SFR_IO_ADDR(DATA2_PORT));
#endif
#if defined DATA3_PIN
  ws2812_sendarray(pixel_buffer, BUFFER_SIZE, _BV(DATA3_PIN), _SFR_IO_ADDR(DATA3_PORT));
#endif
#if defined DATA4_PIN
  ws2812_sendarray(pixel_buffer, BUFFER_SIZE, _BV(DATA4_PIN), _SFR_IO_ADDR(DATA4_PORT));
#endif
#if defined DATA5_PIN
  ws2812_sendarray(pixel_buffer, BUFFER_SIZE, _BV(DATA5_PIN), _SFR_IO_ADDR(DATA5_PORT));
#endif
#if defined DATA6_PIN
  ws2812_sendarray(pixel_buffer, BUFFER_SIZE, _BV(DATA6_PIN), _SFR_IO_ADDR(DATA6_PORT));
#endif
}

//##############################################################################
//                                                                             #
// WS2812 output routine                                                       #
// Extracted from a light weight WS2812 lib by Tim (cpldcpu@gmail.com)         #
// Found on wwww.microcontroller.net                                           #
// Requires F_CPU = 16MHz                                                      #
//                                                                             #
//##############################################################################
//                                                                             #
// M. Böhme: the address of the DATA_PORT is used as immediate,                #
// so each port needs its own function                                         #
// not all controllers have every PORT, so check the #defines                  #
//                                                                             #
//##############################################################################

#if defined PORTA
void ws2812_sendarray_port_a(const uint8_t *data, CNT_TYPE dataLen, const uint8_t dataPinMask)
{
  const uint8_t maskhi = dataPinMask | PORTA;
  const uint8_t masklo = (~data_pin_mask) & PORTA;
  uint8_t curbyte, ctr;

  // interrupts would mess up the timing
  cli();

  while (dataLen--)
  {
    curbyte = *data++;

    // 1CK = 62.5ns
    // 0 = 375n (6CK)  H, 875n (14CK) L
    // 1 = 625n (10CK) H, 625n (10CK) L
    asm volatile
    (
      "		ldi %0,8	\n\t"		// 0 : ctr = 8 (8 bits per byte)
      "loop%=:out %2, %3	\n\t"		// 1 : DATA_PORT = maskhi
      "lsl	%1		\n\t"		// 2 : curbyte <<= 1
      "dec	%0		\n\t"		// 3 : ctr--
      "		rjmp .+0	\n\t"		// 5
      "		brcs .+2	\n\t"		// 6l / 7h : test MSB of curbyte
      "		out %2,%4	\n\t"		// 7l / -  : DATA_PORT = masklo
      "		rjmp .+0	\n\t"		// 9
      "		nop		\n\t"		// 10
      "		out %2,%4	\n\t"		// 11 : DATA_PORT = masklo
      "		breq end%=	\n\t"		// 12      nt. 13 taken
      "		rjmp .+0	\n\t"		// 14
      "		rjmp .+0	\n\t"		// 16
      "		rjmp .+0	\n\t"		// 18
      "		rjmp loop%=	\n\t"		// 20
      "end%=:			\n\t"
      :	"=&d" (ctr)
      :	"r" (curbyte), "I" (_SFR_IO_ADDR(PORTA)), "r" (maskhi), "r" (masklo)
    );
  }

  sei();
}
#endif // defined PORTA

#if defined PORTB
void ws2812_sendarray_port_b(const uint8_t *data, CNT_TYPE dataLen, const uint8_t data_pin_mask)
{
  const uint8_t maskhi = data_pin_mask | PORTB;
  const uint8_t masklo = (~data_pin_mask) & PORTB;
  uint8_t curbyte, ctr;

  // interrupts would mess up the timing
  cli();

  while (dataLen--)
  {
    curbyte = *data++;

    // 1CK = 62.5ns
    // 0 = 375n (6CK)  H, 875n (14CK) L
    // 1 = 625n (10CK) H, 625n (10CK) L
    asm volatile
    (
      "    ldi %0,8  \n\t"   // 0 : ctr = 8 (8 bits per byte)
      "loop%=:out %2, %3  \n\t"   // 1 : DATA_PORT = maskhi
      "lsl  %1    \n\t"   // 2 : curbyte <<= 1
      "dec  %0    \n\t"   // 3 : ctr--
      "   rjmp .+0  \n\t"   // 5
      "   brcs .+2  \n\t"   // 6l / 7h : test MSB of curbyte
      "   out %2,%4 \n\t"   // 7l / -  : DATA_PORT = masklo
      "   rjmp .+0  \n\t"   // 9
      "   nop   \n\t"   // 10
      "   out %2,%4 \n\t"   // 11 : DATA_PORT = masklo
      "   breq end%=  \n\t"   // 12      nt. 13 taken
      "   rjmp .+0  \n\t"   // 14
      "   rjmp .+0  \n\t"   // 16
      "   rjmp .+0  \n\t"   // 18
      "   rjmp loop%= \n\t"   // 20
      "end%=:     \n\t"
      : "=&d" (ctr)
      : "r" (curbyte), "I" (_SFR_IO_ADDR(PORTB)), "r" (maskhi), "r" (masklo)
    );
  }

  sei();
}
#endif // defined PORTB

#if defined PORTC
void ws2812_sendarray_port_c(const uint8_t *data, CNT_TYPE dataLen, const uint8_t data_pin_mask)
{
  const uint8_t maskhi = data_pin_mask | PORTC;
  const uint8_t masklo = (~data_pin_mask) & PORTC;
  uint8_t curbyte, ctr;

  // interrupts would mess up the timing
  cli();

  while (dataLen--)
  {
    curbyte = *data++;

    // 1CK = 62.5ns
    // 0 = 375n (6CK)  H, 875n (14CK) L
    // 1 = 625n (10CK) H, 625n (10CK) L
    asm volatile
    (
      "    ldi %0,8  \n\t"   // 0 : ctr = 8 (8 bits per byte)
      "loop%=:out %2, %3  \n\t"   // 1 : DATA_PORT = maskhi
      "lsl  %1    \n\t"   // 2 : curbyte <<= 1
      "dec  %0    \n\t"   // 3 : ctr--
      "   rjmp .+0  \n\t"   // 5
      "   brcs .+2  \n\t"   // 6l / 7h : test MSB of curbyte
      "   out %2,%4 \n\t"   // 7l / -  : DATA_PORT = masklo
      "   rjmp .+0  \n\t"   // 9
      "   nop   \n\t"   // 10
      "   out %2,%4 \n\t"   // 11 : DATA_PORT = masklo
      "   breq end%=  \n\t"   // 12      nt. 13 taken
      "   rjmp .+0  \n\t"   // 14
      "   rjmp .+0  \n\t"   // 16
      "   rjmp .+0  \n\t"   // 18
      "   rjmp loop%= \n\t"   // 20
      "end%=:     \n\t"
      : "=&d" (ctr)
      : "r" (curbyte), "I" (_SFR_IO_ADDR(PORTC)), "r" (maskhi), "r" (masklo)
    );
  }

  sei();
}
#endif // defined PORTC

#if defined PORTD
void ws2812_sendarray_port_d(const uint8_t *data, CNT_TYPE dataLen, const uint8_t data_pin_mask)
{
  const uint8_t maskhi = data_pin_mask | PORTD;
  const uint8_t masklo = (~data_pin_mask) & PORTD;
  uint8_t curbyte, ctr;

  // interrupts would mess up the timing
  cli();

  while (dataLen--)
  {
    curbyte = *data++;

    // 1CK = 62.5ns
    // 0 = 375n (6CK)  H, 875n (14CK) L
    // 1 = 625n (10CK) H, 625n (10CK) L
    asm volatile
    (
      "    ldi %0,8  \n\t"   // 0 : ctr = 8 (8 bits per byte)
      "loop%=:out %2, %3  \n\t"   // 1 : DATA_PORT = maskhi
      "lsl  %1    \n\t"   // 2 : curbyte <<= 1
      "dec  %0    \n\t"   // 3 : ctr--
      "   rjmp .+0  \n\t"   // 5
      "   brcs .+2  \n\t"   // 6l / 7h : test MSB of curbyte
      "   out %2,%4 \n\t"   // 7l / -  : DATA_PORT = masklo
      "   rjmp .+0  \n\t"   // 9
      "   nop   \n\t"   // 10
      "   out %2,%4 \n\t"   // 11 : DATA_PORT = masklo
      "   breq end%=  \n\t"   // 12      nt. 13 taken
      "   rjmp .+0  \n\t"   // 14
      "   rjmp .+0  \n\t"   // 16
      "   rjmp .+0  \n\t"   // 18
      "   rjmp loop%= \n\t"   // 20
      "end%=:     \n\t"
      : "=&d" (ctr)
      : "r" (curbyte), "I" (_SFR_IO_ADDR(PORTD)), "r" (maskhi), "r" (masklo)
    );
  }

  sei();
}
#endif // defined PORTD

#if defined PORTE
void ws2812_sendarray_port_e(const uint8_t *data, CNT_TYPE dataLen, const uint8_t data_pin_mask)
{
  const uint8_t maskhi = data_pin_mask | PORTE;
  const uint8_t masklo = (~data_pin_mask) & PORTE;
  uint8_t curbyte, ctr;

  // interrupts would mess up the timing
  cli();

  while (dataLen--)
  {
    curbyte = *data++;

    // 1CK = 62.5ns
    // 0 = 375n (6CK)  H, 875n (14CK) L
    // 1 = 625n (10CK) H, 625n (10CK) L
    asm volatile
    (
      "    ldi %0,8  \n\t"   // 0 : ctr = 8 (8 bits per byte)
      "loop%=:out %2, %3  \n\t"   // 1 : DATA_PORT = maskhi
      "lsl  %1    \n\t"   // 2 : curbyte <<= 1
      "dec  %0    \n\t"   // 3 : ctr--
      "   rjmp .+0  \n\t"   // 5
      "   brcs .+2  \n\t"   // 6l / 7h : test MSB of curbyte
      "   out %2,%4 \n\t"   // 7l / -  : DATA_PORT = masklo
      "   rjmp .+0  \n\t"   // 9
      "   nop   \n\t"   // 10
      "   out %2,%4 \n\t"   // 11 : DATA_PORT = masklo
      "   breq end%=  \n\t"   // 12      nt. 13 taken
      "   rjmp .+0  \n\t"   // 14
      "   rjmp .+0  \n\t"   // 16
      "   rjmp .+0  \n\t"   // 18
      "   rjmp loop%= \n\t"   // 20
      "end%=:     \n\t"
      : "=&d" (ctr)
      : "r" (curbyte), "I" (_SFR_IO_ADDR(PORTE)), "r" (maskhi), "r" (masklo)
    );
  }

  sei();
}
#endif // defined PORTE

//##############################################################################
//                                                                             #
// End of program                                                              #
//                                                                             #
//##############################################################################
