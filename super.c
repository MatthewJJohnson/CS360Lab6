/***********************************************************************
 *
 *	Washington State University CPTS 360 
 *   
 *   Copyright (c) Kenneth Eversole and Matthew Johnson 2018
 *
 */


/********* super.c code ***************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <ext2fs/ext2_fs.h> 

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp;

#define BLKSIZE 1024
#define ISIZE 128

char *disk = "mydisk";

char buf[BLKSIZE];
char ibuf[BLKSIZE];
int fd, dev;
int ninodes, nblocks;
int nfreeInodes, nfreeBlocks;
int _bmap, _imap, iblk, iblock;

int get_block(int fd, int blk, char buf[])
{
  lseek(fd, (long)blk*BLKSIZE, SEEK_SET);
  return read(fd, buf, BLKSIZE);
}

int put_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, SEEK_SET);
  write(fd, buf, BLKSIZE);
}

int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}

int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}

int decFreeInodes(int dev)
{

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

int decFreeBlocks(int dev)
{
  

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
}

int ialloc(int dev)
{
  int  i;
 

  // read inode_bitmap block
  get_block(dev, _imap, buf);
  //ip = (INODE *)buf;

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       put_block(dev, _imap, buf);
       decFreeInodes(dev);
       return i+1;
    }
  }
  printf("ialloc(): no more free inodes\n");
  return 0;
}

imap()
{
 
  int  i;

  // read SUPER block
  get_block(fd, 1, buf);
  sp = (SUPER *)buf;

  ninodes = sp->s_inodes_count;
  printf("ninodes = %d\n", ninodes);

  // read Group Descriptor 0
  get_block(fd, 2, buf);
  gp = (GD *)buf;

  _imap = gp->bg_inode_bitmap;
  printf("imap = %d\n", _imap);

  // read inode_bitmap block
  get_block(fd, _imap, buf);

  printf("IMAP\n");
  for (i=0; i < ninodes; i++){
    (tst_bit(buf, i)) ?	putchar('1') : putchar('0');
    if (i && (i % 8)==0)
       printf(" ");
  }
  printf("\n");
}

inode()
{
  

  // read GD
  get_block(fd, 2, buf);
  gp = (GD *)buf;
  /****************
  printf("%8d %8d %8d %8d %8d %8d\n",
	 gp->bg_block_bitmap,
	 gp->bg_inode_bitmap,
	 gp->bg_inode_table,
	 gp->bg_free_blocks_count,
	 gp->bg_free_inodes_count,
	 gp->bg_used_dirs_count);
  ****************/
  iblock = gp->bg_inode_table;   // get inode start block#
  printf("inode_block=%d\n", iblock);

  // get inode start block
  get_block(fd, iblock, buf);

  ip = (INODE *)buf + 1;         // ip points at 2nd INODE

  printf("mode=%4x ", ip->i_mode);
  printf("uid=%d  gid=%d\n", ip->i_uid, ip->i_gid);
  printf("size=%d\n", ip->i_size);
  printf("time=%s", ctime(&ip->i_ctime));
  printf("link=%d\n", ip->i_links_count);
  printf("i_block[0]=%d\n", ip->i_block[0]);

 /*****************************
  u16  i_mode;        // same as st_imode in stat() syscall
  u16  i_uid;                       // ownerID
  u32  i_size;                      // file size in bytes
  u32  i_atime;                     // time fields
  u32  i_ctime;
  u32  i_mtime;
  u32  i_dtime;
  u16  i_gid;                       // groupID
  u16  i_links_count;               // link count
  u32  i_blocks;                    // IGNORE
  u32  i_flags;                     // IGNORE
  u32  i_reserved1;                 // IGNORE
  u32  i_block[15];                 // IMPORTANT, but later
 ***************************/
}

super()
{
  // read SUPER block
  get_block(fd, 1, buf);
  sp = (SUPER *)buf;

  // check for EXT2 magic number:

  printf("s_magic = %x\n", sp->s_magic);
  if (sp->s_magic != 0xEF53){
    printf("NOT an EXT2 FS\n");
    exit(1);
  }

  printf("EXT2 FS OK\n");

  printf("s_inodes_count = %d\n", sp->s_inodes_count);
  printf("s_blocks_count = %d\n", sp->s_blocks_count);

  printf("s_free_inodes_count = %d\n", sp->s_free_inodes_count);
  printf("s_free_blocks_count = %d\n", sp->s_free_blocks_count);
  printf("s_first_data_blcok = %d\n", sp->s_first_data_block);
  printf("s_log_block_size = %d\n", sp->s_log_block_size);
  printf("s_blocks_per_group = %d\n", sp->s_blocks_per_group);
  printf("s_inodes_per_group = %d\n", sp->s_inodes_per_group);
  printf("s_mnt_count = %d\n", sp->s_mnt_count);
  printf("s_max_mnt_count = %d\n", sp->s_max_mnt_count);
  printf("s_magic = %x\n", sp->s_magic);
  printf("s_mtime = %s", ctime(&sp->s_mtime));
  printf("s_wtime = %s", ctime(&sp->s_wtime));
  printf("s_wtime = %s", ctime(&sp->s_inode_size));
}

