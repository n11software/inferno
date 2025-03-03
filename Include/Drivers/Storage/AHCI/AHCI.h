#pragma once

#include <Inferno/stdint.h>

// AHCI Spec Version 1.3.1 Definitions

// Maximum number of ports supported by AHCI
#define AHCI_MAX_PORTS 32

// HBA Memory Registers offsets
#define AHCI_HBA_CAP      0x00 // Host Capabilities
#define AHCI_HBA_GHC      0x04 // Global Host Control
#define AHCI_HBA_IS       0x08 // Interrupt Status
#define AHCI_HBA_PI       0x0C // Ports Implemented
#define AHCI_HBA_VS       0x10 // Version
#define AHCI_HBA_CCC_CTL  0x14 // Command Completion Coalescing Control
#define AHCI_HBA_CCC_PORTS 0x18 // Command Completion Coalescing Ports
#define AHCI_HBA_EM_LOC   0x1C // Enclosure Management Location
#define AHCI_HBA_EM_CTL   0x20 // Enclosure Management Control
#define AHCI_HBA_CAP2     0x24 // Host Capabilities Extended
#define AHCI_HBA_BOHC     0x28 // BIOS/OS Handoff Control and Status

// Port Registers (port_num * 0x80)
#define AHCI_PORT_CLB     0x00 // Command List Base Address
#define AHCI_PORT_CLBU    0x04 // Command List Base Address Upper 32-bits
#define AHCI_PORT_FB      0x08 // FIS Base Address
#define AHCI_PORT_FBU     0x0C // FIS Base Address Upper 32-bits
#define AHCI_PORT_IS      0x10 // Interrupt Status
#define AHCI_PORT_IE      0x14 // Interrupt Enable
#define AHCI_PORT_CMD     0x18 // Command and Status
#define AHCI_PORT_TFD     0x20 // Task File Data
#define AHCI_PORT_SIG     0x24 // Signature
#define AHCI_PORT_SSTS    0x28 // Serial ATA Status
#define AHCI_PORT_SCTL    0x2C // Serial ATA Control
#define AHCI_PORT_SERR    0x30 // Serial ATA Error
#define AHCI_PORT_SACT    0x34 // Serial ATA Active
#define AHCI_PORT_CI      0x38 // Command Issue
#define AHCI_PORT_SNTF    0x3C // Serial ATA Notification
#define AHCI_PORT_FBS     0x40 // FIS-based Switching Control
#define AHCI_PORT_DEVSLP  0x44 // Device Sleep
#define AHCI_PORT_VS      0x70 // Vendor Specific

// HBA Capabilities (CAP)
#define AHCI_HBA_CAP_S64A       (1 << 31) // 64-bit Addressing
#define AHCI_HBA_CAP_SNCQ       (1 << 30) // Native Command Queuing
#define AHCI_HBA_CAP_SSNTF      (1 << 29) // SNotification Register
#define AHCI_HBA_CAP_SMPS       (1 << 28) // Mechanical Presence Switch
#define AHCI_HBA_CAP_SSS        (1 << 27) // Staggered Spin-up
#define AHCI_HBA_CAP_SALP       (1 << 26) // Aggressive Link Power Management
#define AHCI_HBA_CAP_SAL        (1 << 25) // Activity LED
#define AHCI_HBA_CAP_SCLO       (1 << 24) // Command List Override
#define AHCI_HBA_CAP_ISS_MASK   (0xF << 20) // Interface Speed Support Mask
#define AHCI_HBA_CAP_ISS_SHIFT  20 // Interface Speed Support Shift
#define AHCI_HBA_CAP_SNZO       (1 << 19) // Non-zero DMA Offsets
#define AHCI_HBA_CAP_SAM        (1 << 18) // AHCI Mode Only
#define AHCI_HBA_CAP_SPM        (1 << 17) // Port Multiplier
#define AHCI_HBA_CAP_FBSS       (1 << 16) // FIS-based Switching Support
#define AHCI_HBA_CAP_PMD        (1 << 15) // PIO Multiple DRQ Block
#define AHCI_HBA_CAP_SSC        (1 << 14) // Slumber State Capable
#define AHCI_HBA_CAP_PSC        (1 << 13) // Partial State Capable
#define AHCI_HBA_CAP_NCS_MASK   (0x1F << 8) // Number of Command Slots Mask
#define AHCI_HBA_CAP_NCS_SHIFT  8 // Number of Command Slots Shift
#define AHCI_HBA_CAP_CCCS       (1 << 7) // Command Completion Coalescing Supported
#define AHCI_HBA_CAP_EMS        (1 << 6) // Enclosure Management Supported
#define AHCI_HBA_CAP_SXS        (1 << 5) // External SATA
#define AHCI_HBA_CAP_NP_MASK    (0x1F << 0) // Number of Ports Mask
#define AHCI_HBA_CAP_NP_SHIFT   0 // Number of Ports Shift

