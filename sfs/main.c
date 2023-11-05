#include "DataStruct.h"
//宏定义使用双__开头避免与其他宏定义重复
//文件系统的块数 ：1024*1024*8(8M大小)/512(每块的大小)=16384
#define __block_number 16384
//数据区的第一个块号为518 前面有Inode位图占1块（块号为1） 数据块位图占4块（块号为4） inode区占512块 超级块的块号为0
#define __data_first 518
//数据区所占的块数 16384-518=15866
#define __data_size 15866
//inode区块号起始块号为4+1+1=6
#define __inode_first 6
//inode区大小为512块
#define __inode_size 512
//inode位图起始块号为1
#define __inodebitmap_first 1
//inode位图区所占块数为1
#define __inodebitmap_size 1
//数据块位图的起始块号为2
#define __databitmap_first 2
//数据块位图所占块数为4
#define __databitmap_size 4

int main() 
{   
    //完成格式化程序
    //1)生成1个8M大小的文件
    //生成一个名字为filesystem文件，模式为写模式
    FILE *file = fopen("/home/lei/Desktop/libfuse-master/sfs/filesystem", "wb");
    //判断是否成功生成文件
    if (file == NULL) {
        //如果生成失败则打印出失败信息
        printf("creat file fail!\n");
        return 0;
    }
    //如果生成成功则打印出成功信息
    printf("creat file sucess!\n");

    //2)将文件系统的相关信息写入超级块
    //动态为超级块分配内存
    struct sb *supver_block=malloc(sizeof(struct sb));
    //为超级块赋值
    //文件系统块数为16384
    supver_block->fs_size=__block_number;
    //数据区的起始块号为518
    supver_block->first_blk=__data_first;
    //数据区所占块数为15866
    supver_block->datasize=__data_size;
    //inode区的起始块号为6
    supver_block->first_inode=__inode_first;
    //inode区所占块数为512
    supver_block->inode_area_size=__inode_size;
    //inode位图的起始块号为1
    supver_block->fisrt_blk_of_inodebitmap=__inodebitmap_first;
    //inode位图所占块数大小为1
    supver_block->inodebitmap_size=__inodebitmap_size;
    //数据区位图的起始块号为1
    supver_block->first_blk_of_databitmap=__databitmap_first;
    //数据区位图所占块数大小为4
    supver_block->databitmap_size=__databitmap_size;

    //把超级块写入文件中并判断是否写入成功
   if(fwrite(supver_block,sizeof(struct sb),1,file)!=1)
   {    
        //如果写入失败打印出写入失败信息
        printf("write supver block fail!\n");
        return 0;
   }
   //如果写入成功则打印出写入成功信息
    printf("write supver block sucess!\n");

    //3)根目录作为文件系统的第一个文件
    //因为之前将超级块写进了文件中，所以要确保文件的指针要移动到512B位置(即第块号为1的位置)
    //把文件指针移动到512位置处
    if (fseek(file,512,SEEK_SET)!=0)
    {   
        //如果移动失败打印出失败信息
        printf("move seek fail at 512!\n");
        return 0;
    }
    //如果移动成功打印出成功信息
    printf("move seek success at 512!\n");
    //此时文件指针的位置在inode位图所占的块，初始化inode位图
    //定义数组来完成inode位图初始化 一共占1块 共512B
    int inode_arry[128];
    //将数组全部初始化为0 表明inode还未分配
    for (int i=0;i<128;++i)
        inode_arry[i]=0;
    //将数组的第一个元素的第一位赋值为1 表示根目录
    inode_arry[0] |=(1<<0);
    //将数组写入文件中并判断是否写入成功
    if(fwrite(inode_arry,sizeof(inode_arry),1,file)!=1)
    {
        //如果写入失败打印出失败信息
        printf("write inode_arry fail!\n");
        return 0;
    }
    //如果写入成功打印出写入成功信息
    printf("write inode_arry success!\n");
    //此时文件指针位置应该到了1024位置处
    if (fseek(file,1024,SEEK_SET)!=0)
    {
        printf("move seek fail at 1024!\n");
        return 0;
    }
    printf("move seek success at 1024!\n");

    //此时文件指针的位置在数据块位图所占的块，初始化数据块位图
    //将第一个数据块分配给根目录，与inode位图初始化类似
    //数据块位图占4块 512*4B
    int data_arry[512];
    //将数组初值全部赋值0表明此时数据块并没有被分配
    for (int i=0;i<512;++i)
        data_arry[i]=0;
    //将data_arry写入文件中并判断是否写入成功
    if(fwrite(data_arry,sizeof(data_arry),1,file)!=1)
    {
        //如果写入失败打印出失败信息
        printf("write data_arry fail!\n");
        return 0;
    }
    //如果写入成功打印出成功信息
    printf("write data_arry success!\n");
    //此时文件指针位置应该到了 1024+4*512=3072处
    if (fseek(file,3072,SEEK_SET)!=0)
    {
        printf("move seek fail at 3072!\n");
        return 0;
    }
    printf("move seek success at 3072!\n");

    //此时文件指针位置移动到了inode区所占的块
    //将根目录的相关信息填写到inode区的第一个inode
    struct inode *root=malloc(sizeof(struct inode));
    //对inode赋值
    //设置根目录的权限 依次为表示该文件是目录的宏 表示所有者具有读取权限的宏 表示所有者具有写入权限的宏 表示所有者具有执行权限的宏
    root->st_mode=__S_IFDIR|S_IRUSR|S_IWUSR|S_IXUSR;
    //inode号赋值为0
    root->st_mode=0;
    //连接数为2
    root->st_nlink=2;
    //拥有者id用getuid()函数获得
    root->st_uid=getuid();
    //拥有者的组用getgid()函数获得
    root->st_gid=getgid();
    //文件的大小目前为0
    root->st_size=0;
    //对最近一次访问时间进行赋值
    //获取当前时间并判断是否获取成功
    if(timespec_get(&root->st_atim,TIME_UTC)==0)
    {
        //如果获取失败则打印失败信息
        printf("get time fail!\n");
        return 0;
    }
    //如果获取成功则打印成功信息
    printf("get time success!\n");
    //对地址赋值，addr[0]-addr[3]是直接地址，addr[4]是一次间接，addr[5]是二次间接，addr[6]是三次间接。
    //这里采取直接地址赋值,除了addr[0]以外其他全部赋初值为-1表明该位置储存的信息不是地址
    for (int i=0;i<7;++i)
        root->addr[i]=-1;
    //表明根目录的直接地址为第0块数据块(即偏移量)
    root->addr[0]=0;
    //把整个inode写入文件中并判断是否写入成功
    if (fwrite(root,sizeof(struct inode),1,file)!=1)
    {
        //如果写入失败打印出失败信息
        printf("write root fail!\n");
        return 0;
    }
    printf("write root success!\n");
    //此时文件指针位置应该到了3072+512*512=265216处
    if (fseek(file,265216,SEEK_SET)!=0)
    {
        printf("move seek fail at 265216!\n");
        return 0;
    }
    printf("move seek success at 265216!\n");

    //此时文件指针位置到了数据区所占的块，初始化每一个数据块
    //一个数据块为512B 一个目录项共16B 一个数据块可以存放32个目录项 一共要存储4K个文件 那么只需要4096/32=128块数据块来存储目录项即可
    //为根目录的数据块进行操作
    struct directory_entry*dir=malloc(sizeof(struct directory_entry));
    for (int i=0;i<32;++i)
    {
        dir->filename[0]='\0';
        dir->extension[0]='\0';
        dir->f_inode=-1;
        dir->spare[0]='\0';
        fwrite(dir,sizeof(struct directory_entry),1,file);
        dir++;
    }
    for (int i=1;i<__data_size;++i)
    {   
        //为每一个数据块申请内存空间
        struct Data_block *data=malloc(sizeof(struct Data_block));
        //将每一个数据块写入文件并判断是否写入成功
        if(fwrite(data,sizeof(struct Data_block),1,file)!=1)
        {
            //如果写入失败打印出失败信息
            printf("write data fail!\n");
            return 0;
        }
    }
    //关闭文件
    fclose(file);
    //此时格式化程序完成，打印出完成消息
    printf("init disk success!\n");

    return 0;
}