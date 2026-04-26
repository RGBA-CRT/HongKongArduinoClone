#pragma GCC push_options
#pragma GCC optimize("Ofast")

//-----------------
// low level serial comm
//-----------------

// Arduino UNO 16MHz: ポーリング方式
// LGT8F328 32MHz: IRQ方式
// Arduino UnoでIRQ方式使うと速度が40KB/s程度に半減する
#if (F_CPU < 32000000ULL)
#define SEND_METHOD_POLL
#endif

#define TX_BUF_SIZE 128
#define TX_BUF_MASK (TX_BUF_SIZE - 1)

#define RX_BUF_SIZE 32
#define RX_BUF_MASK (RX_BUF_SIZE - 1)

#ifndef SEND_METHOD_POLL
// #error "32MHz"
static volatile uint8_t tx_buf[TX_BUF_SIZE];
static volatile uint8_t tx_head = 0;  // to be sent
static volatile uint8_t tx_tail = 0;  // sent
#else
// #error "16MHz"
#endif

static volatile uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint8_t rx_head = 0;  // received
static volatile uint8_t rx_tail = 0;  // consumed

uint8_t serial_available(void) {
  return (rx_head - rx_tail) & RX_BUF_MASK;
}

uint8_t serial_read(void) {
  while (rx_head == rx_tail)
    ;
  uint8_t data = rx_buf[rx_tail];
  rx_tail = (rx_tail + 1) & RX_BUF_MASK;
  return data;
}

uint16_t serial_read_word(void) {
  return (uint16_t)serial_read() | ((word)serial_read() << 8);
}

void serial_begin(uint32_t baud) {
  uint16_t brr_val = ((F_CPU / 4 / baud) - 1) / 2;
  uint8_t u2x_en = 1;
  if ((brr_val > 0xFFF) /*|| (baud==2000000UL)*/) {
    brr_val = ((F_CPU / 8 / baud) - 1) / 2;
    u2x_en = 0;
  }
  UBRR0H = brr_val >> 8;  // UBRR=16
  UBRR0L = brr_val;

  UCSR0A = (u2x_en << U2X0);
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // 8bit
}

long tx_spin_count = 0;
inline void serial_send(uint8_t data) {
#ifdef SEND_METHOD_POLL
  //UDRが空になるのを待つ
  while (!(UCSR0A & _BV(UDRE0))) {
    tx_spin_count++;
  }
  UDR0 = data;
#else
  uint8_t next = (tx_head + 1) & TX_BUF_MASK;

  // バッファフル待ち
  // 例えばフルの時、head=5 tail=6としてnextは6, tailが進んで7になるまで待つ。
  while (next == tx_tail) {
    tx_spin_count++;
  }

  // noInterrupts();
  tx_buf[tx_head] = data;
  tx_head = next;
  // interrupts();

  // UDRE割り込み有効化（送信開始トリガ）
  UCSR0B |= (1 << UDRIE0);
#endif
}

void serial_send_text(const char* text) {
  while ((*text) != 0) {
    serial_send(*text);
    text++;
  }
}

void serial_flush() {
  while (rx_head != rx_tail)
    ;
#ifndef SEND_METHOD_POLL
  while (tx_head != tx_tail)
    ;
#endif
}

#if 0
inline uint8_t hex2ascii(uint8_t hex) {
  return (hex < 0xA) ? hex + '0' : hex - 0xA + 'A';
}

void send_hexdump(uint8_t hex) {
  serial_send(hex2ascii((hex >> 4) & 0x0f));
  serial_send(hex2ascii(hex & 0x0f));
}
#endif

// recive to large rx buffer
void hostsync_receive(uint16_t length) {
  //Send 'R'equest Signal
  serial_send('R');
  uint16_t i = length;
  uint16_t o = 0;

  serial_flush();
  noInterrupts();
  do {
    while (!(UCSR0A & _BV(RXC0)))
      ;
    large_rx_buf[o++] = UDR0;
  } while (--i);
  interrupts();
}


#ifndef SEND_METHOD_POLL
ISR(USART_UDRE_vect) {
  if (tx_head == tx_tail) {
    // データ無し → 割り込み停止
    UCSR0B &= ~(1 << UDRIE0);
    return;
  }

  UDR0 = tx_buf[tx_tail];
  tx_tail = (tx_tail + 1) & TX_BUF_MASK;
}
#endif

ISR(USART_RX_vect) {
  uint8_t data = UDR0;
  uint8_t next = (rx_head + 1) & RX_BUF_MASK;

  if (next == rx_tail) {
#ifdef RX_OVERFLOW_WARN
    rx_overflow = 1;
#endif
  } else {
    rx_buf[rx_head] = data;
    rx_head = next;
  }
}

#pragma GCC pop_options