// GHC (Global Host Control) register bits
#define AHCI_HBA_GHC_AE         (1 << 31) // AHCI Enable
#define AHCI_HBA_GHC_IE         (1 << 1)  // Interrupt Enable
#define AHCI_HBA_GHC_HR         (1 << 0)  // HBA Reset

// PORT_CMD bits
#define AHCI_PORT_CMD_ASP       (1 << 27) // Aggressive Slumber / Partial
#define AHCI_PORT_CMD_ALPE      (1 << 26) // Aggressive Link Power Management Enable
#define AHCI_PORT_CMD_DLAE      (1 << 25) // Drive LED on ATAPI Enable
#define AHCI_PORT_CMD_ATAPI     (1 << 24) // Device is ATAPI
#define AHCI_PORT_CMD_ESP       (1 << 21) // External SATA Port
#define AHCI_PORT_CMD_CPD       (1 << 20) // Cold Presence Detection
#define AHCI_PORT_CMD_MPSP      (1 << 19) // Mechanical Presence Switch Attached to Port
#define AHCI_PORT_CMD_HPCP      (1 << 18) // Hot Plug Capable Port
#define AHCI_PORT_CMD_PMA       (1 << 17) // Port Multiplier Attached
#define AHCI_PORT_CMD_CPS       (1 << 16) // Cold Presence State
#define AHCI_PORT_CMD_CR        (1 << 15) // Command List Running
#define AHCI_PORT_CMD_FR        (1 << 14) // FIS Receive Running
#define AHCI_PORT_CMD_MPSS      (1 << 13) // Mechanical Presence Switch State
#define AHCI_PORT_CMD_CCS_MASK  (0x1F << 8) // Current Command Slot Mask
#define AHCI_PORT_CMD_CCS_SHIFT 8 // Current Command Slot Shift
#define AHCI_PORT_CMD_FRE       (1 << 4)  // FIS Receive Enable
#define AHCI_PORT_CMD_CLO       (1 << 3)  // Command List Override
#define AHCI_PORT_CMD_POD       (1 << 2)  // Power On Device
#define AHCI_PORT_CMD_SUD       (1 << 1)  // Spin-Up Device
#define AHCI_PORT_CMD_ST        (1 << 0)  // Start

// PORT_TFD (Task File Data) bits
#define AHCI_PORT_TFD_ERR_MASK  (0xFF << 8) // Error Mask
#define AHCI_PORT_TFD_ERR_SHIFT 8 // Error Shift
#define AHCI_PORT_TFD_STS_MASK  (0xFF << 0) // Status Mask
#define AHCI_PORT_TFD_STS_SHIFT 0 // Status Shift
#define AHCI_PORT_TFD_STS_BSY   (1 << 7)  // Busy
#define AHCI_PORT_TFD_STS_DRQ   (1 << 3)  // Data Transfer Requested
#define AHCI_PORT_TFD_STS_ERR   (1 << 0)  // Error

