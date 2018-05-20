#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define F_CPU         8000000UL
#define USB_BAUD_RATE 9600UL
#define USB_UBRR_VAL  (F_CPU/(16*USB_BAUD_RATE) - 1)

#define BLUETOOTH_FREQ      1000000UL
#define BLUETOOTH_BAUD_RATE 9600UL
#define BLUETOOTH_UBRR_VAL   (BLUETOOTH_FREQ/(2*BLUETOOTH_BAUD_RATE))

#define BLUETOOTH_DDR     DDRD
#define BLUETOOTH_PORT    PORTD
#define BLUETOOTH_PIN     PIND
#define BLUETOOTH_RX_PIN  (1 << PIND2)
#define BLUETOOTH_TX_PORT (1 << PORTD3)

#define DDR_BTN    DDRD
#define PIN_BTN    PIND
#define PORT_BTN   PORTD
#define PIN_BTNIN  (1 << PIND5)
#define PORT_BTNIN (1 << PORTD5)

#define DDR_LED     DDRB
#define LED_PORT    PORTB
#define LED_GREEN   (1 << PORTB0)
#define LED_RED     (1 << PORTB1)

#define led_green_on()  (LED_PORT |= LED_GREEN)
#define led_green_off() (LED_PORT &= ~LED_GREEN)
#define led_red_on()    (LED_PORT |= LED_RED)
#define led_red_off()   (LED_PORT &= ~LED_RED)

#define USB_PORT  PORTD
#define USB_PIN   PIND
#define USB_TX    (1 << PORTD4)
#define USB_RX    (1 << PIND2) //int0

#define enableUsbRxInterrupt()  (UCSRB |=  (1 << RXCIE))
#define disableUsbRxInterrupt() (UCSRB &= ~(1 << RXCIE))

#define enableUsbTxInterrupt()  (UCSRB |=  (1 << TXCIE))
#define disableUsbTxInterrupt() (UCSRB &= ~(1 << TXCIE))

#define enableUsbUDRIsEmptyInterrupt()  (UCSRB |=  (1 << UDRIE))
#define disableUsbUDRIsEmptyInterrupt() (UCSRB &= ~(1 << UDRIE))

#define enableUsbReceiver()   (UCSRB |=  (1 << RXEN))
#define disableUsbReceiver()  (UCSRB &= ~(1 << RXEN))

#define enableUsbTransmitter()  (UCSRB |=  (1 << TXEN))
#define disableUsbTransmitter() (UCSRB &= ~(1 << TXEN))

#define enableTimer0OvfInterrupt()  (TIMSK |=  (1 << TOIE0))
#define disableTimer0OvfInterrupt() (TIMSK &= ~(1 << TOIE0))

#define enableInt0Interrupt()  (GIMSK |=  (1 << INT0))
#define disableInt0Interrupt() (GIMSK &= ~(1 << INT0))

#define enableTimer1CompAInterrupt()  (TIMSK |=  (1 << OCIE1A))
#define disableTimer1CompAInterrupt() (TIMSK &= ~(1 << OCIE1A))

#define enableTimer1CompBInterrupt()  (TIMSK |=  (1 << OCIE1B))
#define disableTimer1CompBInterrupt() (TIMSK &= ~(1 << OCIE1B))

#define STATE_REQ 0xa6 //10100110 seems ok
static volatile uint8_t btnState = 0;
//////////////////////////////////////////////////////////////////////////

static void initUsbTtl();
static void initTimer0();
static void initTimer1();
static void initInt0();

//////////////////////////////////////////////////////////////////////////

ISR(USART_RX_vect) {  
  disableUsbRxInterrupt();
  uint8_t rx = UDR;
  if (rx == STATE_REQ) {
    enableUsbUDRIsEmptyInterrupt();
  } else {
    enableUsbRxInterrupt();
  }
}
//////////////////////////////////////////////////////////////////////////

ISR(USART_TX_vect) {  
  enableUsbRxInterrupt();
}
//////////////////////////////////////////////////////////////////////////