INODE *getInode(int dev, int ino)
{
  INODE *ip;
  int blk = iblk + (ino-1) / ISIZE;
  get_block(dev, blk, ibuf);
  ip = (INODE *)ibuf + (ino-1) % ISIZE;
  return ip;
}

//print group descriptor information
int gd()
{
  get_block(fd, 2, buf);//descriptor table stored in the block immediately aftet the superblock (2)
  gp = (GD *)buf;//group descriptor

  printf("Printing Group Descriptor at Block 2\n");
  printf("bg_block_bitmap = %d\n", gp->bg_block_bitmap);//block number of the block allocation for bitmap for this Block Group
  printf("bg_inode_bitmap = %d\n", gp->bg_inode_bitmap);//block number of the inode allocation bitmap for this Block Group
  printf("bg_inode_table = %d\n", gp->bg_inode_table);//block number of the starting block for the inode table for this Block Group
  printf("bg_free_blocks_count = %d\n", gp->bg_free_blocks_count);
  printf("bg_free_inodes_count = %d\n", gp->bg_free_inodes_count);
  printf("bg_used_dirs_count = %d\n", gp->bg_used_dirs_count);
  //group descriptors are placed one after another and together they make the group descriptor table
}
//display Bmap; the blocks bitmap
//similar to imap
int bmap()
{
    
    int  i;

    //read super block
    get_block(fd, 1, buf);
    sp = (SUPER *)buf;

    //numblocks
    nblocks = sp->s_blocks_count;
    printf("nblocks = %d\n", nblocks);

    //read group descriptor 0
    get_block(fd, 2, buf);
    gp = (GD *)buf;

    _bmap = gp->bg_inode_bitmap;
    printf("bmap = %d\n", _bmap);

    // read block_bitmap block
    get_block(fd, _bmap, buf);

    printf("BMAP\n");
    for (i=0; i < nblocks; i++){
      (tst_bit(buf, i)) ? putchar('1') : putchar('0');
      if (i && (i % 8)==0)
         printf(" ");
    }
    printf("\n");
}

int search(INODE *ip, char *name)
{
  char *cp;
  char temp[255];//1-255 char, not terminated with nul

  for (int i=0; i<12; i++){ // assume: DIRs have at most 12 direct blocks
    if (ip->i_block[i]==0)
      break;
    printf("i_blokc[%d] = %d\n", i, ip->i_block[i]);
    get_block(dev, ip->i_block[i], buf);

    cp = buf;
    dp = (DIR *)buf;

    while(cp < buf+1024){
       strncpy(temp, dp->name, dp->name_len);//1-255 char, not terminated with null!!!
       temp[dp->name_len] = 0;

       printf("%4d %4d %4d   %s\n", dp->inode, dp->rec_len, dp->name_len, temp);

       if(strcmp(temp, name) == 0)
       {
         return dp->inode;// Inode number; count from 1, NOT from 0
       }

       cp += dp->rec_len;
       dp = (DIR *)cp;
       memset(temp, 0, 255); //not it, kill that garbage
    }
  }
  return 0; //if not
}

//allocate a FREE disk block; return its bno
//similar to ialloc
int balloc(int dev)
{
    int i;
    
    //read block_bitmap block
    get_block(dev, _bmap, buf);

    for(i = 0; i < nblocks; i++)
    {
        if(tst_bit(buf, i)==0)
        {
            set_bit(buf, i);
            put_block(dev, _bmap, buf);
            decFreeBlocks(dev);
            return i+1;//return its bno. DONT FORGET YE +1! *off-by-1 devil gets sad cause he didnt get me*
        }
    }
    printf("balloc(): no more free blocks\n");
    return 0;
}



int main(int argc, char*argv[])
{
  int i, ino, bno;
  if (argc > 1)
    disk = argv[1];

  fd = open(disk, O_RDONLY);
  if (fd < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  // INODE *ip;
  // ip = getInode(dev, 2);
  // printf("imode = %4x\n", ip->i_mode);
  // if ((ip->i_mode & 0xF000) != 0x4000){
  //   printf("not a DIR\n");
  //   exit(1);
  // }
  //search(ip, "longNameDir");
  //search(ip, "shortNameDir");

  //commented stuff above is needed for search, but has inf loop
  super();
  gd();
  imap();
  bmap();
  inode();


  // read SUPER block
  get_block(fd, 1, buf);
  sp = (SUPER *)buf;

  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;
  nfreeInodes = sp->s_free_inodes_count;
  nfreeBlocks = sp->s_free_blocks_count;
  printf("ninodes=%d nblocks=%d nfreeInodes=%d nfreeBlocks=%d\n",
	 ninodes, nblocks, nfreeInodes, nfreeBlocks);

  // read Group Descriptor 0
  get_block(fd, 2, buf);
  gp = (GD *)buf;

  _imap = gp->bg_inode_bitmap;
  printf("imap = %d\n", _imap);
  //getchar();

  //IALLOC
  for (i=0; i < 5; i++){
    ino = ialloc(fd);
    printf("allocated ino = %d\n", ino);
  }

  //BALLOC
  for (i=0; i < 5; i++){
    bno = balloc(fd);
    printf("allocated bno = %d\n", bno);
  }
}
