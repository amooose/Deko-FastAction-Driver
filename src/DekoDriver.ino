/*
Copyright (C) 2017 Steve Benz <s8878992@hotmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
USA
*/
#include "ps2_Keyboard.h"
#include "ps2_DekoCodes.h"
#include "ps2_DekoLEDs.h"
#include "ps2_SimpleDiagnostics.h"
#include "ps2_UsbTranslator.h"
#include "HID-Project.h"
#define arrLength(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
// Create a log of all the data going to and from the keyboard and the host.
class Diagnostics: public ps2::SimpleDiagnostics<512, 60>{
    typedef ps2::SimpleDiagnostics<512, 60> base;
    enum class UsbTranslatorAppCode : uint8_t {
        sentUsbKeyDown = 0 + base::firstUnusedInfoCode,
        sentUsbKeyUp = 1 + base::firstUnusedInfoCode,
};

public:
    void sentUsbKeyDown(byte b) { this->push(UsbTranslatorAppCode::sentUsbKeyDown, b); }
    void sentUsbKeyUp(byte b) { this->push(UsbTranslatorAppCode::sentUsbKeyUp, b); }
};

static Diagnostics diagnostics;
static ps2::UsbTranslator<Diagnostics> keyMapping(diagnostics);
static ps2::Keyboard<4,3,1, Diagnostics> ps2Keyboard(diagnostics);
static ps2::UsbKeyboardLeds ledValueLastSentToPs2 = ps2::UsbKeyboardLeds::none;
bool LCDTYPEMODE = false;
// This example demonstrates how to create keyboard translations (in this case, how the caps lock
//  and other modifier keys are laid out).  The example reads switch1Pin to toggle whether this
//  behavior is on or off.
// the setup function runs once when you press reset or power the board

byte ToByte(bool b[8])
{
    unsigned char c = 0;
    for (int i=0; i < 8; ++i)
        if (b[i])
            c |= 1 << i;
    return (byte)c;
}

void sendDekoCMD(DekoCode cmd, byte data){
    byte cmdarr [2] = {cmd,data};
    ps2Keyboard.sendBytes(cmdarr);
}

void DekoSendLCDBoolArray(bool* settings){
    byte displayCMD = ToByte(settings);
    sendDekoCMD(DekoCode::DEKO_LCDINSTRUCT,displayCMD);
}

//displayOn = controls display on/off
//cursorShow = controls cursor on/off
//cursonBlinkOn = controls if cursor blinks or is static
void DekoLCDDisplaySettings(bool displayOn,bool cursorShow,bool cursonBlinkOn){
    bool settings [8] = {0,0,0,0,1,displayOn,cursorShow,cursonBlinkOn};
    DekoSendLCDBoolArray(settings);
}

void DekoLCDFunctionSet(){
    sendDekoCMD(DekoCode::DEKO_LCDINSTRUCT,0x38);
}

//IncDecrementCursor 'i' = increment cursor pos, any other char = decrement
//shiftEntireDisplay: if enabled, the display will be shifted to the left (if I/D = 1) or right (if I/D = 0)
void DekoLCDEntryMode(char IncDecrementCursor, bool shiftEntireDisplay){
    bool incDecSetting = (IncDecrementCursor == 'i') ? true : false;
    bool settings [8] = {0,0,0,0,0,1,incDecSetting,shiftEntireDisplay};
    DekoSendLCDBoolArray(settings);
}

//Clear LCD, by default we reset the cursor.
void DekoLCDClear(bool cursorReset=true){
    sendDekoCMD(DekoCode::DEKO_LCDINSTRUCT,0x01);
    if(cursorReset){
        sendDekoCMD(DekoCode::DEKO_LCDINSTRUCT,0x02);
    }
}

bool relightBusy = false;
void DekoLED_LEDOFF_RELIGHT(){
    
    if(relightBusy){return;}
    relightBusy=true;
    byte len = 0;
    //We dont send all 45 leds every time, only up to the length
    byte cmd[95];
    byte cmdo[1]={0xeb};
    ps2Keyboard.sendBytes(cmdo,1,1000);
    ps2Keyboard.notTransmitting();

    for (int i =0; i < 45; i++) {
            uint8_t stat = (uint8_t)DekoLED::DekoLEDArray[i][0];
            uint8_t led = i+(uint8_t)0xC0u;

            switch(stat){
                case (uint8_t)1:
                {
                    cmd[len++]= DekoCode::DEKO_LEDON;
                    cmd[len++]=(byte)led;
                }
                break;
                case (uint8_t)3:
                {
                    cmd[len++]= DekoCode::DEKO_LEDBLINK;
                    cmd[len++]=(byte)led;
                }
                break;            
            }
    }

    //These byte send timing are sensitive, they work
    //fine without, but 80/900 seem like they protect
    //against sending bad data if you spam keys
    bool sent = false;
    if(!ps2Keyboard.isTransmitting()){
        sent = ps2Keyboard.sendBytes(cmd,len,80);
        ps2Keyboard.notTransmitting();
    }
    //We sent bad bytes prob due to spamming led keys, retry
    //but send slower.
    while(!sent){
        sent =ps2Keyboard.sendBytes(cmd,len,900);
        ps2Keyboard.notTransmitting();
    }
    relightBusy = false;
}

