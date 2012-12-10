#include <avr/io.h>
#include <avr/interrupt.h>
#include "utilities.h"
#include <avr/eeprom.h>
#include "eeprom.h"

//Handles non-volatile memory (store/load config data through power-off)
//Blocks on read/write (non-ISR based)
//http://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&cad=rja&ved=0CB8QFjAA&url=http%3A%2F%2Fwww.teslabs.com%2Fopenplayer%2Fdocs%2Fdocs%2Fprognotes%2FEEPROM%2520Tutorial.pdf&ei=OmlyUJ-aH6bAiwLnsIGwDA&usg=AFQjCNEaFVSAXVm66KdG1BUxwXPCzv24ZA&sig2=WBp_OfnVAHFZUW_48YAU7Q


//#############################################################
//## EEPROM PAGE STRUCTURE
//#############################################################

uint16_t EEMEM nv_dac_background; //the DAC output value for the background subtraction
uint16_t EEMEM nv_dac_threshold; //the DAC output value for the comparator threshold



//#############################################################
//## Initialization
//#############################################################

void init_eeprom(){
	eeprom_restore();
}



//#############################################################
//## API
//#############################################################

void eeprom_save(){
	eeprom_write_block((void*)&dac_background, (void*)&nv_dac_background, 2); //multiple size by 2 since uint16_t = 2 bytes
	eeprom_write_block((void*)&dac_threshold, (void*)&nv_dac_threshold, 2); //multiple size by 2 since uint16_t = 2 bytes
}

void eeprom_restore(){
	eeprom_read_block((void*)&dac_background, (void*)&nv_dac_background, 2);
	eeprom_read_block((void*)&dac_threshold, (void*)&nv_dac_threshold, 2);
	dac_out0(dac_background);
	dac_out1(dac_threshold);
}

//mostly for debugging
uint8_t eeprom_read(){
	return eeprom_read_byte(&nv_dac_background);
}

//#############################################################
//## Service
//#############################################################

//2ms Service Loop -- Call once every 2ms
void service_eeprom(){
	
}
