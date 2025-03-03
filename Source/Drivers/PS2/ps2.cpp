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
		prDebug("ps2", "ps2 status reg: %x", status);
		
		outb(PS2_COMMAND, 0xAD);
		prDebug("ps2", "sent disable first port cmd");

		HPET::Wait(10);

		status = inb(PS2_STATUS);
		prDebug("ps2", "ps2 status reg: %x", status);

		uint8_t res = inb(PS2_DATA);
		prDebug("ps2", "ps2 controller response: %x", res);
	
		return (status != 0xFF);
	}

	int init() {
		if (!detectPS2Controller()) {
			ACPI::checkForPS2Controller();
		}

		return 0;
	}
}
