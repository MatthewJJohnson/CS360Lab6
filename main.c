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

//gloabals
char buf[BLKSIZE];
int fd;
int iblock;
int inode_size;
char *diskimage = "diskimage";
int bmap, imap;
int ninodes, nblocks, nfreeInodes, nfreeBlocks;
int  i = 0;


char *path[128];
int current_depth = 0;


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
  printf("s_first_data_block = %d\n", sp->s_first_data_block);
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
//print group descriptor information
gd()
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

int search(INODE *ip, char *name)
{

	get_block(fd, 33, buf);

	DIR *dp = (void *)buf;
	char *cp = buf;

  while (cp < &buf[1024])
	{
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

  get_block(fd, 33, buf);

	DIR *dp = (void *)buf;
	char *cp = buf;

  printf("\nINODE\tNAME\n");
  while (cp < &buf[1024])
	{
    printf("%d\t%s\n",dp->inode,dp->name);
    cp += dp->rec_len;
    dp = (void *) cp;
  }

  printf("\nLooking for file named: %s\n", path[current_depth]);
	int result = search(ip, path[current_depth]);
  if (result != 0)
  {
    //print out loading found inode and the found inode number

  }
  else
  {
    //print out failed
  }

}


void parse_args(char **args)
{
  int i = 0;

  *args++;

  //setup disk image
  diskimage = malloc(strlen(*args) * sizeof(char));
  strcpy(diskimage,*args);
  *args++;

  //tokienize args here then put the tokens into the path
  char *temp = strtok(*args, "/");

    while(temp != NULL)
    {
        path[i] = malloc(strlen(temp) * sizeof(char));
        strcpy(path[i], temp);
        temp = strtok(0, "/");
        i++;

    }
}

main(int argc, char *argv[ ])
{
  int i, bno;

  if (argc > 1)
  {
    parse_args(argv);
  }

  fd = open(diskimage, O_RDONLY);
  if (fd < 0){
    printf("open failed\n");
    exit(1);
  }

  //1.
  printf("\n---------------------------------------------\n");
  super();
  getchar();

  //2.
  printf("\n---------------------------------------------\n");
  gd();
  getchar();

   //6.
  printf("\n---------------------------------------------\n");
  printf("Diskname is %s\n", diskimage);
  printf("Pathname is ");
  int ind=0;
  while(path[ind] != NULL)
  {
      printf("/%s", path[ind]);
      ind++;
  }
  printf("\n");
  dir();

}