// Port Interrupt Status (IS) and Interrupt Enable (IE) bits
#define AHCI_PORT_INT_CPDS      (1 << 31) // Cold Port Detect Status
#define AHCI_PORT_INT_TFES      (1 << 30) // Task File Error Status
#define AHCI_PORT_INT_HBFS      (1 << 29) // Host Bus Fatal Error Status
#define AHCI_PORT_INT_HBDS      (1 << 28) // Host Bus Data Error Status
#define AHCI_PORT_INT_IFS       (1 << 27) // Interface Fatal Error Status
#define AHCI_PORT_INT_INFS      (1 << 26) // Interface Non-fatal Error Status
#define AHCI_PORT_INT_OFS       (1 << 24) // Overflow Status
#define AHCI_PORT_INT_IPMS      (1 << 23) // Incorrect Port Multiplier Status
#define AHCI_PORT_INT_PRCS      (1 << 22) // PhyRdy Change Status
#define AHCI_PORT_INT_DMPS      (1 << 7)  // Device Mechanical Presence Status
#define AHCI_PORT_INT_PCS       (1 << 6)  // Port Connect Change Status
#define AHCI_PORT_INT_DPS       (1 << 5)  // Descriptor Processed 
#define AHCI_PORT_INT_UFS       (1 << 4)  // Unknown FIS Interrupt
#define AHCI_PORT_INT_SDBS      (1 << 3)  // Set Device Bits Interrupt
#define AHCI_PORT_INT_DSS       (1 << 2)  // DMA Setup FIS Interrupt
#define AHCI_PORT_INT_PSS       (1 << 1)  // PIO Setup FIS Interrupt
#define AHCI_PORT_INT_DHRS      (1 << 0)  // Device to Host Register FIS Interrupt

// Serial ATA Status (SSTS) bits
#define AHCI_PORT_SSTS_IPM_MASK (0xF << 8) // Interface Power Management Mask
#define AHCI_PORT_SSTS_IPM_SHIFT 8 // Interface Power Management Shift
#define AHCI_PORT_SSTS_IPM_NONE        0x0 // No device present or in PARTIAL state
#define AHCI_PORT_SSTS_IPM_ACTIVE      0x1 // Interface in active state
#define AHCI_PORT_SSTS_IPM_PARTIAL     0x2 // Interface in PARTIAL power management state
#define AHCI_PORT_SSTS_IPM_SLUMBER     0x6 // Interface in SLUMBER power management state
#define AHCI_PORT_SSTS_IPM_DEVSLEEP    0x8 // Interface in DevSleep power management state

#define AHCI_PORT_SSTS_SPD_MASK (0xF << 4) // Speed Mask
#define AHCI_PORT_SSTS_SPD_SHIFT 4 // Speed Shift
#define AHCI_PORT_SSTS_SPD_GEN1 1 // Gen 1 (1.5 Gbps)
#define AHCI_PORT_SSTS_SPD_GEN2 2 // Gen 2 (3 Gbps)
#define AHCI_PORT_SSTS_SPD_GEN3 3 // Gen 3 (6 Gbps)

#define AHCI_PORT_SSTS_DET_MASK (0xF << 0) // Device Detection Mask
#define AHCI_PORT_SSTS_DET_SHIFT 0 // Device Detection Shift
#define AHCI_PORT_SSTS_DET_NONE        0x0 // No device detected, no phy communication
#define AHCI_PORT_SSTS_DET_PRESENT     0x1 // Device detected but no communication
#define AHCI_PORT_SSTS_DET_OFFLINE     0x2 // Device detected but phy offline
#define AHCI_PORT_SSTS_DET_ESTABLISHED 0x3 // Device detected and phy communication established

// FIS Types
#define FIS_TYPE_REG_H2D    0x27 // Register FIS - Host to Device
#define FIS_TYPE_REG_D2H    0x34 // Register FIS - Device to Host
#define FIS_TYPE_DMA_ACT    0x39 // DMA Activate FIS
#define FIS_TYPE_DMA_SETUP  0x41 // DMA Setup FIS
#define FIS_TYPE_DATA       0x46 // Data FIS
#define FIS_TYPE_BIST       0x58 // BIST Activate FIS
#define FIS_TYPE_PIO_SETUP  0x5F // PIO Setup FIS
#define FIS_TYPE_DEV_BITS   0xA1 // Set Device Bits FIS

