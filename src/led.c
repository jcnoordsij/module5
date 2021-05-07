/*
 * led.c
 *
 *  Created on: Jan 8, 2019
 *      Author: f001yfp
 */

#include "led.h"

/* led states */
#define OUTPUT_PS 1
#define LED_ON true
#define LED_OFF false

#define ALL 0xFFFFFFFF		/* A value designating ALL leds */
#define OUTPUT 0x0							/* setting GPIO direction to output */
#define OUTPUT_ENABLE 1
#define LED4 7
#define CHANNEL1 1							/* channel 1 of the GPIO port */

static XGpio port;
static XGpio portColor;
static XGpioPs portPS;
static bool led4status = LED_OFF;


/*
 * Initialize the led module
 */
void led_init(void) {

	XGpioPs_Config* XGpioConfig;

	XGpio_Initialize(&port, XPAR_AXI_GPIO_0_DEVICE_ID); /* initialize device AXI_GPIO_0 */
	XGpio_SetDataDirection(&port, CHANNEL1, OUTPUT);	    /* set tristate buffer to output */

	XGpio_Initialize(&portColor, XPAR_AXI_GPIO_1_DEVICE_ID); /* initialize device AXI_GPIO_1 for colored LEDs*/
	XGpio_SetDataDirection(&portColor, CHANNEL1, OUTPUT);	    /* set tristate buffer to output */
	//XGpio_DiscreteWrite(&portColor, CHANNEL1, 0x1);

	XGpioConfig = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);

	XGpioPs_CfgInitialize(&portPS, XGpioConfig, XPAR_PS7_GPIO_0_BASEADDR);
	XGpioPs_SetDirectionPin(&portPS, LED4, OUTPUT_PS);
	XGpioPs_SetOutputEnablePin(&portPS, LED4, OUTPUT_ENABLE);
}

/*
 * Set <led> to one of {LED_ON,LED_OFF,...}
 *
 * <led> is either ALL or a number >= 0
 * Does nothing if <led> is invalid
 */
void led_set(u32 led, bool tostate) {
	u32 ledStatus;
	u32 bit;
	u32 colorStatus;

	ledStatus = XGpio_DiscreteRead(&port, CHANNEL1);

	if (tostate == LED_ON) {
		if (led == 0) {
			bit = 0x1;
		} else if (led == 1) {
			bit = 0x2;
		} else if (led == 2) {
			bit = 0x4;
		} else if (led == 3) {
			bit = 0x8;
		} else if (led == ALL) {
			bit = 0xF;
		} else if (led == 5) { //red
			bit = 0x1;
		} else if (led == 6) { //green
			bit = 0x2;
		} else if (led == 7) { //blue
			bit = 0x4;
		} else if (led == 8) { //yellow (red and green)
			bit = 0x6;
		}
	} else {
		if (led == 0) {
			bit = 0xE;
		} else if (led == 1) {
			bit = 0xD;
		} else if (led == 2) {
			bit = 0xB;
		} else if (led == 3) {
			bit = 0x7;
		} else if (led == ALL) {
			bit = 0x0;
		}
	}

	if ((led <= 3 && led >= 0) || (led == ALL)) {
		if (tostate == LED_ON) {
			ledStatus |= bit;
		} else {
			ledStatus &= bit;
		}
		XGpio_DiscreteWrite(&port, CHANNEL1, ledStatus);
		if (tostate == LED_ON) {
			XGpioPs_WritePin(&portPS, LED4, 1);
		} else {
			XGpioPs_WritePin(&portPS, LED4, 0);
		}
	} else if (led == 4) {
		if (tostate == LED_ON) {
			XGpioPs_WritePin(&portPS, LED4, 1);
		} else {
			XGpioPs_WritePin(&portPS, LED4, 0);
		}
	} else if (led <= 8 && led >= 5) {
		if (tostate == LED_ON) {
			colorStatus = bit;
			XGpio_DiscreteWrite(&portColor, CHANNEL1, colorStatus);
		}
	}
	return;
}


/*
 * Get the status of <led>
 *
 * <led> is a number >= 0
 * returns {LED_ON,LED_OFF,...}; LED_OFF if <led> is invalid
 */
bool led_get(u32 led) {

	u32 ledStatus = 0x0;

	ledStatus = XGpio_DiscreteRead(&port, CHANNEL1);

	if (led <= 3 && led >= 0) {
		if (led == 0) {
			ledStatus &= 0x1;
		} else if (led == 1) {
			ledStatus &= 0x2;
		} else if (led == 2) {
			ledStatus &= 0x4;
		} else if (led == 3) {
			ledStatus &= 0x8;
		}
	}

	if (ledStatus) return LED_ON;
	else return LED_OFF;
}

/*
 * Toggle <led>
 *
 * <led> is a value >= 0
 * Does nothing if <led> is invalid
 */
void led_toggle(u32 led) {
	bool ledStatus;

	if (led <= 3 && led >= 0) {
		ledStatus = led_get(led);
		if (ledStatus == LED_ON) {
			led_set(led, LED_OFF);
		} else {
			led_set(led, LED_ON);
		}
	} else if (led == 4) {
		if (led4status == LED_ON) {
			led_set(4, LED_OFF);
			led4status = LED_OFF;
		} else {
			led_set(4, LED_ON);
			led4status = LED_ON;
		}
	}
	return;
}
