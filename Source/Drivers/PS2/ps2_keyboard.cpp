#include <Inferno/IO.h>
#include <Inferno/Log.h>
#include <Inferno/stdint.h>

#include <Drivers/PS2/ps2.h>

namespace PS2 {
	uint8_t readKey() {
		if (isControllerReady()) {
			uint8_t scancode = inb(0x60);

			prDebug("ps2kb0", "received scancode: 0x%x", scancode);

			if (scancode != 0) {
				return scancode;
			}
		} else {
			prDebug("ps2kb0", "PS/2 controller is not ready...");
		}

		return 0;
	}
}