// Command List Structure (one per port)
typedef struct {
    uint32_t dw0;  // Command Header DW0
    uint32_t prdbc; // Physical Region Descriptor Byte Count
    uint32_t ctba;  // Command Table Base Address (low 32 bits)
    uint32_t ctbau; // Command Table Base Address (upper 32 bits)
    uint32_t reserved[4]; // Reserved
} ahci_cmd_header_t;

// Command DW0 bits
#define AHCI_CMD_HDR_CFL_MASK   (0x1F << 0) // Command FIS Length in DWORDs, 2 ~ 16
#define AHCI_CMD_HDR_A          (1 << 5)    // ATAPI
#define AHCI_CMD_HDR_W          (1 << 6)    // Write (1: H2D, 0: D2H)
#define AHCI_CMD_HDR_P          (1 << 7)    // Prefetchable
#define AHCI_CMD_HDR_R          (1 << 8)    // Reset
#define AHCI_CMD_HDR_B          (1 << 9)    // BIST
#define AHCI_CMD_HDR_C          (1 << 10)   // Clear Busy upon R_OK
#define AHCI_CMD_HDR_PMP_MASK   (0xF << 12) // Port Multiplier Port
#define AHCI_CMD_HDR_PRDTL_MASK (0xFFFF << 16) // Physical Region Descriptor Table Length

// Physical Region Descriptor (PRD) Entry
typedef struct {
    uint32_t dba;    // Data Base Address (low 32 bits)
    uint32_t dbau;   // Data Base Address (upper 32 bits)
    uint32_t reserved;
    uint32_t dbc;    // Data Byte Count (D=0 means 4MB)
} ahci_prd_entry_t;

#define AHCI_PRD_DBC_MASK      0x00FFFFFF // Byte Count Mask
#define AHCI_PRD_DBC_INTERRUPT 0x80000000 // Interrupt on Completion

// Register H2D FIS Structure (Command FIS)
typedef struct {
    uint8_t fis_type;     // FIS_TYPE_REG_H2D
    uint8_t pmport:4;     // Port multiplier port
    uint8_t reserved0:3;  // Reserved
    uint8_t c:1;          // Command bit
    uint8_t command;      // Command register
    uint8_t featurel;     // Feature register, 7:0
    uint8_t lba0;         // LBA low register, 7:0
    uint8_t lba1;         // LBA mid register, 15:8
    uint8_t lba2;         // LBA high register, 23:16
    uint8_t device;       // Device register
    uint8_t lba3;         // LBA register, 31:24
    uint8_t lba4;         // LBA register, 39:32
    uint8_t lba5;         // LBA register, 47:40
    uint8_t featureh;     // Feature register, 15:8
    uint8_t countl;       // Count register, 7:0
    uint8_t counth;       // Count register, 15:8
    uint8_t icc;          // Isochronous command completion
    uint8_t control;      // Control register
    uint8_t reserved1[4]; // Reserved
} __attribute__((packed)) fis_reg_h2d_t;

// Register D2H FIS Structure
typedef struct {
    uint8_t fis_type;     // FIS_TYPE_REG_D2H
    uint8_t pmport:4;     // Port multiplier port
    uint8_t reserved0:2;  // Reserved
    uint8_t i:1;          // Interrupt bit
    uint8_t reserved1:1;  // Reserved
    uint8_t status;       // Status register
    uint8_t error;        // Error register
    uint8_t lba0;         // LBA low register, 7:0
    uint8_t lba1;         // LBA mid register, 15:8
    uint8_t lba2;         // LBA high register, 23:16
    uint8_t device;       // Device register
    uint8_t lba3;         // LBA register, 31:24
    uint8_t lba4;         // LBA register, 39:32
    uint8_t lba5;         // LBA register, 47:40
    uint8_t reserved2;    // Reserved
    uint8_t countl;       // Count register, 7:0
    uint8_t counth;       // Count register, 15:8
    uint8_t reserved3[6]; // Reserved
} __attribute__((packed)) fis_reg_d2h_t;