ISR(USART_UDRE_vect) {  
  UDR = btnState;
  disableUsbUDRIsEmptyInterrupt();
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

//start of transmission
ISR(INT0_vect) {
  disableInt0Interrupt();
  OCR1A = TCNT1+BLUETOOTH_UBRR_VAL;
  enableTimer1CompAInterrupt();
}
//////////////////////////////////////////////////////////////////////////

static volatile uint8_t btrx_buff = 0;
static volatile uint8_t btrx_count = 0;
static volatile uint8_t bt_received = 0;
//rx timer
ISR(TIMER1_COMPA_vect) {
  register uint8_t rx_state = BLUETOOTH_RX_PIN & BLUETOOTH_PIN;
  OCR1A += BLUETOOTH_UBRR_VAL;
  if (++btrx_count == 1) {
    btrx_buff = 0;
    if (rx_state) { //high level is not valid start bit
      btrx_count = 0;
      disableTimer1CompAInterrupt();
      enableInt0Interrupt();
    }
    return;
  }

  if (btrx_count == 19) {
    disableTimer1CompAInterrupt();
    bt_received = 1;
    btrx_count = 0;
    return;
  }

  if (btrx_count & 0x01) { //odd count
    btrx_buff >>= 1; //MSB!!!!!!
    if (rx_state)
      btrx_buff |= 0x80;
  }
}
//////////////////////////////////////////////////////////////////////////

static volatile uint8_t bttx_buff;
static volatile uint8_t bttx_count;

//tx timer
ISR(TIMER1_COMPB_vect) {
  OCR1B += BLUETOOTH_UBRR_VAL;
  if (++bttx_count == 18) {
    disableTimer1CompBInterrupt();
    bttx_count = 0;
    BLUETOOTH_PORT |= BLUETOOTH_TX_PORT;
    return;
  }

  if (bttx_count & 0x01)
    return;

  if (bttx_buff & 0x01) //LSB
    BLUETOOTH_PORT |= BLUETOOTH_TX_PORT;
  else
    BLUETOOTH_PORT &= ~BLUETOOTH_TX_PORT;
  bttx_buff >>= 1;
}
//////////////////////////////////////////////////////////////////////////

int main(void) {
  DDR_BTN &= ~PIN_BTNIN;
  PORT_BTN |= PORT_BTNIN; //pull up
  DDR_LED = LED_GREEN | LED_RED;

  BLUETOOTH_DDR  |= BLUETOOTH_TX_PORT;
  BLUETOOTH_DDR &= ~(BLUETOOTH_RX_PIN);
  BLUETOOTH_PORT |= BLUETOOTH_TX_PORT;

  initUsbTtl();
  initTimer0();
  initTimer1();
  initInt0();

  sei();

  while (1) {
    if (bt_received) {
      if (btrx_buff == STATE_REQ && !bttx_count) { //state request && transmission isn't in progress
        bttx_count = 0;
        bttx_buff = btnState;
        OCR1B = TCNT1 + BLUETOOTH_UBRR_VAL;
        BLUETOOTH_PORT &= ~BLUETOOTH_TX_PORT; //startbit
        enableTimer1CompBInterrupt();
      }
      bt_received = 0;
      enableInt0Interrupt();
    }
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////

void initUsbTtl() {
  UBRRH = (uint8_t)(USB_UBRR_VAL >> 8);
  UBRRL = (uint8_t) USB_UBRR_VAL;

  enableUsbReceiver();
  enableUsbTransmitter();

  enableUsbRxInterrupt();
  enableUsbTxInterrupt();

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
//////////////////////////////////////////////////////////////////////////

void initTimer1() {
  TCCR1B = 1 << CS11; //8 prescale
}
//////////////////////////////////////////////////////////////////////////

void initInt0() {
  MCUCR = 1 << ISC11; // The falling edge of INT0 generates an interrupt request.
  enableInt0Interrupt();
}
//////////////////////////////////////////////////////////////////////////
