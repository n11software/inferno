#include <Inferno/Log.h>
#include <Inferno/stdint.h>

#include <Drivers/Keyboard/keyboard.h>
#include <Drivers/PS2/ps2_keyboard.h>

namespace kbd {
	void keyboardLoop() {
		prDebug("kb0", "Entering keyboard input loop...\n");

		while (true) {
			uint8_t scancode = PS2::readKey();
		}
	}
}
