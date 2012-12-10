#ifndef __dac_h
#define __dac_h

	//DEFINITIONS

	//GLOBAL STATE
		uint16_t dac_background;
		uint16_t dac_threshold;
		
	//FUNCTIONS
		void init_dac();
		void dac_output0(uint8_t config);
		void dac_output1(uint8_t config);
		uint16_t dac_out0(uint16_t value);
		uint16_t dac_out1(uint16_t value);
		uint16_t dac_read0(void);
		uint16_t dac_read1(void);
		void service_dac();
#endif
