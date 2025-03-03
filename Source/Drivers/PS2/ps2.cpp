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

		outb(0x64, 0xFE);
		HPET::Wait(200);

		prDebug("ps2kb0", "checking controller status");
		uint8_t status = inb(0x64);

		if (status & 0x02) {
			prDebug("ps2kb0", "sending command to enable...");
			outb(0x64, 0xAE);
			HPET::Wait(200);
		} else {
			prErr("ps2kb0", "Error: PS/2 controller input buffer is full. Aborting.");
			return;
		}

		if (status & 0x01) {
			uint8_t res = inb(0x60);
			prDebug("ps2kb0", "controller res: 0x%x", res);

			if (res != 0xFA) {
				prErr("ps2kb0", "error: unexpected response after enabling controller, aborting");
				return;
			}
			prInfo("ps2kb0", "PS/2 controller enabled");
		} else {
			prErr("ps2kb0", "PS/2 controller did not respond, aborting");
			return;
		}
	}

	void enablePS2Keyboard() {
		prInfo("ps2kb0", "enabling ps/2 keyboard...");

		uint8_t status;
		int tries = 0;
		while (tries < 5) {
			status = inb(0x64);
			if (status & 0x62) {
				prDebug("ps2kb0", "controller ready, status 0x%x", status);
				break;
			}
			prDebug("ps2kb0", "waiting for controller to be ready. status 0x%x", status);
			HPET::Wait(100);
			tries++;
		}

		if (tries == 5) {
			prErr("ps2kb0", "error: PS/2 controller not ready after 5 attempts.");
			return;
		} else {

			prDebug("ps2kb0", "sending keyboard enable command...");
			outb(0x64, 0xF6);
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
