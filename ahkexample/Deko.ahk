
IfExist, %I_Icon%
    Menu, Tray, Icon, %I_Icon%
;return
#Persistent
#InstallKeybdHook
#SingleInstance Force
#Include C:\Users\Dylan\Documents\changeAudio.ahk

sendData(data){
	port := FileOpen("COM9", "w") ; SET YOUR COM PORT
	port.Write(Chr(data))
	port.__Handle ; This flushes the write buffer.
	port.Close() ; and close the Port
	return
}

setSolidLED() {
    sendData(1)
	return
}
setBlinkLED() {
    sendData(2)
	return
}

SC070 & F14 Up::
SetDefaultEndpoint( GetDeviceID(Devices, "Headset") )
setSolidLED()
sendData(205) ;205 = 0xCE in ps2_DekoLEDs.h aka PGM B key
return
SC070 & F15 Up::
SetDefaultEndpoint( GetDeviceID(Devices, "Speakers") )
setSolidLED()
sendData(206) ;206 = 0xCE in ps2_DekoLEDs.h aka PGM A key
return

