#pragma once


void UpdateInput();

// Snapshot the current keyboard state into both current and previous
// buffers so the next UpdateInput() call cannot synthesize a phantom
// trigger from the all-zero initial state. Call this right before
// entering a scene that watches GetKeyTrigger / GetAnyKeyTrigger.
void PrimeInput();

bool GetKeyPress( BYTE KeyCode );
bool GetKeyTrigger( BYTE KeyCode );

// True when any "main" key transitions from up to down this frame.
// Excludes mouse buttons, modifier keys (Shift/Ctrl/Alt) and lock keys
// (CapsLock/NumLock/ScrollLock) to avoid accidental triggers.
bool GetAnyKeyTrigger();