void DekoLEDToggle(DekoLED::LED led){
    noInterrupts();
    bool status = DekoLED::toggleDekoLED(led);
    if(status){
        delay(15);
        sendDekoCMD(DekoCode::DEKO_LEDON,(byte)led);
    } else{
        DekoLED_LEDOFF_RELIGHT();
    }
    ps2Keyboard.notTransmitting();
    interrupts();
}

void DekoLEDBlinkToggle(DekoLED::LED led){
    noInterrupts();
    uint8_t status = DekoLED::blinkToggleDekoLED(led);
    if(status==3||status==1){
        delay(15);
        sendDekoCMD(DekoCode::DEKO_LEDBLINK,(byte)led);
    } else{
        DekoLED_LEDOFF_RELIGHT();
    }
    ps2Keyboard.notTransmitting();
    interrupts();
}


bool LCDBusy = false;
bool LEDBusy = false;
//Outputs a string to the LCD, given initial position 'pos' and string 'str'
//If you wish to leave position as it was left previously, set updatePos to false.
void DekoLCDDisplayString(String str, int pos=0, bool updatePos=true, bool sec=false){
    while(LCDBusy&&!sec){
        //Serial.println("busy");
    }
    LCDBusy = true;
    //set initial position from argument
    if(pos>=20){
        pos+=44;
    }
    if(updatePos){
        sendDekoCMD(DekoCode::DEKO_LCDCURSORPOS,(byte)pos);
    }
    String tmp;
    if(str.length()>20){
        tmp=str.substring(20);
        str=str.substring(0,20);
    }
    //build cmd byte array
    byte charInst = (DekoCode::DEKO_LCDCHAROUT);
    byte fullCMD [str.length()*2];
    int j = 0;
    for (int i = 0; j < str.length(); i+=2) {
        fullCMD[i] = charInst;
        fullCMD[i+1] = (byte)str.charAt(j);
        j++;
        pos++;
        
    }

    //execute cmds
    ps2Keyboard.sendBytes(fullCMD, arrLength(fullCMD));
    if(tmp.length()>=1){
        DekoLCDDisplayString(tmp, pos,updatePos,true);
    }
    LCDBusy=false;
    
}

//Deko outputs line 1: 0-19, line 2: 64-83
//HD44780 weirdness? Anyways this maps it to 0-39.
byte LCDPos(byte pos){
    return pos>19 ? pos+44 : pos;
}

//Displays a single character
void DekoLCDDisplayChar(char ch, byte pos=0){
    //execute cmds
    sendDekoCMD(DekoCode::DEKO_LCDCURSORPOS,pos);
    sendDekoCMD(DekoCode::DEKO_LCDCHAROUT,(byte)ch);
}

static KeyboardKeycode optionalRemap(KeyboardKeycode usbKeystroke,uint8_t gest)
{
    KeyboardKeycode enumVar = usbKeystroke;
    switch (enumVar)
    {
        /*
        case KeyboardKeycode::DEKOKEY_CHAR:{
            return HID_KEYBOARD_LEFTARROW;
            break;
        }
        case KeyboardKeycode::DEKOKEY_PRESETMACRO:{
            return KEY_LEFT_WINDOWS;
            break;
        }
        case KeyboardKeycode::DEKOKEY_ROW:{
            return HID_KEYBOARD_DOWNARROW;
            break;
        }
        case KeyboardKeycode::DEKOKEY_LAYER:{
            return HID_KEYBOARD_RIGHTARROW;
            break;
        }
        case KeyboardKeycode::HID_KEYBOARD_DOWNARROW:{
            return HID_KEYBOARD_UPARROW;
            break;
        }
        case KeyboardKeycode::HID_KEYBOARD_UPARROW:{
            return KeyboardKeycode::DEKOKEY_CHAR;
            break;
        }
        case KeyboardKeycode::HID_KEYBOARD_LEFTARROW:{
            return KeyboardKeycode::DEKOKEY_ROW;
            break;
        }
        case KeyboardKeycode::HID_KEYBOARD_RIGHTARROW:{
            return KeyboardKeycode::DEKOKEY_LAYER;
            break;
        }
        */
        default:{

        }
    }
    //Execute on keyup only (gesture=0).
    if(gest == 0){
        switch (enumVar)
        {
            case KeyboardKeycode::DEKOKEY_MACROEDIT:{
                DekoLCDClear();
                DekoLCDDisplayString("EGG STATUS:         DELIVERED (YUM)", 0,true,false);
                break;
            }
            default:{
                // is likely to be an error
            }
        }

    }
    return usbKeystroke;
}


