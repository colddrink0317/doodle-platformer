
#include "Main.h"
#include "Input.h"


BYTE g_OldKeyState[256]{};
BYTE g_KeyState[256]{};



void UpdateInput()
{

	memcpy( g_OldKeyState, g_KeyState, 256 );

	GetKeyboardState( g_KeyState );

}

void PrimeInput()
{
	GetKeyboardState(g_KeyState);
	memcpy(g_OldKeyState, g_KeyState, 256);
}

bool GetKeyPress(BYTE KeyCode)
{
	return (g_KeyState[KeyCode] & 0x80);
}

bool GetKeyTrigger(BYTE KeyCode)
{
	return ((g_KeyState[KeyCode] & 0x80) && !(g_OldKeyState[KeyCode] & 0x80));
}

static bool IsIgnoredAnyKey(int vk)
{
	switch (vk)
	{
	case VK_LBUTTON:
	case VK_RBUTTON:
	case VK_CANCEL:
	case VK_MBUTTON:
	case VK_XBUTTON1:
	case VK_XBUTTON2:
	case VK_SHIFT:
	case VK_CONTROL:
	case VK_MENU:
	case VK_LSHIFT:
	case VK_RSHIFT:
	case VK_LCONTROL:
	case VK_RCONTROL:
	case VK_LMENU:
	case VK_RMENU:
	case VK_CAPITAL:
	case VK_NUMLOCK:
	case VK_SCROLL:
		return true;
	default:
		return false;
	}
}

bool GetAnyKeyTrigger()
{
	for (int vk = 0x08; vk <= 0xFE; ++vk)
	{
		if (IsIgnoredAnyKey(vk)) continue;
		if (GetKeyTrigger((BYTE)vk)) return true;
	}
	return false;
}
