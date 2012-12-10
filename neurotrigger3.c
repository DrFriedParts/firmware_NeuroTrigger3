#include <avr/io.h>
#include <avr/interrupt.h>
#include "utilities.h"
#include "uart.h"
#include "ui.h"
#include "dac.h"
#include "adc.h"
#include "ac.h"
#include "eeprom.h"
#include "neurotrig.h"
#include "pwm.h"
#include "uart_buffer.h"

uint8_t STATE_Autolevel = AUTOLEVEL_IDLE;

int main(void){
	uint8_t blah;
	uint16_t blah16;
	int8_t updown;
	uint16_t push_counter = 0;
	uint8_t push_blinker = 0;

	//[LED's, Button, & Switches]
		init_ui(); //init LED's first so that they are available for debugging

	//[CPU CLOCK]
		//Boot up and configure oscillator
			OSC.XOSCCTRL = B8(00100010); //enable external 32kHz Xtal using low-power (e.g. low-swing) mode
			OSC.CTRL = B8(00001011); //enable 32M-RC & External Xtal -- also "enable" 2M-RC since its already running b/c we booted from it and can't actually disable it until we switch sources
		//Wait for stability
			led_on(LED_LEFT);
			//This is actually tricky sequencing because we boot from the 2MHz internal RC so previous write to OSC.CTRL was ineffective at shutting down the 2M-RC so OSC.STATUS will still reflect that it is running
			while(OSC.STATUS != B8(00001011)); //stall for external xtal and 32M-RC stability
			led_off(LED_LEFT);
		//Configure 
			OSC.DFLLCTRL = B8(00000010); //use external xtal for 32M-RC calibration
			DFLLRC32M.CTRL = B8(00000001); //enable Xtal calibration of internal 32MHz RC oscillator
		//Switch system clock over to stable RC oscillator
			//Switch to 32M-RC as system clock source and disable the 2M-RC that we booted from.
			//----REQUIRES CONFIGURATION PROTECTION REGISTER 
			CCP = CCP_IOREG_gc; //disable change protection for IO register
			CLK.CTRL = CLK_SCLKSEL_RC32M_gc;
			OSC.CTRL = B8(00001010); //re-execute this write -- this will shutdown the 2M-RC since we are no longer running from it.
		//Now running live at 32MHz		

	//[UARTs]
		init_uart(&uctrl, BAUD_115200);
		init_uart_buffer(&uctrl);
		init_uart(&udata, BAUD_115200);

	//[ADC]
		init_adc();

	//[DAC]
		//dac0 is background level
		//dac1 is threshold level
		init_dac();

	//[AC]
		init_ac();

	//[PWM] 
		init_pwm(); // and serial trigger

	//[EEPROM]
		init_eeprom();

	//[Realtime Loop Timer]
		//Use PortC's T/C0
		TCC0.CTRLA = 0x07; //Start the timer; Div1024 operation = 32M/1024 = 31250
		//TCC0.PER = 31; //992uS per timer period
		TCC0.PER = 61;

	//[PMIC (Interrupt Controller)]
		PMIC.CTRL = B8(10000111); //enable all three interrupt levels (lowest one with round-robin)
		sei(); //ENABLE INTERRUPTS AND GO LIVE!

	//[RTOS START!]	
		blah = 1;
		blah16 = 0;
		updown = 1;
		led_off(LED_LEFT);
		led_off(LED_MID);

		while(1){
			led_dim(LED_LEFT, blah);
			
			blah += updown;			
			if ((blah == 255) || (blah == 0)) {
				updown = -1 * updown;
			}
			
			blah16++;
			
			service_pwm();
			service_switches();
			service_leds();
			service_button();
			service_uart_buffer();
			service_eeprom();
			
			if (STATE_Button == BUTTON_PUSHED){
				if (pwm_mode() == false){
					//DIODE mode
					if (++push_counter >= EEPROM_HOLD_TO_SAVE_TIME){
						STATE_Autolevel = AUTOLEVEL_DIODE;
						led_on(LED_LEFT);
						led_on(LED_MID);						
					}						
				}
				else{
					//AUDIO mode
					//--deprecated, no longer supported.
				}					
			}
			else{
				push_counter = 0;
			}

			//PERFORM AUTO-LEVELING!
				//adc is 503uV resolution
				//dac is 806uV resolution
				//ergo ADC -> DAC values must be adjusted by: (x*0.625 = x*5/8)
			switch(STATE_Autolevel){
			case AUTOLEVEL_DIODE:
				//Determine background level
					adc_ch1_stats(1024);
				//Set background level
					dac_background = (adc_avg*5)/8 + 170; //background (x*0.625 = x*5/8)
					dac_threshold = dac_background + 200;
					dac_out0(dac_background); 
					dac_out1(dac_threshold);
				//Write to permanent memory
					eeprom_save();
				//We're done!
					led_off(LED_LEFT);
					led_off(LED_MID);
					STATE_Autolevel = AUTOLEVEL_DONE;
				break;
			case AUTOLEVEL_DONE:
				if (STATE_Button == BUTTON_RELEASED){
					STATE_Autolevel = AUTOLEVEL_IDLE;
					led_off(LED_LEFT);
					led_off(LED_MID);
				}
				else {
					if (++push_blinker > 10){
						led_toggle(LED_LEFT);
						led_toggle(LED_MID);
						push_blinker = 0;
					}
				}
			case AUTOLEVEL_IDLE:
			default:		
				break;
			}
					
			while((TCC0.INTFLAGS & _BV(0)) != 0x01); //Wait for the loop time to expire
			TCC0.INTFLAGS = 0x01; //Clear the interrupt flag
		}//while
}//int main(void)
