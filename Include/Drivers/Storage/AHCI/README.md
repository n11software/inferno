# AHCI SATA Driver

This is a driver for SATA devices using the AHCI (Advanced Host Controller Interface) standard. AHCI is the preferred interface for SATA devices as it provides features like Native Command Queuing (NCQ), hot-plugging, and better performance compared to legacy IDE interfaces.

## Features

- AHCI 1.3.1 specification compliant
- Support for SATA devices
- 48-bit LBA addressing
- Read and write operations
- Device identification
- Command slot management
- Multiple PRD (Physical Region Descriptor) support for large transfers

## API Overview

### Initialization

```cpp
// Initialize the AHCI controller
int ahci_init(uint16_t vendor_id, uint16_t device_id);
```

This function initializes the AHCI controller and scans all ports for SATA devices. It returns the number of SATA devices found or a negative value if an error occurred.

### Device Information

```cpp
// Get information about a device
ahci_device_t* ahci_get_device_info(int port_num);
```

This function returns a pointer to a structure containing information about the device connected to the specified port, including:
- Model name
- Serial number
- Firmware revision
- Total sector count
- Sector size

### Reading and Writing

```cpp
// Read sectors from a device
int ahci_read_sectors(int port_num, uint64_t start, uint32_t count, void* buffer);

// Write sectors to a device
int ahci_write_sectors(int port_num, uint64_t start, uint32_t count, const void* buffer);
```

These functions read or write a specified number of sectors from/to the device. They return the number of bytes read/written or a negative value if an error occurred.

## Usage Example

```cpp
#include <Drivers/Storage/AHCI/AHCI.h>

void example() {
    // Initialize AHCI
    int devices = ahci_init(vendor_id, device_id);
    if (devices <= 0) {
        // Error handling
        return;
    }
    
    // Find a device
    int port_num = -1;
    for (int i = 0; i < AHCI_MAX_PORTS; i++) {
        ahci_device_t* dev = ahci_get_device_info(i);
        if (dev && dev->is_present) {
            port_num = i;
            break;
        }
    }
    
    if (port_num < 0) {
        // No devices found
        return;
    }
    
    // Get device info
    ahci_device_t* dev = ahci_get_device_info(port_num);
    printf("Device: %s\n", dev->model);
    
    // Read data
    void* buffer = malloc(dev->sector_size);
    int result = ahci_read_sectors(port_num, 0, 1, buffer);
    
    // Process data...
    
    // Write data
    result = ahci_write_sectors(port_num, 1, 1, buffer);
    
    free(buffer);
}
```

## Implementation Details

The driver uses memory-mapped I/O to communicate with the AHCI controller. It allocates memory for command lists, FIS (Frame Information Structure) buffers, and command tables for each port.

### Memory Requirements

The driver allocates the following memory per port:
- Command list: 1KB
- Received FIS: 256 bytes
- Command tables: 32 * 256 bytes = 8KB

### Limitations

- The driver assumes physical address = virtual address. In a real OS with paging, you'd need to convert virtual addresses to physical.
- Error recovery is minimal.
- No support for ATAPI devices, port multipliers, or hot-plugging.
- No DMA alignment checking.

## Future Improvements

- Support for ATAPI devices
- Hot-plug support
- Advanced error recovery
- Power management
- NCQ optimization
- Support for port multipliers 