#ifndef _EXT2_H_
#define _EXT2_H_

#include <stdint.h>

namespace FS {
namespace EXT2 {

// Superblock structure
typedef struct {
    uint32_t inodes_count;          // Total number of inodes
    uint32_t blocks_count;          // Total number of blocks
    uint32_t r_blocks_count;        // Number of reserved blocks
    uint32_t free_blocks_count;     // Number of free blocks
    uint32_t free_inodes_count;     // Number of free inodes
    uint32_t first_data_block;      // First data block
    uint32_t log_block_size;        // Block size = 1024 << log_block_size
    uint32_t log_frag_size;         // Fragment size
    uint32_t blocks_per_group;      // Blocks per group
    uint32_t frags_per_group;       // Fragments per group
    uint32_t inodes_per_group;      // Inodes per group
    uint32_t mtime;                 // Mount time
    uint32_t wtime;                 // Write time
    uint16_t mnt_count;             // Mount count
    uint16_t max_mnt_count;         // Maximum mount count
    uint16_t magic;                 // Magic signature (0xEF53)
    uint16_t state;                 // File system state
    uint16_t errors;                // Error handling
    uint16_t minor_rev_level;       // Minor revision level
    uint32_t lastcheck;             // Last check time
    uint32_t checkinterval;         // Check interval
    uint32_t creator_os;            // Creator OS
    uint32_t rev_level;             // Revision level
    uint16_t def_resuid;            // Default UID for reserved blocks
    uint16_t def_resgid;            // Default GID for reserved blocks
    
    // EXT2 v1 fields (revision 1 and higher)
    uint32_t first_ino;             // First inode for standard files
    uint16_t inode_size;            // Size of inode structure
    uint16_t block_group_nr;        // Block group number
    uint32_t feature_compat;        // Compatible features
    uint32_t feature_incompat;      // Incompatible features
    uint32_t feature_ro_compat;     // Read-only compatible features
    uint8_t  uuid[16];              // Volume UUID
    char     volume_name[16];       // Volume name
    char     last_mounted[64];      // Last mounted directory
    uint32_t algo_bitmap;           // Compression algorithm bitmap
    
    // Performance hints
    uint8_t  prealloc_blocks;       // Blocks to preallocate
    uint8_t  prealloc_dir_blocks;   // Blocks to preallocate for directories
    uint16_t padding1;
    
    // Journaling support
    uint8_t  journal_uuid[16];      // Journal UUID
    uint32_t journal_inum;          // Journal inode
    uint32_t journal_dev;           // Journal device
    uint32_t last_orphan;           // Head of orphan inode list
    
    // Directory indexing support
    uint32_t hash_seed[4];          // HTREE hash seed
    uint8_t  def_hash_version;      // Default hash algorithm
    uint8_t  padding2[3];
    
    // Other options
    uint32_t default_mount_opts;    // Default mount options
    uint32_t first_meta_bg;         // First metadata block group
    uint8_t  reserved[760];         // Padding to 1024 bytes
} __attribute__((packed)) ext2_superblock_t;

// Block Group Descriptor
typedef struct {
    uint32_t block_bitmap;        // Block bitmap block
    uint32_t inode_bitmap;        // Inode bitmap block
    uint32_t inode_table;         // Inode table block
    uint16_t free_blocks_count;   // Free blocks count
    uint16_t free_inodes_count;   // Free inodes count
    uint16_t used_dirs_count;     // Directories count
    uint16_t pad;                 // Padding
    uint8_t  reserved[12];        // Reserved
} __attribute__((packed)) ext2_group_desc_t;

// Inode structure
typedef struct {
    uint16_t mode;           // File mode
    uint16_t uid;            // Owner UID
    uint32_t size;           // Size in bytes
    uint32_t atime;          // Access time
    uint32_t ctime;          // Creation time
    uint32_t mtime;          // Modification time
    uint32_t dtime;          // Deletion time
    uint16_t gid;            // Group ID
    uint16_t links_count;    // Links count
    uint32_t blocks;         // Blocks count (in 512-byte units)
    uint32_t flags;          // File flags
    uint32_t osd1;           // OS dependent value
    uint32_t block[15];      // Pointers to blocks
    uint32_t generation;     // File version
    uint32_t file_acl;       // File ACL
    uint32_t dir_acl;        // Directory ACL
    uint32_t faddr;          // Fragment address
    uint8_t  osd2[12];       // OS dependent value
} __attribute__((packed)) ext2_inode_t;

// Directory entry structure
typedef struct {
    uint32_t inode;         // Inode number
    uint16_t rec_len;       // Record length
    uint8_t  name_len;      // Name length
    uint8_t  file_type;     // File type
    char     name[];        // File name (variable length)
} __attribute__((packed)) ext2_dir_entry_t;

// File types
#define EXT2_FT_UNKNOWN    0
#define EXT2_FT_REG_FILE   1
#define EXT2_FT_DIR        2
#define EXT2_FT_CHRDEV     3
#define EXT2_FT_BLKDEV     4
#define EXT2_FT_FIFO       5
#define EXT2_FT_SOCK       6
#define EXT2_FT_SYMLINK    7

// File mode bits
#define EXT2_S_IFMT      0xF000  // Format mask
#define EXT2_S_IFSOCK    0xC000  // Socket
#define EXT2_S_IFLNK     0xA000  // Symbolic link
#define EXT2_S_IFREG     0x8000  // Regular file
#define EXT2_S_IFBLK     0x6000  // Block device
#define EXT2_S_IFDIR     0x4000  // Directory
#define EXT2_S_IFCHR     0x2000  // Character device
#define EXT2_S_IFIFO     0x1000  // FIFO

// Constants
#define EXT2_SUPER_MAGIC    0xEF53
#define EXT2_ROOT_INO       2     // Root inode number

// Function prototypes
bool Initialize(int port_num);
bool ReadSuperblock(int port_num, uint32_t sector_offset);
bool ReadBlockGroupDescriptors(int port_num);
bool ReadRootDirectory(int port_num);
bool ListDirectory(int port_num, uint32_t inode_num);
uint32_t FindDirectoryInode(int port_num, const char* path);
bool GetDirectoryInode(int port_num, const char* path, uint32_t* out_inode);
uint32_t FindFileInode(int port_num, const char* path);
bool ReadFileContents(int port_num, uint32_t inode_num, char* buffer, uint32_t buffer_size, uint32_t* bytes_read);
char* GetFileType(uint8_t type);

} // namespace EXT2
} // namespace FS

#endif // _EXT2_H_ 