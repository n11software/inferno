#include <fs/ext2/ext2.h>
#include <Drivers/Storage/AHCI/AHCI.h>
#include <Inferno/Log.h>
#include <Inferno/string.h> // Added explicit include for string functions
// Use the standard memory functions from stdlib
#include <stdint.h>
#include <Inferno/Log.h>
#include <Memory/Mem_.hpp>
#include <stdarg.h> // For va_list

#define DEBUG false

// Needed string functions and memory functions (imported from C library or kernel functions)
extern "C" {
    void* malloc(size_t size);
    void free(void* ptr);
    
    // Simple sprintf implementation
    int sprintf(char* str, const char* format, ...) {
        va_list args;
        va_start(args, format);
        
        char* ptr = str;
        
        while (*format != '\0') {
            if (*format == '%') {
                format++;
                
                // Handle format specifiers
                switch (*format) {
                    case 's': {
                        char* s = va_arg(args, char*);
                        if (s == nullptr) {
                            // Handle null string pointers safely
                            const char* null_str = "(null)";
                            while (*null_str != '\0') {
                                *ptr++ = *null_str++;
                            }
                        } else {
                            // Copy the string
                            while (*s != '\0') {
                                *ptr++ = *s++;
                            }
                        }
                        break;
                    }
                    case 'c': {
                        // Handle character format specifier properly
                        char c = (char)va_arg(args, int); // char is promoted to int in varargs
                        *ptr++ = c;
                        break;
                    }
                    case 'u': {
                        unsigned int d = va_arg(args, unsigned int);
                        
                        // Convert unsigned int to string
                        char num_buf[16];
                        char* num_ptr = num_buf + 15;
                        *num_ptr = '\0';
                        
                        do {
                            *--num_ptr = '0' + (d % 10);
                            d /= 10;
                        } while (d > 0);
                        
                        // Copy to output
                        while (*num_ptr != '\0') {
                            *ptr++ = *num_ptr++;
                        }
                        break;
                    }
                    case 'x': {
                        unsigned int d = va_arg(args, unsigned int);
                        
                        // Convert to hex
                        char num_buf[16];
                        char* num_ptr = num_buf + 15;
                        *num_ptr = '\0';
                        
                        const char* hex_chars = "0123456789abcdef";
                        do {
                            *--num_ptr = hex_chars[d & 0xF];
                            d >>= 4;
                        } while (d > 0);
                        
                        // Copy to output
                        while (*num_ptr != '\0') {
                            *ptr++ = *num_ptr++;
                        }
                        break;
                    }
                    case '%':
                        *ptr++ = '%';
                        break;
                    default:
                        // Handle unknown format specifiers
                        if (*format == '0' && *(format+1) == '2' && *(format+2) == 'x') {
                            // Handle %02x format (for bytes in hex)
                            format += 2; // Skip '0' and '2'
                            unsigned int d = va_arg(args, unsigned int);
                            const char* hex_chars = "0123456789abcdef";
                            // Always print 2 hex digits
                            *ptr++ = hex_chars[(d >> 4) & 0xF];
                            *ptr++ = hex_chars[d & 0xF];
                        } else {
                            // Unknown format, just copy as-is
                            *ptr++ = '%';
                            *ptr++ = *format;
                        }
                }
            } else {
                *ptr++ = *format;
            }
            
            format++;
        }
        
        *ptr = '\0';
        va_end(args);
        return ptr - str;
    }
}