// Data FIS Structure
typedef struct {
    uint8_t fis_type;     // FIS_TYPE_DATA
    uint8_t pmport:4;     // Port multiplier port
    uint8_t reserved0:4;  // Reserved
    uint8_t reserved1[2]; // Reserved
    uint32_t data[1];     // Payload (can be of any size)
} __attribute__((packed)) fis_data_t;

// PIO Setup FIS Structure
typedef struct {
    uint8_t fis_type;     // FIS_TYPE_PIO_SETUP
    uint8_t pmport:4;     // Port multiplier port
    uint8_t reserved0:1;  // Reserved
    uint8_t d:1;          // Data transfer direction (1 = D2H)
    uint8_t i:1;          // Interrupt bit
    uint8_t reserved1:1;  // Reserved
    uint8_t status;       // Status register
    uint8_t error;        // Error register
    uint8_t lba0;         // LBA low register, 7:0
    uint8_t lba1;         // LBA mid register, 15:8
    uint8_t lba2;         // LBA high register, 23:16
    uint8_t device;       // Device register
    uint8_t lba3;         // LBA register, 31:24
    uint8_t lba4;         // LBA register, 39:32
    uint8_t lba5;         // LBA register, 47:40
    uint8_t reserved2;    // Reserved
    uint8_t countl;       // Count register, 7:0
    uint8_t counth;       // Count register, 15:8
    uint8_t reserved3;    // Reserved
    uint8_t e_status;     // New value of status register
    uint16_t tc;          // Transfer count
    uint8_t reserved4[2]; // Reserved
} __attribute__((packed)) fis_pio_setup_t;

// DMA Setup FIS Structure
typedef struct {
    uint8_t fis_type;     // FIS_TYPE_DMA_SETUP
    uint8_t pmport:4;     // Port multiplier port
    uint8_t reserved0:1;  // Reserved
    uint8_t d:1;          // Data transfer direction (1 = D2H)
    uint8_t i:1;          // Interrupt bit
    uint8_t a:1;          // Auto-activate
    uint64_t dma_buffer_id; // DMA Buffer Identifier
    uint32_t reserved1;   // Reserved
    uint32_t dma_buf_offset; // DMA buffer offset
    uint32_t transfer_count; // DMA transfer count
    uint32_t reserved2;   // Reserved
} __attribute__((packed)) fis_dma_setup_t;

// Set Device Bits FIS Structure
typedef struct {
    uint8_t fis_type;     // FIS_TYPE_DEV_BITS
    uint8_t pmport:4;     // Port multiplier port
    uint8_t reserved0:2;  // Reserved
    uint8_t i:1;          // Interrupt bit
    uint8_t n:1;          // Notification bit
    uint8_t statusl:3;    // Status Low register
    uint8_t reserved1:1;  // Reserved
    uint8_t statush:3;    // Status High register
    uint8_t reserved2:1;  // Reserved
    uint8_t error;        // Error register
    uint32_t protocol;    // Protocol specific
} __attribute__((packed)) fis_dev_bits_t;

// Received FIS Structure (one per port)
typedef volatile struct {
    fis_dma_setup_t  dsfis;     // DMA Setup FIS
    uint8_t          pad1[4];   // Padding
    fis_pio_setup_t  psfis;     // PIO Setup FIS
    uint8_t          pad2[12];  // Padding
    fis_reg_d2h_t    rfis;      // D2H Register FIS
    uint8_t          pad3[4];   // Padding
    fis_dev_bits_t   sdbfis;    // Set Device Bits FIS
    uint8_t          ufis[64];  // Unknown FIS
    uint8_t          reserved[0x100-0xA0]; // Reserved
} __attribute__((packed)) ahci_received_fis_t;

// Command Table Structure
typedef struct {
    uint8_t cfis[64];     // Command FIS
    uint8_t acmd[16];     // ATAPI command
    uint8_t reserved[48]; // Reserved
    ahci_prd_entry_t prdt[1]; // Physical Region Descriptor Table (variable size)
} __attribute__((packed)) ahci_cmd_table_t;

