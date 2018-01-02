#ifndef EXT2_TYPE_H
#define EXT2_TYPE_H

#include <minix/libminixfs.h>

/* On the disk all attributes are stored in little endian format.
 * Inode structure was taken from linux/include/linux/ext2_fs.h.
 */
typedef struct {
    uint16_t  i_mode;         /* File mode */
    uint16_t  i_uid;          /* Low 16 bits of Owner Uid */
    uint32_t  i_size;         /* Size in bytes */
    uint32_t  i_atime;        /* Access time */
    uint32_t  i_ctime;        /* Creation time */
    uint32_t  i_mtime;        /* Modification time */
    uint32_t  i_dtime;        /* Deletion Time */
    uint16_t  i_gid;          /* Low 16 bits of Group Id */
    uint16_t  i_links_count;  /* Links count */
    uint32_t  i_blocks;       /* Blocks count */
    uint32_t  i_flags;        /* File flags */
    union {
        struct {
                uint32_t  l_i_reserved1;
        } linux1;
        struct {
                uint32_t  h_i_translator;
        } hurd1;
        struct {
                uint32_t  m_i_reserved1;
        } masix1;
    } osd1;                         /* OS dependent 1 */
    uint32_t  i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
    uint32_t  i_generation;   /* File version (for NFS) */
    uint32_t  i_file_acl;     /* File ACL */
    uint32_t  i_dir_acl;      /* Directory ACL */
    uint32_t  i_faddr;        /* Fragment address */
    union {
        struct {
            uint8_t    l_i_frag;       /* Fragment number */
            uint8_t    l_i_fsize;      /* Fragment size */
            uint16_t   i_pad1;
            uint16_t  l_i_uid_high;   /* these 2 fields    */
            uint16_t  l_i_gid_high;   /* were reserved2[0] */
            uint32_t   l_i_reserved2;
        } linux2;
        struct {
            uint8_t    h_i_frag;       /* Fragment number */
            uint8_t    h_i_fsize;      /* Fragment size */
            uint16_t  h_i_mode_high;
            uint16_t  h_i_uid_high;
            uint16_t  h_i_gid_high;
            uint32_t  h_i_author;
        } hurd2;
        struct {
            uint8_t    m_i_frag;       /* Fragment number */
            uint8_t    m_i_fsize;      /* Fragment size */
            uint16_t   m_pad1;
            uint32_t   m_i_reserved2[2];
        } masix2;
    } osd2;                         /* OS dependent 2 */
} d_inode;


/* Part of on disk directory (entry description).
 * It includes all fields except name (since size is unknown.
 * In revision 0 name_len is uint16_t (here is structure of rev >= 0.5,
 * where name_len was truncated with the upper 8 bit to add file_type).
 * MIN_DIR_ENTRY_SIZE depends on this structure.
 */
struct ext2_disk_dir_desc {
  uint32_t     d_ino;
  uint16_t     d_rec_len;
  uint8_t      d_name_len;
  uint8_t      d_file_type;
  char      d_name[1];
};

/* Current position in block */
#define CUR_DISC_DIR_POS(cur_desc, base)  ((char*)cur_desc - (char*)base)
/* Return pointer to the next dentry */
#define NEXT_DISC_DIR_DESC(cur_desc)	((struct ext2_disk_dir_desc*)\
					((char*)cur_desc + cur_desc->d_rec_len))
/* Return next dentry's position in block */
#define NEXT_DISC_DIR_POS(cur_desc, base) (cur_desc->d_rec_len +\
					   CUR_DISC_DIR_POS(cur_desc, base))
/* Structure with options affecting global behavior. */
struct opt {
  int use_orlov;		/* Bool: Use Orlov allocator */
  /* In ext2 there are reserved blocks, which can be used by super user only or
   * user specified by resuid/resgid. Right now we can't check what user
   * requested operation (VFS limitation), so it's a small warkaround.
   */
  int mfsalloc;			/* Bool: use mfslike allocator */
  int use_reserved_blocks;	/* Bool: small workaround */
  unsigned int block_with_super;/* Int: where to read super block,
                                 * uses 1k units. */
  int use_prealloc;		/* Bool: use preallocation */
};


#endif /* EXT2_TYPE_H */
