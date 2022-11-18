

#include <stdint.h>
namespace DekoLED {
	enum class LED : uint8_t {
		DEKO_LED_CMD = 0xC0,
		DEKO_LED_LEARN = 0xC1,
		DEKO_LED_BROWSETYPEFACE = 0xC2,
		DEKO_LED_STYLE1 = 0xC3,
		DEKO_LED_STYLE2 = 0xC4,
		DEKO_LED_STYLE3 = 0xC5,
		DEKO_LED_STYLE4 = 0xC6,
		DEKO_LED_STYLE5 = 0xC7,
		DEKO_LED_STYLE6 = 0xC8,
		DEKO_LED_STYLE7 = 0xC9,
		DEKO_LED_STYLE8 = 0xCA,
		DEKO_LED_FONT = 0xCB,
		DEKO_LED_LOOK = 0xCC,
		DEKO_LED_PGMB = 0xCD,
		DEKO_LED_PGMA = 0xCE,
		DEKO_LED_MOVE = 0xCF,
		DEKO_LED_SCALE = 0xD0,
		DEKO_LED_HELP = 0xD1,
		DEKO_LED_BLANK1 = 0xD2,
		DEKO_LED_BROWSEGRAPHICS = 0xD3,
		DEKO_LED_COLOR1 = 0xD4,
		DEKO_LED_COLOR2 = 0xD5,
		DEKO_LED_COLOR3 = 0xD6,
		DEKO_LED_COLOR4 = 0xD7,
		DEKO_LED_COLOR5 = 0xD8,
		DEKO_LED_COLOR6 = 0xD9,
		DEKO_LED_COLOR7 = 0xDA,
		DEKO_LED_COLOR8 = 0xDB,
		DEKO_LED_SHADER = 0xDC,
		DEKO_LED_EFFECT = 0xDD,
		DEKO_LED_BLANK2 = 0xDE,
		DEKO_LED_BLANK3 = 0xDF,
		DEKO_LED_ROTSKW = 0xE0,
		DEKO_LED_KERN = 0xE1,
		DEKO_LED_SEQEDIT = 0xE2,
		DEKO_LED_MACROEDIT = 0xE3,
		DEKO_LED_NUMLOCK = 0xE4,
		DEKO_LED_STOP = 0xE7,
		DEKO_LED_PLAY = 0xE8,
		DEKO_LED_CAPSLOCK = 0xE9,
		DEKO_LED_PREVIEW = 0xEA,
		DEKO_LED_PROGRAM = 0xEB,
		DEKO_LED_SCROLLLOCK = 0xEC,
	};

	//Keep track of all Deko LEDs
	static uint8_t DekoLEDArray[45][1];
	void initDekoArray(){
		for (int i =0; i < 45; i++) {
			for (int j =0; j < 1; j++) {
				DekoLEDArray[i][j] = 0;
			}
		}
	}

	bool toggleDekoLED(LED val){
		uint8_t pos = (uint8_t)val-0xC0;
		uint8_t set = DekoLEDArray[pos][0];
		set=set==1? 0 : 1;
		DekoLEDArray[pos][0] = set;
		return set;
	}

	bool blinkToggleDekoLED(LED val){
		uint8_t pos = (uint8_t)val-0xC0;
		uint8_t set = DekoLEDArray[pos][0];
		set=set==3? 0 : 3;
		DekoLEDArray[pos][0] = set;
		return set;
	}
}