// HBA Port Structure (HBA Port Registers)
typedef volatile struct {
    uint32_t clb;     // Command List Base Address
    uint32_t clbu;    // Command List Base Address Upper 32-bits
    uint32_t fb;      // FIS Base Address
    uint32_t fbu;     // FIS Base Address Upper 32-bits
    uint32_t is;      // Interrupt Status
    uint32_t ie;      // Interrupt Enable
    uint32_t cmd;     // Command and Status
    uint32_t reserved0; // Reserved
    uint32_t tfd;     // Task File Data
    uint32_t sig;     // Signature
    uint32_t ssts;    // Serial ATA Status
    uint32_t sctl;    // Serial ATA Control
    uint32_t serr;    // Serial ATA Error
    uint32_t sact;    // Serial ATA Active
    uint32_t ci;      // Command Issue
    uint32_t sntf;    // Serial ATA Notification
    uint32_t fbs;     // FIS-based Switching Control
    uint32_t devslp;  // Device Sleep
    uint8_t  reserved1[40]; // Reserved
    uint8_t  vendor[4]; // Vendor Specific
} __attribute__((packed)) ahci_hba_port_t;

// Define the HBA memory structure
typedef struct {
    // Generic Host Control registers
    uint32_t cap;     // Host Capabilities
    uint32_t ghc;     // Global Host Control
    uint32_t is;      // Interrupt Status
    uint32_t pi;      // Ports Implemented
    uint32_t vs;      // Version
    uint32_t ccc_ctl; // Command Completion Coalescing Control
    uint32_t ccc_ports; // Command Completion Coalescing Ports
    uint32_t em_loc;  // Enclosure Management Location
    uint32_t em_ctl;  // Enclosure Management Control
    uint32_t cap2;    // Host Capabilities Extended
    uint32_t bohc;    // BIOS/OS Handoff Control and Status
    uint8_t  reserved[0x74-0x2C]; // Reserved
    uint8_t  vendor[0x100-0x74];  // Vendor Specific registers
    
    // Port registers
    ahci_hba_port_t ports[32]; // Port control registers (up to 32 ports)
} __attribute__((packed)) ahci_hba_memory_t;

// AHCI Device Structure
typedef struct {
    uint8_t port_num;              // Port number
    uint8_t device_type;           // Device type (SATA, SATAPI, etc.)
    uint8_t is_present;            // Is device present
    uint64_t sector_count;         // Total number of sectors
    uint32_t sector_size;          // Sector size in bytes
    char model[41];                // Model string (null-terminated)
    char serial[21];               // Serial number string (null-terminated)
    char firmware_rev[9];          // Firmware revision string (null-terminated)
} ahci_device_t;

// Device types
#define AHCI_DEV_NULL     0x00
#define AHCI_DEV_SATA     0x01
#define AHCI_DEV_SATAPI   0x02
#define AHCI_DEV_SEMB     0x03
#define AHCI_DEV_PM       0x04

// Function prototypes
int ahci_init(uint16_t vendor_id, uint16_t device_id, uint16_t bus = 0, uint16_t device = 0, uint16_t function = 0);
int ahci_init_impl(uint16_t vendor_id, uint16_t device_id, uint16_t bus = 0, uint16_t device = 0, uint16_t function = 0);
void ahci_port_rebase(int port_num);
int ahci_port_init(int port_num);
int ahci_identify_device(int port_num);
int ahci_read_sectors(int port_num, uint64_t start, uint32_t count, void* buffer);
int ahci_write_sectors(int port_num, uint64_t start, uint32_t count, const void* buffer);
ahci_device_t* ahci_get_device_info(int port_num);
int ahci_check_port_type(volatile ahci_hba_memory_t* hba, int port_num);
int ahci_port_start_cmd(volatile ahci_hba_memory_t* hba, int port_num);
int ahci_port_stop_cmd(volatile ahci_hba_memory_t* hba, int port_num);
int ahci_find_command_slot(volatile ahci_hba_memory_t* hba, int port_num);
int ahci_detect_filesystem(int port_num);
