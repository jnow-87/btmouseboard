#include <config.h>
#include <driver/usb_serial_jtag.h>
#include <hal/usb_serial_jtag_ll.h>
#include <firmware/blekeyboard.h>
#include <firmware/blemouse.h>
#include <protocol.h>


/* macros */
#define LEN(x)	(sizeof(x) / sizeof(x[0]))


/* local/static prototypes */
static response_t ping(hdr_t hdr);
static response_t close(hdr_t hdr);
static response_t key(hdr_t hdr);
static response_t button(hdr_t hdr);
static response_t scroll(hdr_t hdr);
static response_t move(hdr_t hdr);

static char translate(char key);

static uint8_t read(void);
static void write(uint8_t c);

static void led_toggle(void);


/* static variables */
static uint8_t nonascii_keys[] = {
	KEY_LEFT_CTRL,
	KEY_LEFT_SHIFT,
	KEY_LEFT_ALT,
	KEY_LEFT_GUI,
	KEY_RIGHT_CTRL,
	KEY_RIGHT_SHIFT,
	KEY_RIGHT_ALT,
	KEY_RIGHT_GUI,
	KEY_UP_ARROW,
	KEY_DOWN_ARROW,
	KEY_LEFT_ARROW,
	KEY_RIGHT_ARROW,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_RETURN,
	KEY_ESC,
	KEY_INSERT,
	KEY_PRTSC,
	KEY_DELETE,
	KEY_PAGE_UP,
	KEY_PAGE_DOWN,
	KEY_HOME,
	KEY_END,
	KEY_CAPS_LOCK,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_F13,
	KEY_F14,
	KEY_F15,
	KEY_F16,
	KEY_F17,
	KEY_F18,
	KEY_F19,
	KEY_F20,
	KEY_F21,
	KEY_F22,
	KEY_F23,
	KEY_F24,
	KEY_NUM_0,
	KEY_NUM_1,
	KEY_NUM_2,
	KEY_NUM_3,
	KEY_NUM_4,
	KEY_NUM_5,
	KEY_NUM_6,
	KEY_NUM_7,
	KEY_NUM_8,
	KEY_NUM_9,
	KEY_NUM_SLASH,
	KEY_NUM_ASTERISK,
	KEY_NUM_MINUS,
	KEY_NUM_PLUS,
	KEY_NUM_ENTER,
	KEY_NUM_PERIOD,
	KEY_NUM_LOCK,
};

static uint8_t mouse_buttons[] = {
	0,
	MOUSE_LEFT,
	MOUSE_MIDDLE,
	MOUSE_RIGHT,
};

static response_t(*cmds[])(hdr_t hdr){
	0x0,
	ping,
	close,
	key,
	key,
	button,
	button,
	scroll,
	scroll,
	move,
};

static BleKeyboard kb("rc-mouseboard", "brickworks", 42);
static BleMouse mouse(&kb);


/* global functions */
void setup(){
	usb_serial_jtag_driver_config_t jtag_cfg = {
		.tx_buffer_size = 128,
		.rx_buffer_size = 128,
	};


	pinMode(CONFIG_FW_LED_PORT, OUTPUT);
	digitalWrite(CONFIG_FW_LED_PORT, HIGH);

#ifdef CONFIG_UART_MODE_USB_JTAG
	usb_serial_jtag_driver_install(&jtag_cfg);
#else
	Serial.begin(CONFIG_UART_BAUDRATE);
	while(!Serial);
	Serial.setTimeout(5000);
#endif // CONFIG_UART_MODE_USB_JTAG

	kb.begin();
	mouse.begin();
}

void loop(){
	hdr_t hdr;
	response_t resp;


	hdr = (hdr_t)read();
	resp = (hdr > 0 && hdr < HDR_MAX) ? cmds[hdr](hdr) : RESP_EINVAL_CMD;

	write(resp);
	led_toggle();
}


/* local functions */
static response_t ping(hdr_t hdr){
	return RESP_MAGIC;
}

static response_t close(hdr_t hdr){
	kb.releaseAll();

	for(uint8_t i=0; i<4; i++)
		mouse.release(i);

	return RESP_OK;
}

static response_t key(hdr_t hdr){
	char key;


	key = translate(read());

	if(!kb.isConnected())
		return RESP_ENOCON;

	if(key == 0)
		return RESP_EINVAL_KEY;

	if(hdr == HDR_KEY_PRESS)
		kb.press(key);

	if(hdr == HDR_KEY_RELEASE)
		kb.release(key);

	return RESP_OK;
}

static response_t button(hdr_t hdr){
	uint8_t button;


	button = read();

	if(!kb.isConnected())
		return RESP_ENOCON;

	if(button >= LEN(mouse_buttons))
		return RESP_EINVAL_KEY;

	if(hdr == HDR_BUTTON_PRESS)
		mouse.press(mouse_buttons[button]);

	if(hdr == HDR_BUTTON_RELEASE)
		mouse.release(mouse_buttons[button]);

	return RESP_OK;
}

static response_t scroll(hdr_t hdr){
	int8_t v;


	v = read();

	if(!kb.isConnected())
		return RESP_ENOCON;

	if(hdr == HDR_VSCROLL)
		 mouse.move(0, 0, v, 0);

	if(hdr == HDR_HSCROLL)
		 mouse.move(0, 0, 0, v);

	return RESP_OK;
}

static response_t move(hdr_t hdr){
	uint8_t dx,
					dy;


	dx = read();
	dy = read();

	if(!kb.isConnected())
		return RESP_ENOCON;

	mouse.move(dx, dy, 0, 0);

	return RESP_OK;
}

static char translate(char key){
	if(key >= 32 && key < 127)
		return key;

	if(key >= 127 && key < 127 + LEN(nonascii_keys))
		return nonascii_keys[key - 127];

	switch(key){
	case 27:	return KEY_ESC;
	case 8:		return KEY_BACKSPACE;
	case 13:	return KEY_RETURN;
	case 127:	return KEY_BACKSPACE;
	}

	return 0;
}

static uint8_t read(void){
	uint8_t c;


#ifdef CONFIG_UART_MODE_USB_JTAG
	while(usb_serial_jtag_read_bytes(&c, 1, 0) != 1);
#else
	while(Serial.readBytes((uint8_t*)&c, 1) != 1);
#endif // CONFIG_UART_MODE_SERIAL

	return c;
}

static void write(uint8_t c){
#ifdef CONFIG_UART_MODE_USB_JTAG
	usb_serial_jtag_write_bytes(&c, 1, 0);
	usb_serial_jtag_ll_txfifo_flush();
#else
	Serial.write(c);
#endif // CONFIG_UART_MODE_USB_JTAG
}

static void led_toggle(void){
	static bool state = false;


	digitalWrite(CONFIG_FW_LED_PORT, state ? HIGH : LOW);
	state = !state;
}
