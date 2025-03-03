#include <Inferno/IO.h>
#include <Inferno/Log.h>
#include <Inferno/stdint.h>
#include <Interrupts/HPET.hpp>

#include <Drivers/ACPI/acpi.h>
#include <Drivers/PS2/ps2.h>

#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64
#define PS2_DATA 0x60

namespace PS2 {
	bool detectPS2Controller() {
		uint8_t status = inb(PS2_STATUS);
		prDebug("ps2kb0", "ps2 status reg: %x", status);
		
		outb(PS2_COMMAND, 0xAD);
		prDebug("ps2kb0", "sent disable first port cmd");

		HPET::Wait(10);

		status = inb(PS2_STATUS);
		prDebug("ps2kb0", "ps2 status reg: %x", status);

		uint8_t res = inb(PS2_DATA);
		prDebug("ps2kb0", "ps2 controller response: %x", res);
	
		return (status != 0xFF);
	}

	bool isControllerReady() {
		uint8_t status = inb(PS2_STATUS);
		return (status & 0x02) == 0;
	}

	void enablePS2Controller() {
		prInfo("ps2kb0", "enabling ps/2 controller...");
		outb(PS2_COMMAND, 0xAE);
		HPET::Wait(100);

		if (!isControllerReady()) {
			prErr("ps2kb0", "PS/2 controller is not ready, aborting...");
			return;
		}
		prInfo("ps2kb0", "PS/2 controller enabled sucessfully");
	}

	void enablePS2Keyboard() {
		prInfo("ps2kb0", "enabling ps/2 keyboard...");
		if (!isControllerReady()) {
			prErr("ps2kb0", "PS/2 controller is not ready to send keyboard enable command...");
			return;
		}

		prDebug("ps2kb0", "sending keyboard enable command...");
		outb(0x64, 0xF5);
		prDebug("ps2kb0", "waiting before getting response");
		HPET::Wait(100);
		uint8_t response = inb(0x60);
		prInfo("ps2kb0", "keyboard response: 0x%x", response);

		if (response == 0xFA) {
			prInfo("ps2kb0", "PS/2 keyboard initalized successfully");
		} else {
			prErr("ps2kb0", "PS/2 keyboard failed to initalize");
		}
	}

	void init() {
		prInfo("ps2kb0", "initializing PS/2 controller");

		if (detectPS2Controller() || ACPI::checkForPS2Controller()) {
			prInfo("ps2kb0", "PS/2 controller detected");
			enablePS2Controller();
			enablePS2Keyboard();
		} else {
			prWarn("ps2kb0", "No PS/2 Controller detected, skipping initalization");
		}
	}
}