bool fullIntro = false;
String introMsg = "v2.0";
uint8_t introIndex = 0;
uint8_t cind = 80;
void playIntro(){
    char tmp = 16;
    static int last_time = millis();
    int current_time = millis();
    if(current_time - last_time >= 15){
        String sc = String(introMsg.charAt(introIndex));
        char letter = introMsg.c_str()[introIndex];
        while(tmp<=25){
            DekoLCDDisplayChar(tmp,cind);
            tmp+=1;
        }
        DekoLCDDisplayChar(letter,cind++);
        introIndex++;
        tmp = 0;
        last_time = current_time;
        if(introIndex==introMsg.length()){
            fullIntro=true;
        }
    }
}
/*
void DekoLCDDisplayStringCenter(String msg){
    int wid = 20; //for your 20x1
    int row = 0;  //always 1st row      
    int len = msg.length();
    int pos=(20 - len) / 2;
    //lcd.print(msg);
}*/

void setup() {
    //Delay as Deko needs time to boot up before we send startup signal.
    //1040ms seems like the absolute lowest to not produce an error.
    delay(1150);
    Serial.begin( 9600 );
    ps2Keyboard.begin();
    DekoLED::initDekoArray();
    diagnostics.reset();
    ps2Keyboard.reset();
    BootKeyboard.releaseAll();
    BootKeyboard.flush();
}

//We use 0xA2 to 0xDF as they are unused in Windows.. i hope
void handleDekoKey(KeyboardKeycode &hidCode, KeyboardKeycode &sKey){
    if(hidCode>=0xA5&&hidCode<0xDF){
        unsigned int index = (hidCode-0xA5u);
        //Assign F13-F24
        hidCode = (KeyboardKeycode)(104u+(index%12));
        //Assign "Modifiers"
        if(index<12){
            sKey =KeyboardKeycode::KEY_INTERNATIONAL1;
        }
        if(index>=12&&index<24){
            sKey =KeyboardKeycode::KEY_INTERNATIONAL2;
        }
        if(index>=24&&index<36){
            sKey =KeyboardKeycode::KEY_INTERNATIONAL3;
        }
        if(index>=36&&index<48){
            sKey =KeyboardKeycode::KEY_INTERNATIONAL4;
        }
        if(index>=48){
            sKey =KeyboardKeycode::KEY_INTERNATIONAL5;
        }

    }
    //Handle Row/char/layer here, cause i forgot+i moved the arrow keys over em
    if(hidCode>=0xA2&&hidCode<0xA5){
        sKey =KeyboardKeycode::KEY_INTERNATIONAL6;
        hidCode = (KeyboardKeycode)(104u+(hidCode-0xA2u));
    }
}

bool blink = false;
void handleAHKInput(unsigned int input){
    if(input>0u && input<=2u){
        blink= (input==1u && input>0u)? false : true;
    }
    if(input>2){
        if(blink){
            DekoLEDBlinkToggle((DekoLED::LED)(unsigned int)input);
        } else{
            DekoLEDToggle((DekoLED::LED)(unsigned int)input);
        }
    }
}

void loop() {
    if(!fullIntro){
        playIntro();
    }
    //Handle any commands incoming from AHK
    if (Serial.available())
    {
       handleAHKInput(Serial.read());
    }

    // Handle 3 LEDS Caps/Scroll/Numlock
    ps2::UsbKeyboardLeds newLedState = (ps2::UsbKeyboardLeds)BootKeyboard.getLeds();
    if (newLedState != ledValueLastSentToPs2){
        ps2Keyboard.sendLedStatus(keyMapping.translateLeds(newLedState));
        ledValueLastSentToPs2 = newLedState;
    }

    // Handle ps2 scanCode to USB HID
    ps2::KeyboardOutput scanCode = ps2Keyboard.readScanCode();
    if (scanCode != ps2::KeyboardOutput::none && scanCode != ps2::KeyboardOutput::garbled){
        ps2::UsbKeyAction action = keyMapping.translatePs2Keycode(scanCode);
        KeyboardKeycode hidCode = (KeyboardKeycode)action.hidCode;
        Serial.println("INCOME:");
        Serial.println(hidCode,HEX);
        hidCode = optionalRemap(hidCode,(uint8_t)action.gesture);
        KeyboardKeycode sKey=DEKOKEY_NONE;
        handleDekoKey(hidCode,sKey);
        
        switch (action.gesture) {
            case ps2::UsbKeyAction::KeyDown:{
                //check if its a deko macro key
                if(sKey!=DEKOKEY_NONE){
                    BootKeyboard.press(sKey);
                }
                BootKeyboard.press(hidCode);
            }
            break;

            case ps2::UsbKeyAction::KeyUp:{
                //check if its a deko macro key
                if(sKey!=DEKOKEY_NONE){
                    BootKeyboard.release(sKey);
                }
                BootKeyboard.release(hidCode);
            }
            break;
        }
    }
}