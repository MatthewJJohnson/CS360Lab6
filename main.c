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




/******************* in <ext2type.h>*******************************
struct ext2_super_block {
  u32  s_inodes_count;       // total number of inodes
  u32  s_blocks_count;       // total number of blocks
  u32  s_r_blocks_count;     
  u32  s_free_blocks_count;  // current number of free blocks
  u32  s_free_inodes_count;  // current number of free inodes 
  u32  s_first_data_block;   // first data block in this group
  u32  s_log_block_size;     // 0 for 1KB block size
  u32  s_log_frag_size;
  u32  s_blocks_per_group;   // 8192 blocks per group 
  u32  s_frags_per_group;
  u32  s_inodes_per_group;    
  u32  s_mtime;
  u32  s_wtime;
  u16  s_mnt_count;          // number of times mounted 
  u16  s_max_mnt_count;      // mount limit
  u16  s_magic;              // 0xEF53
  // A FEW MORE non-essential fields
};
**********************************************************************/

//gloabals
char buf[BLKSIZE];
int fd;
int iblock;
int inode_size;
char *disk = "diskimage";
int bmap, imap;
int ninodes, nblocks, nfreeInodes, nfreeBlocks;
  int  i = 0;
//basic functions
int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

int put_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  write(fd, buf, BLKSIZE);
}

int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit / 8;  j = bit % 8;
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



int decFreeBlocks(int dev)
{
  char buf[BLKSIZE];
  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
}

int decFreeInodes(int dev)
{
  char buf[BLKSIZE];
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






//1.
// read SUPER block
super()
{

  printf("\n[SUPER INFORMATION]\n");
  get_block(fd, 1, buf);  
  sp = (SUPER *)buf;

  // check for EXT2 magic number:

  printf("s_magic = %x\n", sp->s_magic);
  if (sp->s_magic != 0xEF53){
    printf("[ERROR] NOT an EXT2 FS\n");
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
  inode_size = sp->s_inode_size;
}

//2.
gd()
{
	printf("\n[GROUP DESCRIPTOR INFORMATION]\n");
	get_block(fd, 1, buf);
	gp = (GD *)buf;

	printf("bg_block_bitmap = %d\n", gp->bg_block_bitmap);
	printf("bg_inode_bitmap = %d\n", gp->bg_inode_bitmap);
	printf("bg_inode_table = %d\n", gp->bg_inode_table);
	printf("bg_free_blocks_countt = %d\n", gp->bg_free_blocks_count);
	printf("bg_free_inodes_count = %d\n", gp->bg_free_inodes_count);
	printf("bg_used_dirs_count= %d\n", gp->bg_used_dirs_count);
}

//3.
int run_imap()
{

  printf("\n[IMAP INFORMATION]\n");
  int  i;

  // read SUPER block
  get_block(fd, 1, buf);
  sp = (SUPER *)buf;

  ninodes = sp->s_inodes_count;
  printf("number of inodes = %d\n", ninodes);

  // read Group Descriptor 0
  get_block(fd, 2, buf);
  gp = (GD *)buf;

  imap = gp->bg_inode_bitmap;
  printf("bmap = %d\n", imap);

  // read inode_bitmap block
  get_block(fd, imap, buf);

  for (i=0; i < ninodes; i++){
    (tst_bit(buf, i)) ?	putchar('1') : putchar('0');
    if (i && (i % 8)==0)
       printf(" ");
  }
  printf("\n");
}


//4.
run_bmap()
{

  printf("\n[BITMAP INFORMATION]\n");
  int  i;

  // read SUPER block
  get_block(fd, 1, buf);
  sp = (SUPER *)buf;

  nblocks = sp->s_blocks_count;
  printf("nblocks = %d\n", nblocks);

  // read Group Descriptor 0
  get_block(fd, 2, buf);
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  printf("block bitmap = %d\n", bmap);

  // read inode_bitmap block
  get_block(fd, bmap, buf);

  for (i=0; i < nblocks; i++)
  {
    (tst_bit(buf, i)) ?	putchar('1') : putchar('0');
    if (i && (i % 8)==0)
       printf(" ");
  }
  printf("\n");
}


//5.
inode()
{
  char buf[BLKSIZE];

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



int search(INODE *ip, char *name)
{

	get_block(fd, 33, buf);

	DIR *dp = (void *)buf;
	char *cp = buf;

  printf("\nINODE\tNAME\n");
  while (cp < &buf[1024])
	{
    printf("%d\t%s\n",dp->inode,dp->name);
	  if(strcmp(dp->name, name) == 0)
	  {
		  return dp->inode; //true, found, return inode number
	  }
    cp += dp->rec_len;
    dp = (void *) cp;
  }
	return 0; //false, didnt find name, return 0
}

//6.
//search for a function in the directory
dir()
{
	printf("\n[DIRECTORY INFORMATION]\n");

	ip = (INODE *)buf + 1;  // ip points at 2nd INODE
	
  printf("\nLooking for file named: NOTHEHRE!\n");
	int result = search(ip, "NOTHERE");

	if(result == 0)
	{
		printf("Name not found, 0 was returned!\n");
	}
	if(result != 0)
	{
		printf("Name found, the inode number returned was: %d\n", result);
	}
}

//7.
int ialloc(int dev)
{
  int i = 0;
  char buf[BLKSIZE];
  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeInodes(dev);

       put_block(dev, imap, buf);

       return i+1;
    }
  }
  printf("ialloc(): no more free inodes\n");
  return 0;
}

run_ialloc(int fd)
{
  i = 0; 
  int ino = 0;

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

  imap = gp->bg_inode_bitmap;
  printf("imap = %d\n", imap);
  getchar();

    ino = ialloc(fd);
    printf("allocated ino = %d\n", ino);
  
}


//8.
int balloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, bmap, buf);

  for (i=0; i < nblocks; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeBlocks(dev);

       put_block(dev, bmap, buf);

       return i+1;
    }
  }
  printf("balloc(): no more free blocks\n");
  return 0;
}


run_balloc(int fd)
{
  int i,bno;
   
  bmap = gp->bg_block_bitmap;
  printf("bmap = %d\n", bmap);
  //getchar();
 
    bno = balloc(fd);
    printf("allocated bno = %d\n", bno);
  

}




main(int argc, char *argv[ ])
{ 
  int i, bno;

  if (argc > 1)
  {
    disk = argv[1];
  }
  fd = open(disk, O_RDONLY);
  if (fd < 0){
    printf("open failed\n");
    exit(1);
  }


  






  //1.
  printf("\n---------------------------------------------\n");
  super();

  //2.
  printf("\n---------------------------------------------\n");
  gd();

  //3.
  printf("\n---------------------------------------------\n");
  run_imap();

  //4.
  printf("\n---------------------------------------------\n");
  run_bmap();

  //5.
  printf("\n---------------------------------------------\n");
  inode();

   //6.
  printf("\n---------------------------------------------\n");
  dir();


  //not working right
  printf("\n---------------------------------------------\n");
  run_ialloc(fd);
  printf("\n---------------------------------------------\n");
  run_balloc(fd);



  
}

/***** SAMPLE OUTPUTs of super.c ****************
s_inodes_count                 =      184
s_blocks_count                 =     1440
s_free_inodes_count            =      174
s_free_blocks_count            =     1411
s_log_block_size               =        0
s_blocks_per_group             =     8192
s_inodes_per_group             =      184
s_mnt_count                    =        1
s_max_mnt_count                =       34
s_magic                        =     ef53
s_mtime = Mon Feb  9 07:08:22 2004
s_inode_size                   =      128
************************************************/
