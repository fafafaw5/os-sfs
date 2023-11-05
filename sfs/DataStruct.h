#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

//定义超级块结构体
struct sb {
//文件系统的大小，以块为单位   
long fs_size;
//数据区的第一块块号，根目录也放在此
long first_blk; 
 //数据区大小，以块为单位
long datasize; 
 //inode区起始块号
long first_inode; 
//inode区大小，以块为单位  
long inode_area_size; 
 //inode位图区起始块号
long fisrt_blk_of_inodebitmap;
// inode位图区大小，以块为单位
long inodebitmap_size;  
//数据块位图起始块号
long first_blk_of_databitmap;  
//数据块位图大小，以块为单位
long databitmap_size;     
};

//定义inode结构体
struct inode { 
//对文件的权限
short int st_mode; 
//i-node号，2字节
short int st_ino;
//连接数，1字节
char st_nlink;
//拥有者的用户 ID ，4字节
uid_t st_uid; 
//拥有者的组 ID，4字节
gid_t st_gid; 
//文件大小，4字节
off_t st_size;
//16个字节 最近一次访问时间
struct timespec st_atim;
//磁盘地址，14字节 其中addr[0]-addr[3]是直接地址，addr[4]是一次间接，addr[5]是二次间接，addr[6]是三次间接。
short int addr [7];  
};

//定义数据块数据结构 用来存放文件内容
//缓冲区大小一共为512-4-2=506
struct Data_block
{   
    //用buffer来记录数据块写入的内容
    char buffer[502];
    //用来记录存入缓冲区的字节数 
    int data_size;
    //如果是间接数据块，则指示下一个数据块的偏移量
    short int next;
};
//定义目录项
struct directory_entry
{
    //文件名为8B
    char filename[8];
    //扩展名为3B
    char extension[3];
    //inode号为2B 实际使用12位
    short int f_inode;
    //备用3B
    char spare[3];
};