#include "Global.h"
#include "VKey.h"
#include "WindowsMouse.h"

WindowsMouse::WindowsMouse(DeviceAPI api, int hWheel, wchar_t *displayName, wchar_t *instanceID, wchar_t *deviceID) :
Device(api, MOUSE, displayName, instanceID, deviceID) {
	int i;
	for (i=0; i<5; i++) {
		AddPhysicalControl(PSHBTN, i, i);
	}

	for (i=0; i<3+hWheel; i++) {
		AddPhysicalControl(RELAXIS, i+5, i+5);
	}
}

wchar_t *WindowsMouse::GetPhysicalControlName(PhysicalControl *control) {
	wchar_t *names[9] = {
		L"L Button",
		L"R Button",
		L"M Button",
		L"Mouse 4",
		L"Mouse 5",
		L"X Axis",
		L"Y Axis",
		L"Y Wheel",
		L"X Wheel"
	};
	if (control->id < 9) return names[control->id];
	return Device::GetPhysicalControlName(control);
}

void WindowsMouse::UpdateButton(unsigned int button, int state) {
	if (button > 4) return;
	physicalControlState[button] = (state << 16);
}

void WindowsMouse::UpdateAxis(unsigned int axis, int delta) {
	if (axis > 3) return;
	// 1 mouse pixel = 1/8th way down.
	physicalControlState[5+axis] += (delta<<(16 - 3*(axis < 2)));
}


