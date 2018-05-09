#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define F_CPU 8000000UL
#define USB_BAUD_RATE 9600UL
#define USB_UBRR_VAL (F_CPU/(16*USB_BAUD_RATE) - 1)

#define DDR_BTN DDRD
#define PIN_BTN PIND
#define PORT_BTN PORTD
#define PIN_BTNIN (1 << PIND5)
#define PORT_BTNIN (1 << PORTD5)

#define DDR_LED DDRB
#define POUT_LED PORTB
#define LED_GREEN  (1 << PORTB0)
#define LED_RED    (1 << PORTB1)

#define led_green_on()  (POUT_LED |= LED_GREEN)
#define led_green_off() (POUT_LED &= ~LED_GREEN)
#define led_red_on()    (POUT_LED |= LED_RED)
#define led_red_off()   (POUT_LED &= ~LED_RED)

#define USB_PORT  PORTD
#define USB_PIN   PIND
#define USB_TX (1 << PORTD4)
#define USB_RX (1 << PIND2) //int0

#define enableUsbRxInterrupt() (UCSRB |= (1 << RXCIE))
#define disableUsbRxInterrupt() (UCSRB &= ~(1 << RXCIE))

#define enableUsbTxInterrupt() (UCSRB |= (1 << TXCIE))
#define disableUsbTxInterrupt() (UCSRB &= ~(1 << TXCIE))

#define enableUsbUDRIsEmptyInterrupt() (UCSRB |= (1 << UDRIE))
#define disableUsbUDRIsEmptyInterrupt() (UCSRB &= ~(1 << UDRIE))

#define enableUsbReceiver() (UCSRB |= (1 << RXEN))
#define disableUsbReceiver() (UCSRB &= ~(1 << RXEN))

#define enableUsbTransmitter() (UCSRB |= (1 << TXEN))
#define disableUsbTransmitter() (UCSRB &= ~(1 << TXEN))

#define enableTimer0OvfInterrupt() (TIMSK |= (1 << TOIE0))
#define disableTimer0OvfInterrupt() (TIMSK &= ~(1 << TOIE0))

#define USB_RXTX_BUFF_SIZE 4
//static uint8_t usbBuff[USB_RXTX_BUFF_SIZE];
static volatile uint8_t btnState = 0;
//////////////////////////////////////////////////////////////////////////

static void initUsbTtl();
static void initTimer0();
//////////////////////////////////////////////////////////////////////////

ISR(USART_RX_vect) {
}
//////////////////////////////////////////////////////////////////////////

ISR(USART_TX_vect) {
}
//////////////////////////////////////////////////////////////////////////

ISR(USART_UDRE_vect) {  
}
//////////////////////////////////////////////////////////////////////////

ISR(TIMER0_OVF_vect) {
  disableTimer0OvfInterrupt();
  btnState = (PIN_BTN & PIN_BTNIN);
  if (!btnState) {
    led_green_on();
    led_red_off();
  } else {
    led_green_off();
    led_red_on();
  }
  enableTimer0OvfInterrupt();
}
//////////////////////////////////////////////////////////////////////////

void main(void) {
  DDR_BTN &= ~PIN_BTNIN;
  PORT_BTN |= PORT_BTNIN;

  DDR_LED = LED_GREEN | LED_RED;
  initUsbTtl();
  initTimer0();
  sei();
  while (1) {
  }
}

//////////////////////////////////////////////////////////////////////////

void initUsbTtl() {
  UBRRH = (uint8_t)(USB_UBRR_VAL >> 8);
  UBRRL = (uint8_t)USB_UBRR_VAL;

  enableUsbReceiver();
  enableUsbTransmitter();

  enableUsbRxInterrupt();
  enableUsbTxInterrupt();
  enableUsbUDRIsEmptyInterrupt();

  UCSRC = 0x00         |                // UMSEL0 = UMSEL1 = 0 - async
          (0 << UPM0)  | (0 << UPM1)  | // parity none
          (0 << USBS)  |                // 1 stop bit
          (1 << UCSZ0) | (1 << UCSZ1) | // 8-bit
          (0 << UCPOL);                 // TX - RISING, RX - FALLING
}
//////////////////////////////////////////////////////////////////////////

void initTimer0() {
  TCCR0B = (1 << CS02) | (1 << CS00); //1024 prescale
  enableTimer0OvfInterrupt();
}