namespace FS {
namespace EXT2 {

// Global variables to hold filesystem metadata
static ext2_superblock_t superblock;
static ext2_group_desc_t* group_descriptors = nullptr;
static uint32_t block_size = 1024;
static uint32_t inodes_per_block = 0;
static uint32_t blocks_per_group = 0;
static uint32_t inodes_per_group = 0;
static uint32_t sectors_per_block = 0;
static int active_port = -1;
static uint32_t sector_size = 512; // Default sector size

// Helper functions
static uint32_t GetBlockSize() {
    return block_size;
}

static uint32_t BlockToSector(uint32_t block) {
    return block * sectors_per_block;
}

// Returns the group descriptor for the specified inode
static ext2_group_desc_t* GetGroupDescriptor(uint32_t inode) {
    uint32_t group = (inode - 1) / inodes_per_group;
    return &group_descriptors[group];
}

// Reads a block from disk
static void* ReadBlock(int port_num, uint32_t block_num) {
    void* buffer = malloc(block_size);
    if (!buffer) {
        prErr("ext2", "Failed to allocate memory for block %u", block_num);
        return nullptr;
    }

    // Initialize buffer to zeros
    memset(buffer, 0, block_size);

    uint32_t sector = BlockToSector(block_num);
    if (DEBUG) {
        prInfo("ext2", "Reading block %u (sector %u, count %u)", 
               block_num, sector, sectors_per_block);
    }
    
    // Get device info for better error reporting
    ahci_device_t* device = ahci_get_device_info(port_num);
    if (!device) {
        prErr("ext2", "Failed to get device info for port %d", port_num);
        free(buffer);
        return nullptr;
    }
    
    // Additional validation
    if (device->sector_count > 0 && sector >= device->sector_count) {
        prErr("ext2", "Block %u (sector %u) exceeds device sector count %u",
              block_num, sector, (unsigned int)device->sector_count);
        // Continue anyway for testing purposes, but log the warning
        prInfo("ext2", "Continuing with read operation despite range error (for debugging)");
    }
    
    int result = ahci_read_sectors(port_num, sector, sectors_per_block, buffer);
    if (result != 0) {
        prErr("ext2", "Failed to read block %u (sector %u, error code %d)", 
              block_num, sector, result);
              
        // Log additional information for debugging
        prInfo("ext2", "Device info: port=%d, sectors=%u, sector_size=%u", 
               port_num, (unsigned int)device->sector_count, (unsigned int)device->sector_size);
        
        // For debugging, let's examine what's in the buffer, it might contain partial data
        uint8_t* bytes = (uint8_t*)buffer;
        prInfo("ext2", "Buffer contains (first 16 bytes): %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", 
               bytes[0], bytes[1], bytes[2], bytes[3], 
               bytes[4], bytes[5], bytes[6], bytes[7],
               bytes[8], bytes[9], bytes[10], bytes[11],
               bytes[12], bytes[13], bytes[14], bytes[15]);
               
        free(buffer);
        return nullptr;
    }

    return buffer;
}

// Reads the superblock from the disk
bool ReadSuperblock(int port_num, uint32_t sector_offset) {
    // prInfo("ext2", "Reading superblock from port %d at sector %u", port_num, sector_offset);
    
    // Get device info
    ahci_device_t* device = ahci_get_device_info(port_num);
    if (!device) {
        prErr("ext2", "Failed to get device info for port %d", port_num);
        return false;
    }
    
    // Use a default sector size if the device info doesn't provide one
    if (device->sector_size == 0) {
        prInfo("ext2", "Invalid sector size, using default (512 bytes)");
        sector_size = 512;
    } else {
        sector_size = device->sector_size;
        // prInfo("ext2", "Using sector size: %u bytes", (unsigned int)sector_size);
    }
    
    // Calculate sectors needed to read 1024 bytes (superblock size)
    uint32_t sectors_to_read = (1024 + sector_size - 1) / sector_size;
    // prInfo("ext2", "Reading %u sectors to get superblock", sectors_to_read);
    
    // Read the sectors containing the superblock
    void* buffer = malloc(sectors_to_read * sector_size);
    if (!buffer) {
        prErr("ext2", "Failed to allocate memory for superblock");
        return false;
    }

    // Initialize buffer to all zeros to help with debugging
    memset(buffer, 0, sectors_to_read * sector_size);

    // prInfo("ext2", "Reading superblock at sector %u, count: %u", 
    //        sector_offset, sectors_to_read);
    
    int result = ahci_read_sectors(port_num, sector_offset, sectors_to_read, buffer);
    if (result != 0) {
        prErr("ext2", "Failed to read superblock sectors, error: %d", result);
        
        // Log the first few bytes of what we got (useful for debugging)
        uint8_t* bytes = (uint8_t*)buffer;
        prInfo("ext2", "Buffer contains: %02x %02x %02x %02x %02x %02x %02x %02x", 
               bytes[0], bytes[1], bytes[2], bytes[3], 
               bytes[4], bytes[5], bytes[6], bytes[7]);
        
        free(buffer);
        return false;
    }
    
    // Debug output - display first several bytes of the superblock
    // prInfo("ext2", "Superblock first 32 bytes:");
    uint8_t* bytes = (uint8_t*)buffer;
    // for (int i = 0; i < 32; i += 8) {
    //     prInfo("ext2", "  %02x %02x %02x %02x %02x %02x %02x %02x", 
    //            bytes[i], bytes[i+1], bytes[i+2], bytes[i+3], 
    //            bytes[i+4], bytes[i+5], bytes[i+6], bytes[i+7]);
    // }
    
    // Try to find EXT2 magic number in the buffer
    bool found_magic = false;
    uint16_t* magic_ptr = nullptr;
    
    // The magic number is at offset 56 (0x38) in the superblock
    if (sectors_to_read * sector_size >= 58) {  // Make sure we have at least 58 bytes
        magic_ptr = (uint16_t*)(bytes + 56);
        if (*magic_ptr == EXT2_SUPER_MAGIC) {
            found_magic = true;
            // prInfo("ext2", "Found magic number at offset 56: 0x%04x", *magic_ptr);
        }
    }
    
    if (!found_magic) {
        // Search through the buffer for the magic number
        for (uint32_t i = 0; i < (sectors_to_read * sector_size) - 1; i += 2) {
            uint16_t value = *(uint16_t*)(bytes + i);
            if (value == EXT2_SUPER_MAGIC) {
                found_magic = true;
                magic_ptr = (uint16_t*)(bytes + i);
                prInfo("ext2", "Found magic number at offset %u: 0x%04x", i, value);
                break;
            }
        }
    }
    
    if (!found_magic) {
        prErr("ext2", "EXT2 magic number not found in buffer");
        free(buffer);
        return false;
    }
    
    // Calculate the offset of the superblock in the buffer
    uint32_t superblock_offset = (uint8_t*)magic_ptr - bytes - 56;
    
    // Copy the superblock data from the correct offset
    memcpy(&superblock, bytes + superblock_offset, sizeof(ext2_superblock_t));
    
    // Verify the ext2 magic number
    if (superblock.magic != EXT2_SUPER_MAGIC) {
        prErr("ext2", "Invalid superblock magic number: 0x%04x (expected 0x%04x)", 
              superblock.magic, EXT2_SUPER_MAGIC);
        free(buffer);
        return false;
    }
    
    // Calculate filesystem parameters
    block_size = 1024 << superblock.log_block_size;
    sectors_per_block = block_size / sector_size;
    inodes_per_block = block_size / sizeof(ext2_inode_t);
    
    // Set block groups parameters
    blocks_per_group = superblock.blocks_per_group;
    inodes_per_group = superblock.inodes_per_group;
    
    // prInfo("ext2", "EXT2 filesystem detected (magic: 0x%x)", superblock.magic);
    // prInfo("ext2", "Block size: %u bytes, %u sectors per block", block_size, sectors_per_block);
    // prInfo("ext2", "Total blocks: %u, blocks per group: %u", 
        //    superblock.blocks_count, blocks_per_group);
    // prInfo("ext2", "Total inodes: %u, inodes per group: %u", 
        //    superblock.inodes_count, inodes_per_group);
    
    free(buffer);
    return true;
}

// Reads the block group descriptors
bool ReadBlockGroupDescriptors(int port_num) {
    uint32_t num_groups = (superblock.blocks_count + blocks_per_group - 1) / blocks_per_group;
    uint32_t desc_table_size = num_groups * sizeof(ext2_group_desc_t);
    uint32_t desc_per_block = block_size / sizeof(ext2_group_desc_t);
    uint32_t desc_table_blocks = (num_groups + desc_per_block - 1) / desc_per_block;

    // prInfo("ext2", "Block groups: %u", num_groups);

    // Allocate memory for group descriptors
    group_descriptors = (ext2_group_desc_t*)malloc(desc_table_size);
    if (!group_descriptors) {
        prErr("ext2", "Failed to allocate memory for group descriptors");
        return false;
    }

    // Block group descriptor table starts right after the superblock
    uint32_t start_block = superblock.first_data_block + 1;
    
    for (uint32_t i = 0; i < desc_table_blocks; i++) {
        void* block_buffer = ReadBlock(port_num, start_block + i);
        if (!block_buffer) {
            prErr("ext2", "Failed to read block group descriptor table");
            free(group_descriptors);
            group_descriptors = nullptr;
            return false;
        }

        uint32_t offset = i * desc_per_block;
        uint32_t count = ((num_groups - offset) > desc_per_block) ? desc_per_block : (num_groups - offset);
        memcpy((char*)group_descriptors + offset * sizeof(ext2_group_desc_t), 
               block_buffer, count * sizeof(ext2_group_desc_t));
        
        free(block_buffer);
    }

    // prInfo("ext2", "Block group descriptors read successfully");
    return true;
}

// Reads an inode from disk
static ext2_inode_t* ReadInode(int port_num, uint32_t inode_num) {
    if (inode_num < 1) {
        prErr("ext2", "Invalid inode number: %u", inode_num);
        return nullptr;
    }
    
    // Calculate group and index
    uint32_t group = (inode_num - 1) / inodes_per_group;
    uint32_t index = (inode_num - 1) % inodes_per_group;
    uint32_t inode_table_block = group_descriptors[group].inode_table;
    
    // Calculate block offset and inode offset within block
    // Adjust for the actual inode size if present in the superblock
    uint32_t inode_size = superblock.inode_size > 0 ? superblock.inode_size : sizeof(ext2_inode_t);
    uint32_t block_offset = (index * inode_size) / block_size;
    uint32_t inode_offset = (index * inode_size) % block_size;

    // Read the block containing the inode
    void* block_data = ReadBlock(port_num, inode_table_block + block_offset);
    if (!block_data) {
        prErr("ext2", "Failed to read inode table block for inode %u", inode_num);
        return nullptr;
    }
    
    // Allocate memory for the inode
    ext2_inode_t* inode = (ext2_inode_t*)malloc(sizeof(ext2_inode_t));
    if (!inode) {
        prErr("ext2", "Failed to allocate memory for inode %u", inode_num);
        free(block_data);
        return nullptr;
    }
    
    // Copy the inode data
    memcpy(inode, (uint8_t*)block_data + inode_offset, sizeof(ext2_inode_t));
    
    free(block_data);
    return inode;
}

// Reads data from an inode's block pointer
static void* ReadInodeData(int port_num, ext2_inode_t* inode, uint32_t block_index) {
    // Debug information about the inode
    // prInfo("ext2", "Inode details: size=%u, blocks=%u, sector size=%u, sectors per block=%u",
    //        inode->size, inode->blocks, sector_size, sectors_per_block);
    
    // EXT2 reports blocks in 512-byte units regardless of the actual block size
    // So we need to convert the inode->blocks count to our block_size units
    // The conversion factor is block_size / 512
    uint32_t blocks_count_in_fs_blocks = inode->blocks / (block_size / 512);
    
    if (inode->blocks == 0) {
        // Special case: Empty file or directory with no allocated blocks
        prInfo("ext2", "Inode has 0 blocks, treating as empty");
        return nullptr;
    }
    
    // For debugging, let's dump the first few block pointers
    // prInfo("ext2", "Direct block pointers:");
    // for (int i = 0; i < 12 && i < inode->blocks; i++) {
    //     prInfo("ext2", "  block[%d] = %u", i, inode->block[i]);
    // }
    
    // For safety, ignore the blocks_count check and just try to read the direct block
    // This is needed because some EXT2 implementations don't set the blocks count correctly
    if (block_index < 12) {
        uint32_t block_num = inode->block[block_index];
        if (block_num == 0) {
            prInfo("ext2", "Block pointer %u is null (0)", block_index);
            return nullptr;
        }
        
        // prInfo("ext2", "Reading block pointer %u -> block number %u", block_index, block_num);
        return ReadBlock(port_num, block_num);
    }

    // Indirect blocks
    prErr("ext2", "Indirect blocks not implemented yet");
    return nullptr; // Not implemented yet
}

// Lists files in a directory
bool ListDirectory(int port_num, uint32_t inode_num) {
    ext2_inode_t* inode = ReadInode(port_num, inode_num);
    if (!inode) {
        return false;
    }

    // Check if it's a directory
    if (!(inode->mode & EXT2_S_IFDIR)) {
        prErr("ext2", "Inode %u is not a directory", inode_num);
        free(inode);
        return false;
    }

    // Only show debug info if DEBUG is enabled
    if (DEBUG) {
        prInfo("ext2", "Directory listing for inode %u:", inode_num);
    }

    // Simple header for directory listing
    if (DEBUG) {
        kprintf(" Directory listing:\n -----------------\n");
    }

    // Read directory entries from direct blocks
    uint32_t remaining_size = inode->size;
    uint32_t block_index = 0;
    
    // Count for items
    int total_items = 0;
    int total_dirs = 0;
    
    // Buffer to collect all filenames for simplified output
    char all_filenames[4096] = {0};
    char* filename_ptr = all_filenames;
    
    while (remaining_size > 0 && block_index < 12) {
        void* block_data = ReadInodeData(port_num, inode, block_index);
        if (!block_data) {
            free(inode);
            return false;
        }

        uint32_t offset = 0;
        while (offset < block_size && offset < remaining_size) {
            ext2_dir_entry_t* entry = (ext2_dir_entry_t*)((char*)block_data + offset);
            if (entry->inode != 0) {
                // Safely copy the name with bounds checking
                char name_buf[256] = {0};
                uint8_t safe_len = (entry->name_len < 255) ? entry->name_len : 255;
                if (safe_len > 0) {
                    // Copy the name data
                    for (int i = 0; i < safe_len; i++) {
                        name_buf[i] = entry->name[i];
                    }
                    name_buf[safe_len] = '\0';
                    
                    // Only show detailed debug info if DEBUG is enabled
                    if (DEBUG) {
                        // Raw entry data
                        // prInfo("ext2", "  Raw entry data: inode=%u, rec_len=%u, name_len=%u, file_type=%u",
                            //   entry->inode, entry->rec_len, entry->name_len, entry->file_type);
                        
                        // Filename and type info
                        // prInfo("ext2", "    Name: '%s' [%s] (inode: %u)", 
                            //    name_buf, GetFileType(entry->file_type), entry->inode);
                               
                        // Debug tag for the ls output
                        // prInfo("ext2", "ls:  ");
                    }

                    // Skip . and .. entries from the simplified output
                    if (entry->name_len > 0 && name_buf[0] != '.') {
                        // Track stats
                        total_items++;
                        if (entry->file_type == EXT2_FT_DIR) {
                            total_dirs++;
                        }
                        
                        // Add to our concatenated output
                        // Copy the name to our buffer
						// add blue \033[32m if it's a directory before the name
						if (entry->file_type == EXT2_FT_DIR) {
							*filename_ptr++ = '\033';
							*filename_ptr++ = '[';
							*filename_ptr++ = '3';
							*filename_ptr++ = '4';
							*filename_ptr++ = 'm';
						}
                        for (int i = 0; i < safe_len; i++) {
                            *filename_ptr++ = name_buf[i];
                        }
                        
                        // Add directory indicator and space
                        if (entry->file_type == EXT2_FT_DIR) {
                            *filename_ptr++ = '/';
							*filename_ptr++ = '\033';
							*filename_ptr++ = '[';
							*filename_ptr++ = '0';
							*filename_ptr++ = 'm';
                        }
                        *filename_ptr++ = ' ';
                    }
                } else {
                    // Handle empty filenames
                    sprintf(name_buf, "[empty]");
                    
                    if (DEBUG) {
                        prInfo("ext2", "    Name: '%s' [%s] (inode: %u)", 
                               name_buf, GetFileType(entry->file_type), entry->inode);
                    }
                }
            }
            
            // Move to the next entry with a sanity check
            if (entry->rec_len == 0) {
                prErr("ext2", "Invalid directory entry: rec_len is 0");
                break;
            }
            offset += entry->rec_len;
        }
        
        free(block_data);
        block_index++;
        remaining_size = (remaining_size > block_size) ? (remaining_size - block_size) : 0;
    }
    
    // Null-terminate our buffer
    *filename_ptr = '\0';
    
    // Print the simple output format
    kprintf("%s\n", all_filenames);
    
    if (DEBUG) {
        // Print summary
        kprintf(" Total: %d items, %d directories\n", total_items, total_dirs);
    }

    free(inode);
    return true;
}

// Initialize the EXT2 driver
bool Initialize(int port_num) {
    // Check if the given port is valid
    // prInfo("ext2", "Initializing EXT2 driver for port %d", port_num);
    
    // Get device information using the correct AHCI function
    ahci_device_t* device = ahci_get_device_info(port_num);
    if (device == nullptr) {
        prErr("ext2", "Failed to get device info for port %d", port_num);
        return false;
    }
    
    uint32_t device_sectors = device->sector_count;
    uint32_t device_sector_size = device->sector_size;
    
    if (device_sectors == 0 || device_sector_size == 0) {
        prWarn("ext2", "Device reports zero sectors or sector size, using defaults");
        if (device_sectors == 0) device_sectors = 2048;  // Use a reasonable default
        if (device_sector_size == 0) device_sector_size = 512;
    }
    
    // prInfo("ext2", "Device has %u sectors of %u bytes each", device_sectors, device_sector_size);
    
    // Store the sector size
    sector_size = device_sector_size;
    active_port = port_num;
    
    // Try standard initialization from sector 2
    // prInfo("ext2", "Reading superblock at sector %u, count: 2", 2);
    bool result = ReadSuperblock(port_num, 2);

    // If standard initialization fails, try other offsets
    if (!result) {
        // prInfo("ext2", "Standard superblock initialization failed, trying alternate offsets");
        
        // An array of potential superblock offsets to try
        uint32_t alt_offsets[] = {
            2,     // Standard (1024 bytes from start)
            8192,  // 4 MB into disk
            16384, // 8 MB into disk
            32768, // 16 MB into disk
            65536, // 32 MB into disk
            0,     // As a last resort, try sector 0 (some corrupt FS have SB here)
            1      // Sector 1 in some variants
        };
        
        for (size_t i = 0; i < sizeof(alt_offsets) / sizeof(alt_offsets[0]); i++) {
            prInfo("ext2", "Trying alternate superblock at sector %u", alt_offsets[i]);
            result = ReadSuperblock(port_num, alt_offsets[i]);
            if (result) {
                prInfo("ext2", "Found valid superblock at sector %u", alt_offsets[i]);
                break;
            }
        }
    }
    
    // If we still don't have a valid superblock, fail
    if (!result) {
        prErr("ext2", "Failed to find a valid superblock, aborting initialization");
        return false;
    }
    
    // Read block group descriptors
    result = ReadBlockGroupDescriptors(port_num);
    if (!result) {
        prErr("ext2", "Failed to read block group descriptors");
        return false;
    }
    
    // Try to read the root inode to validate our configuration
    ext2_inode_t* root_inode = ReadInode(port_num, EXT2_ROOT_INO);
    if (!root_inode) {
        prErr("ext2", "Failed to read root inode");
        return false;
    }
    
    // Dump root inode details
    // prInfo("ext2", "Root inode details:");
    // prInfo("ext2", "  Mode: 0x%x (Is directory: %s)", 
        //    root_inode->mode, 
        //    (root_inode->mode & EXT2_S_IFDIR) ? "Yes" : "No");
    // prInfo("ext2", "  Size: %u bytes", root_inode->size);
    // prInfo("ext2", "  Blocks: %u (in 512-byte units)", root_inode->blocks);
    // prInfo("ext2", "  Block pointers:");
    // for (int i = 0; i < 12 && root_inode->block[i] != 0; i++) {
    //     // prInfo("ext2", "    block[%d] = %u", i, root_inode->block[i]);
    // }
    
    // Free root inode
    free(root_inode);
    
    // List the root directory to test the driver
    result = ListDirectory(port_num, EXT2_ROOT_INO);
    if (!result) {
        prErr("ext2", "Failed to list root directory");
    }
    
    return result;
}

// Read root directory
bool ReadRootDirectory(int port_num) {
    return ListDirectory(port_num, EXT2_ROOT_INO);
}

// Get a human-readable file type string
char* GetFileType(uint8_t type) {
    switch (type) {
        case EXT2_FT_REG_FILE: return (char*)"File";
        case EXT2_FT_DIR: return (char*)"Directory";
        case EXT2_FT_CHRDEV: return (char*)"Character Device";
        case EXT2_FT_BLKDEV: return (char*)"Block Device";
        case EXT2_FT_FIFO: return (char*)"FIFO";
        case EXT2_FT_SOCK: return (char*)"Socket";
        case EXT2_FT_SYMLINK: return (char*)"Symlink";
        default: return (char*)"Unknown";
    }
}

// Find the inode number of a directory by its path
uint32_t FindDirectoryInode(int port_num, const char* path) {
    // Start at the root directory inode
    uint32_t current_inode = EXT2_ROOT_INO;
    
    // If path is empty or just "/", return the root inode
    if (!path || !*path || (path[0] == '/' && path[1] == '\0')) {
        return current_inode;
    }
    
    // Make a copy of the path that we can modify
    char path_copy[256];
    strcpy(path_copy, path);
    
    // Skip leading slash if present
    char* current_path = path_copy;
    if (*current_path == '/') {
        current_path++;
    }
    
    // Process each path component
    char* next_component = strtok(current_path, "/");
    while (next_component) {
        // Special case: ignore "." (current directory)
        if (strcmp(next_component, ".") == 0) {
            next_component = strtok(NULL, "/");
            continue;
        }
        
        // Special case: ".." (parent directory) - not implemented
        if (strcmp(next_component, "..") == 0) {
            prErr("ext2", "'..' navigation not implemented yet");
            return 0;
        }
        
        // Read the current directory inode
        ext2_inode_t* inode = ReadInode(port_num, current_inode);
        if (!inode) {
            prErr("ext2", "Failed to read inode %u when looking for '%s'", current_inode, next_component);
            return 0;
        }
        
        // Check if it's a directory
        if (!(inode->mode & EXT2_S_IFDIR)) {
            prErr("ext2", "Inode %u is not a directory", current_inode);
            free(inode);
            return 0;
        }
        
        // Search for the next component in the current directory
        bool found = false;
        uint32_t next_inode = 0;
        
        // Read directory entries from direct blocks
        uint32_t remaining_size = inode->size;
        uint32_t block_index = 0;
        
        while (remaining_size > 0 && block_index < 12 && !found) {
            void* block_data = ReadInodeData(port_num, inode, block_index);
            if (!block_data) {
                free(inode);
                return 0;
            }
            
            uint32_t offset = 0;
            while (offset < block_size && offset < remaining_size && !found) {
                ext2_dir_entry_t* entry = (ext2_dir_entry_t*)((char*)block_data + offset);
                if (entry->inode != 0) {
                    // Compare entry name with the path component
                    if (entry->name_len == strlen(next_component)) {
                        bool name_matches = true;
                        for (int i = 0; i < entry->name_len; i++) {
                            if (entry->name[i] != next_component[i]) {
                                name_matches = false;
                                break;
                            }
                        }
                        
                        if (name_matches) {
                            next_inode = entry->inode;
                            found = true;
                        }
                    }
                }
                
                offset += entry->rec_len;
            }
            
            block_index++;
            remaining_size -= (remaining_size > block_size) ? block_size : remaining_size;
            free(block_data);
        }
        
        // Free the inode
        free(inode);
        
        if (!found) {
            prErr("ext2", "Directory entry '%s' not found", next_component);
            return 0;
        }
        
        // Update current inode and move to next component
        current_inode = next_inode;
        next_component = strtok(NULL, "/");
    }
    
    return current_inode;
}

// Check if a directory exists and get its inode if it does
bool GetDirectoryInode(int port_num, const char* path, uint32_t* out_inode) {
    if (!path || !out_inode) {
        return false;
    }
    
    uint32_t inode = FindDirectoryInode(port_num, path);
    if (inode == 0) {
        return false;
    }
    
    // Verify that the inode is actually a directory
    ext2_inode_t* inode_struct = ReadInode(port_num, inode);
    if (!inode_struct) {
        return false;
    }
    
    bool is_dir = (inode_struct->mode & EXT2_S_IFDIR) != 0;
    free(inode_struct);
    
    if (!is_dir) {
        prErr("ext2", "Path '%s' is not a directory", path);
        return false;
    }
    
    *out_inode = inode;
    return true;
}

// Find the inode number of a file by its path
uint32_t FindFileInode(int port_num, const char* path) {
    if (!path || !*path) {
        return 0;
    }
    
    // Make a copy of the path that we can modify
    char path_copy[256];
    strcpy(path_copy, path);
    
    // Extract the directory path and filename
    char* last_slash = strrchr(path_copy, '/');
    if (!last_slash) {
        // No slash in path, assume it's a file in the root directory
        last_slash = path_copy;
        *last_slash = '\0'; // Empty string for directory
        strcpy(path_copy, "/"); // Root directory
    } else {
        *last_slash = '\0'; // Split the string
    }
    
    const char* dir_path = path_copy;
    const char* filename = last_slash + 1;
    
    // Empty filename means the path ends with a slash, which is a directory
    if (!*filename) {
        prErr("ext2", "Path '%s' appears to be a directory, not a file", path);
        return 0;
    }
    
    // Find the parent directory's inode
    uint32_t dir_inode;
    if (strcmp(dir_path, "/") == 0) {
        // Root directory
        dir_inode = EXT2_ROOT_INO;
    } else {
        dir_inode = FindDirectoryInode(port_num, dir_path);
    }
    
    if (dir_inode == 0) {
        prErr("ext2", "Parent directory '%s' not found", dir_path);
        return 0;
    }
    
    // Read the directory inode
    ext2_inode_t* inode = ReadInode(port_num, dir_inode);
    if (!inode) {
        prErr("ext2", "Failed to read directory inode %u", dir_inode);
        return 0;
    }
    
    // Check if it's a directory
    if (!(inode->mode & EXT2_S_IFDIR)) {
        prErr("ext2", "Inode %u is not a directory", dir_inode);
        free(inode);
        return 0;
    }
    
    // Search for the filename in the directory
    uint32_t file_inode = 0;
    uint32_t remaining_size = inode->size;
    uint32_t block_index = 0;
    
    while (remaining_size > 0 && block_index < 12 && file_inode == 0) {
        void* block_data = ReadInodeData(port_num, inode, block_index);
        if (!block_data) {
            free(inode);
            return 0;
        }
        
        uint32_t offset = 0;
        while (offset < block_size && offset < remaining_size && file_inode == 0) {
            ext2_dir_entry_t* entry = (ext2_dir_entry_t*)((char*)block_data + offset);
            if (entry->inode != 0) {
                // Compare entry name with the filename
                if (entry->name_len == strlen(filename)) {
                    bool name_matches = true;
                    for (int i = 0; i < entry->name_len; i++) {
                        if (entry->name[i] != filename[i]) {
                            name_matches = false;
                            break;
                        }
                    }
                    
                    if (name_matches) {
                        file_inode = entry->inode;
                    }
                }
            }
            
            offset += entry->rec_len;
        }
        
        block_index++;
        remaining_size -= (remaining_size > block_size) ? block_size : remaining_size;
        free(block_data);
    }
    
    // Free the inode
    free(inode);
    
    if (file_inode == 0) {
        prErr("ext2", "File '%s' not found", filename);
    }
    
    return file_inode;
}

// Read the contents of a file
bool ReadFileContents(int port_num, uint32_t inode_num, char* buffer, uint32_t buffer_size, uint32_t* bytes_read) {
    if (!buffer || buffer_size == 0 || !bytes_read) {
        return false;
    }
    
    // Initialize bytes read
    *bytes_read = 0;
    
    // Read the file inode
    ext2_inode_t* inode = ReadInode(port_num, inode_num);
    if (!inode) {
        prErr("ext2", "Failed to read file inode %u", inode_num);
        return false;
    }
    
    // Check if it's a regular file
    if (!(inode->mode & EXT2_S_IFREG)) {
        prErr("ext2", "Inode %u is not a regular file", inode_num);
        free(inode);
        return false;
    }
    
    // Debug output
    prInfo("ext2", "File size: %u bytes", inode->size);
    
    // Determine how much we can read
    uint32_t size_to_read = (inode->size < buffer_size) ? inode->size : buffer_size;
    
    // Read from direct blocks
    uint32_t remaining_to_read = size_to_read;
    uint32_t bytes_read_so_far = 0;
    uint32_t block_index = 0;
    
    while (remaining_to_read > 0 && block_index < 12) {
        void* block_data = ReadInodeData(port_num, inode, block_index);
        if (!block_data) {
            // If we can't read a block but have already read some data, return what we have
            if (bytes_read_so_far > 0) {
                *bytes_read = bytes_read_so_far;
                free(inode);
                return true;
            }
            
            prErr("ext2", "Failed to read block %u of file inode %u", block_index, inode_num);
            free(inode);
            return false;
        }
        
        // Calculate how much to copy from this block
        uint32_t block_bytes_to_read = (remaining_to_read < block_size) ? remaining_to_read : block_size;
        
        // Debug output
        prInfo("ext2", "Reading %u bytes from block %u", block_bytes_to_read, block_index);
        
        // Copy the data to the output buffer
        memcpy(buffer + bytes_read_so_far, block_data, block_bytes_to_read);
        
        // Update counters
        bytes_read_so_far += block_bytes_to_read;
        remaining_to_read -= block_bytes_to_read;
        block_index++;
        
        // Free the block data
        free(block_data);
    }
    
    // Handle singly indirect blocks if needed (future implementation)
    if (remaining_to_read > 0 && inode->block[12] != 0) {
        prErr("ext2", "Indirect blocks not implemented yet");
    }
    
    // Free the inode
    free(inode);
    
    // Update bytes read
    *bytes_read = bytes_read_so_far;
    
    // Debug output
    prInfo("ext2", "Read %u bytes from file inode %u", bytes_read_so_far, inode_num);
    
    return true;
}

} // namespace EXT2
} // namespace FS 