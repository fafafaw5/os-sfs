#include "DataStruct.h"
//首先声明： 相对块号指从某个块开始计数的块号 而绝对块号是相对于0开始计数的块号
//声明宏定义
//定义一个块的大小
#define block_size 512
//先声明函数定义


// 此函数应查找输入路径，确保它是一个目录，然后列出内容。
// 要列出内容，您需要使用 filler（） 函数。例如：filler（buf， “.”， NULL， 0）;将当前目录添加到 ls -a 生成的列表中
// 通常，您只需将第二个参数更改为要添加到列表中的文件或目录的名称。
// 如果成功则返回0 如果失败则返回-ENOENT
// static int SFS_readdir();

// 此函数应将新目录添加到根级别，并应更新 .directory 文件
// 成功时为 0
// -ENAMETOOLONG，如果名称超过 8 个字符
// -EPERM 如果目录不在根目录下
// -EEXIST（如果目录已存在）
static int SFS_mkdir();



// 此函数应将 buf 中的数据写入由 path 表示的文件中，从偏移量开始。
// 成功则返回写入数据的大小
// -EFBIG，如果偏移量超出文件大小（但句柄附加）
static int SFS_write();

// 此函数应将文件中以路径表示的数据读取到 buf，从偏移量开始。
// 如果阅读成功返回阅读的字节大小
// -EISDIR 如果路径是目录
static int SFS_read();

// 删除文件
// 0 阅读成功
// -EISDIR 如果路径是目录
// -如果找不到该文件，则为 ENOENT
static int SFS_unlink();


//对inode位图操作，找到空闲的inode块，并对相应的位图进行操作
int allocateInode(int* inodeBitmap) 
{   
    //对整个位图进行遍历
    for (int i = 0; i < 128; i++) {
        for (int j = 0; j < 32; j++) {
            // 找到一个未使用的位
            if ((inodeBitmap[i] & (1 << j)) == 0) {  
                // 将该位设置为已使用
                inodeBitmap[i] |= (1 << j);  
                 // 计算 inode 的索引
                int inodeIndex = i * 32 + j;
                // 表示第几个inode块空闲（实际上是偏移量）（从inode区开始计数）所以找实际块号要加上sb中的first_inode
                return inodeIndex;
            }
        }
    }
    //没有可用的inode
    return -1; 
}
//释放占用的inode inodeIndex传入的参数为相对块号 要用绝对块号减去sb中的first_inode
void freeInode(int *inodeBitmap,int inodeIndex) 
{
    int bitmapIndex = inodeIndex / 32;
    int bitOffset = inodeIndex % 32;
    // 将指定 inode的位设置为未使用
    inodeBitmap[bitmapIndex] &= ~(1 << bitOffset); 
}

//同理对数据块位图进行同样的操作
int allocateData(int* DataBitmap) 
{   
    //对整个位图进行遍历
    for (int i = 0; i < 512; i++) {
        for (int j = 0; j < 32; j++) {
            // 找到一个未使用的位
            if ((DataBitmap[i] & (1 << j)) == 0) {  
                // 将该位设置为已使用
                DataBitmap[i] |= (1 << j);  
                 // 计算 inode 的索引
                int inodeIndex = i * 32 + j;
                // 表示第几个数据块空闲（实际上是偏移量）（从inode区开始计数）所以找实际块号要加上sb中的first_blk
                return inodeIndex;
            }
        }
    }
    //没有可用的数据块
    return -1; 
}
//释放占用的数据块 inodeIndex传入的参数为相对块号 要用绝对块号减去sb中的first_blk
void freeData(int *DataBitmap,int inodeIndex) 
{
    int bitmapIndex = inodeIndex / 32;
    int bitOffset = inodeIndex % 32;
    // 将指定 inode的位设置为未使用
    DataBitmap[bitmapIndex] &= ~(1 << bitOffset); 
}
//根据文件路径找到inode号 
int path_to_inode(const char*path)
{   
    //打开磁盘
    FILE *file = fopen("/home/lei/Desktop/libfuse-master/sfs/filesystem", "r");
    //读出超级块
    struct sb *super=malloc(sizeof(struct sb));
    fread(super,sizeof(struct sb),1,file); 
    //首先判断是否是根目录
    if(strcmp(path,"/")==0)
    {
        printf("要查询的路径是根目录");
        return super->first_inode;
    }
    //如果不是根目录则继续往下查询，查询根目录的目录项
    struct inode*Inode=malloc(sizeof(struct inode));
    struct directory_entry*dir=malloc(sizeof(struct directory_entry));
    struct Data_block*data=malloc(sizeof(struct Data_block));
    //读出根目录的inode信息
    fseek(file,super->first_inode*512,SEEK_SET);
    fread(Inode,sizeof(struct inode),1,file);
    //根据inode里面的addr读出数据块，此时数据块存储的信息是目录项
    fseek(file,(Inode->addr[0]+super->first_blk)*512,SEEK_SET);
    fread(data,sizeof(struct Data_block),1,file);
    //将data赋值给directory_entry
    dir=(struct directory_entry*)data;
    //将path复制一份，以便接下来的操作
    char*temp=malloc(strlen(path)+1);
    strcpy(temp,path);
    char*filename=malloc(strlen(path)+1);
    filename=strtok(temp,"/");
    char*next=malloc(strlen(path)+1);
    char*fextension=malloc(strlen(path)+1);
    int feindex=0;
    char* fn=malloc(strlen(path)+1);
    int fnindex=0;
    //判断此时的filename是文件名还是目录名
    int flag=0;
    for (int i=0;i<strlen(filename);i++)
    {
        if (filename[i]=='.')
        {
            flag=1;
            continue;
        }
        if (flag==1)
        {
            fextension[feindex++]=filename[i];
        }
        fn[fnindex++]=filename[i];
    }
    fextension[feindex]='\0';
    fn[fnindex]='\0';
    //此时没有扩展名
    if (flag==0)
    {
        while(strcmp(filename,dir->filename)!=0)
            dir++;
        //将该文件所对应的inode读出来
        fseek(file,dir->f_inode*64+super->first_inode*512,SEEK_SET);
        fread(Inode,sizeof(struct inode),1,file);
    }
    //如果有扩展名，则filename一定是路径的末尾
    else if (flag==1)
    {
        while (1)
        {
            if (strcmp(fn,dir->filename)==0&&strcmp(fextension,dir->extension)==0)
                break;
            dir++;
        }
        //将该文件所对应的inode读出来
        fseek(file,dir->f_inode*64+super->first_inode*512,SEEK_SET);
        fread(Inode,sizeof(struct inode),1,file);
        //释放内存
        free(super);
        free(Inode);
        free(dir);
        free(data);
        free(temp);
        free(filename);
        free(fextension);
        free(fn);
        free(next);
        return Inode->st_ino;
    }
    do{
        //next用来判断路径是否走完,如果为空，则说明此时filename就是路径的末尾
        next=strtok(NULL,"/");
        if (next==NULL)
            {   
                //释放内存
                free(super);
                free(Inode);
                free(dir);
                free(data);
                free(temp);
                free(filename);
                free(fextension);
                free(fn);
                free(next);
                return Inode->st_ino;
            }
        filename=next;
        //如果是直接寻址
        if (Inode->addr[0]!=-1)
        {   
            int address=Inode->addr[0];
            if (Inode->addr[1]!=-1)
                address+=Inode->addr[1];
            if (Inode->addr[2]!=-1)
                address+=Inode->addr[2];
            if (Inode->addr[3]!=-1)
                address+=Inode->addr[3];
            //根据inode里面的addr读出数据块，此时数据块存储的信息是目录项
            fseek(file,(address+super->first_blk)*512,SEEK_SET);
            fread(data,sizeof(struct Data_block),1,file);
            dir=(struct directory*)data;
            char*fextension=malloc(strlen(path)+1);
            int feindex=0;
            char* fn=malloc(strlen(path)+1);
            int fnindex=0;
            //判断此时的filename是文件名还是目录名
            int flag=0;
            for (int i=0;i<strlen(filename);i++)
            {
                if (filename[i]=='.')
                {
                    flag=1;
                    continue;
                }
                if (flag==1)
                {
                    fextension[feindex++]=filename[i];
                }
                fn[fnindex++]=filename[i];
            }
            fextension[feindex]='\0';
            fn[fnindex]='\0';
            //此时没有扩展名
            if (flag==0)
            {
                while(strcmp(filename,dir->filename)!=0)
                    dir++;
                //将该文件所对应的inode读出来
                fseek(file,dir->f_inode*64+super->first_inode*512,SEEK_SET);
                fread(Inode,sizeof(struct inode),1,file);
            }
            //如果有扩展名，则filename一定是路径的末尾
            else if (flag==1)
            {
                while (1)
                {
                    if (strcmp(fn,dir->filename)==0&&strcmp(fextension,dir->extension)==0)
                        break;
                    dir++;
                }
                //将该文件所对应的inode读出来
                fseek(file,dir->f_inode*64+super->first_inode*512,SEEK_SET);
                fread(Inode,sizeof(struct inode),1,file);
                //释放内存
                free(super);
                free(Inode);
                free(dir);
                free(data);
                free(temp);
                free(filename);
                free(fextension);
                free(fn);
                free(next);
                return Inode->st_ino;
            }
        }
        //一级间接寻址
        else if (Inode->addr[4]!=-1)
        {
            //移动到一级间接数据块
            fseek(file,(Inode->addr[4]+super->first_blk)*512,SEEK_SET);
            //读出间接数据块内容
            fread(data,sizeof(struct Data_block),1,file);
            //移动到直接数据块
            fseek(file,(data->next+super->first_blk)*512,SEEK_SET);
            fread(data,sizeof(struct Data_block),1,file);
            dir=(struct directory*)data;
            char*fextension=malloc(strlen(path)+1);
            int feindex=0;
            char* fn=malloc(strlen(path)+1);
            int fnindex=0;
            //判断此时的filename是文件名还是目录名
            int flag=0;
            for (int i=0;i<strlen(filename);i++)
            {
                if (filename[i]=='.')
                {
                    flag=1;
                    continue;
                }
                if (flag==1)
                {
                    fextension[feindex++]=filename[i];
                }
                fn[fnindex++]=filename[i];
            }
            fextension[feindex]='\0';
            fn[fnindex]='\0';
            //此时没有扩展名
            if (flag==0)
            {
                while(strcmp(filename,dir->filename)!=0)
                    dir++;
                //将该文件所对应的inode读出来
                fseek(file,dir->f_inode*64+super->first_inode*512,SEEK_SET);
                fread(Inode,sizeof(struct inode),1,file);
            }
            //如果有扩展名，则filename一定是路径的末尾
            else if (flag==1)
            {
                while (1)
                {
                    if (strcmp(fn,dir->filename)==0&&strcmp(fextension,dir->extension)==0)
                        break;
                    dir++;
                }
                //将该文件所对应的inode读出来
                fseek(file,dir->f_inode*64+super->first_inode*512,SEEK_SET);
                fread(Inode,sizeof(struct inode),1,file);
                //释放内存
                free(super);
                free(Inode);
                free(dir);
                free(data);
                free(temp);
                free(filename);
                free(fextension);
                free(fn);
                free(next);
                return Inode->st_ino;
            }
        }
        //二级间接寻址
        else if (Inode->addr[5]!=-1)
        {
            //移动到二级间接数据块
            fseek(file,(Inode->addr[5]+super->first_blk)*512,SEEK_SET);
            //读出二级间接数据块内容
            fread(data,sizeof(struct Data_block),1,file);
            //移动到一级间接数据块
            fseek(file,(data->next+super->first_blk)*512,SEEK_SET);
            //读出一级间接数据块内容
            fread(data,sizeof(struct Data_block),1,file);
            //移动到直接数据块
            fseek(file,(data->next+super->first_blk)*512,SEEK_SET);
            fread(data,sizeof(struct Data_block),1,file);
            dir=(struct directory*)data;
            char*fextension=malloc(strlen(path)+1);
            int feindex=0;
            char* fn=malloc(strlen(path)+1);
            int fnindex=0;
            //判断此时的filename是文件名还是目录名
            int flag=0;
            for (int i=0;i<strlen(filename);i++)
            {
                if (filename[i]=='.')
                {
                    flag=1;
                    continue;
                }
                if (flag==1)
                {
                    fextension[feindex++]=filename[i];
                }
                fn[fnindex++]=filename[i];
            }
            fextension[feindex]='\0';
            fn[fnindex]='\0';
            //此时没有扩展名
            if (flag==0)
            {
                while(strcmp(filename,dir->filename)!=0)
                    dir++;
                //将该文件所对应的inode读出来
                fseek(file,dir->f_inode*64+super->first_inode*512,SEEK_SET);
                fread(Inode,sizeof(struct inode),1,file);
            }
            //如果有扩展名，则filename一定是路径的末尾
            else if (flag==1)
            {
                while (1)
                {
                    if (strcmp(fn,dir->filename)==0&&strcmp(fextension,dir->extension)==0)
                        break;
                    dir++;
                }
                //将该文件所对应的inode读出来
                fseek(file,dir->f_inode*64+super->first_inode*512,SEEK_SET);
                fread(Inode,sizeof(struct inode),1,file);
                //释放内存
                free(super);
                free(Inode);
                free(dir);
                free(data);
                free(temp);
                free(filename);
                free(fextension);
                free(fn);
                free(next);
                return Inode->st_ino;
            }
        }
        //三级间接寻址
        else if (Inode->addr[6]!=-1)
        {
            //移动到三级间接数据块
            fseek(file,(Inode->addr[6]+super->first_blk)*512,SEEK_SET);
            //读出三级间接数据块内容
            fread(data,sizeof(struct Data_block),1,file);
            //移动到二级间接数据块
            fseek(file,(data->next+super->first_blk)*512,SEEK_SET);
            //读出二级间接数据块内容
            fread(data,sizeof(struct Data_block),1,file);
            //移动到一级间接数据块
            fseek(file,(data->next+super->first_blk)*512,SEEK_SET);
            //读出一级间接数据块内容
            fread(data,sizeof(struct Data_block),1,file);
            //移动到直接数据块
            fseek(file,(data->next+super->first_blk)*512,SEEK_SET);
            fread(data,sizeof(struct Data_block),1,file);
            dir=(struct directory*)data;
            char*fextension=malloc(strlen(path)+1);
            int feindex=0;
            char* fn=malloc(strlen(path)+1);
            int fnindex=0;
            //判断此时的filename是文件名还是目录名
            int flag=0;
            for (int i=0;i<strlen(filename);i++)
            {
                if (filename[i]=='.')
                {
                    flag=1;
                    continue;
                }
                if (flag==1)
                {
                    fextension[feindex++]=filename[i];
                }
                fn[fnindex++]=filename[i];
            }
            fextension[feindex]='\0';
            fn[fnindex]='\0';
            //此时没有扩展名
            if (flag==0)
            {
                while(strcmp(filename,dir->filename)!=0)
                    dir++;
                //将该文件所对应的inode读出来
                fseek(file,dir->f_inode*64+super->first_inode*512,SEEK_SET);
                fread(Inode,sizeof(struct inode),1,file);
            }
            //如果有扩展名，则filename一定是路径的末尾
            else if (flag==1)
            {
                while (1)
                {
                    if (strcmp(fn,dir->filename)==0&&strcmp(fextension,dir->extension)==0)
                        break;
                    dir++;
                }
                //将该文件所对应的inode读出来
                fseek(file,dir->f_inode*64+super->first_inode*512,SEEK_SET);
                fread(Inode,sizeof(struct inode),1,file);
                //释放内存
                free(super);
                free(Inode);
                free(dir);
                free(data);
                free(temp);
                free(filename);
                free(fextension);
                free(fn);
                free(next);
                return Inode->st_ino;
            }
        }
    }
    while(1);
}

//具体函数的实现
// 此函数应查找输入路径以确定它是目录还是文件。如果是目录，则返回相应的权限。
// 如果是文件，则返回专有权限以及实际大小。此大小必须准确，因为它用于确定 EOF，因此可能无法调用读取。
// 如果成功则返回0以及正确的设置结构 如果失败则返回-ENOENT
//1、实现SFS_getattr
static int SFS_getattr(const char* path,struct stat*buf,struct fuse_file_info *fi)
{   
    printf("正在调用SFS_getattr函数\n");
    //在这个函数中并没有用到fi，在这里赋空类型防止出现警告
    (void) fi;
    //首先判断是不是根目录
    //如果是根目录，则返回相应的权限
    if (strcmp(path,"/")==0)
    {
        //此时要打开磁盘，根据inode数据结构返回权限
        FILE *file=fopen("/home/lei/Desktop/libfuse-master/sfs/filesystem", "r");
        //将文件指针移动到根目录所对应的inode区,第七块，块号为6
        fseek(file,block_size*6,SEEK_SET);
        //此时读出inode记录的信息 并返回相应的权限
        //首先给inode分配内存空间
        struct inode *dir=malloc(sizeof(struct inode));
        //从文件读取根目录inode信息并存储到dir中
        fread(dir,sizeof(struct inode),1,file);
        //把inode所对应的权限赋值给buf
        buf->st_mode=dir->st_mode;
        //关闭文件
        fclose(file);
        //释放dir
        free(dir);
        return 0;
    }
    //如果不是根目录，判断是否存在该文件，判断方法为打开路径的文件
    FILE*temp=fopen(path,"r");
    //如果打不开，说明没有该文件
    if (temp==NULL)
        {   
            fclose(temp);
            return -ENOENT;
        }
    fclose(temp);
    //否则找到根据路径找到对应的inode号
    int inode=path_to_inode(path);
    //打开磁盘
    FILE *file = fopen("/home/lei/Desktop/libfuse-master/sfs/filesystem", "r");
    //读出超级块
    struct sb*super_block=malloc(sizeof(struct sb));
    fread(super_block,sizeof(struct sb),1,file);
    //将文件指针移动到inode号对应的区域
    fseek(file,inode*64+super_block->first_inode*512,SEEK_SET);
    //读出inode
    struct inode*Inode=malloc(sizeof(struct inode));
    fread(Inode,sizeof(struct inode),1,file);
    buf->st_ino=Inode->st_ino;
    buf->st_atime=Inode->st_atim.tv_sec;
    //判断是否是目录
    //如果是目录,则返回相应的权限
    if (Inode->st_mode&__S_IFDIR==1)
    {
        buf->st_mode=Inode->st_mode;
        //释放内存
        free(super_block);
        free(Inode);
        //关闭文件
        fclose(file);
        return 0;
    }
    //否则是文件
    else
    {
        buf->st_mode=Inode->st_mode;
        buf->st_size=Inode->st_size;
        //释放内存
        free(super_block);
        free(Inode);
        //关闭文件
        fclose(file);
        return 0;
    }
}

// 此函数应将新文件添加到子目录，并应使用修改后的目录条目结构适当更新 .directory 文件。
// 成功时为 0
// -ENAMETOOLONG，如果名称超过 8.3 个字符
// -EPERM 如果文件正在尝试在根目录中创建
// -EEXIST 如果文件已存在
static int SFS_mknod(const char*path,mode_t mode,dev_t dev)
{   
    int res=0;
    printf("正在调用SFS_mkonde函数\n");
    //先判断该文件是否存在
    FILE* exist=fopen(path,"r");
    //如果该文件存在，则返回错误
    if (exist!=NULL)
    {
        printf("该文件已经存在!\n");
        fclose(exist);
        return -EEXIST;
    }
    //关闭文件
    fclose(exist);
    //假设要创建的文件名为test 路径为 /home/lei/test
    //则我们先要找到test前面的路径
    char*filepath=malloc(strlen(path)+1);
    char*temp=malloc(strlen(path)+1);
    strcpy(filepath,path);
    temp=strrchr(filepath,'/');
    //先对该文件名是否合法进行判断，即filename不能超过8 fextension不能超过3
    char* temp1=malloc(strlen(path)+1);
    strcpy(temp1,temp+1);
    //判断是否有扩展名，此时temp1保留的是test
    int before=0;
    int behind=0;
    //定义文件名
    char* fname=malloc(strlen(path)+1);
    //定义扩展名
    char *fextension=malloc(strlen(path)+1);
    int flag=0;
    for (int i=0;i<strlen(temp1);++i)
    {
        if (temp1[i]=='.')
        {
            flag=1;
            continue;
        }
        if (flag==0)
            fname[before++]=temp1[i];
        else
            fextension[behind++]=temp1[i];
    }
    fname[before]='\0';
    fextension[behind]='\0';
    //文件名不合法
    if (before>8||behind>3)
    {
        //释放内存
        free(filepath);
        free(temp);
        free(temp1);
        free(fname);
        free(fextension);
        //返回错误
        printf("文件名非法!\n");
        return -ENAMETOOLONG;
    }
    if (temp!=NULL)
        *temp="\0";
    //此时filepath保留的就是新增文件名前面的路径
    //如果filepath此时为"\0" 则说明是在根目录下创建文件
    if (filepath[0]=='\0')
    {
        //释放内存
        free(filepath);
        free(temp);
        free(temp1);
        free(fname);
        free(fextension);
        //返回错误
        printf("尝试在根目录下创建文件!\n");
        return -EEXIST;
    }
    //读出超级块
    FILE *file = fopen("/home/lei/Desktop/libfuse-master/sfs/filesystem", "r");
    struct sb*super_block=malloc(sizeof(struct sb));
    fread(super_block,sizeof(struct sb),1,file);
    //将文件指针移动到inode位图区
    fseek(file,super_block->fisrt_blk_of_inodebitmap*512,SEEK_SET);
    int*inodeBitmap=malloc(sizeof(int)*128);
    //读出inode位图
    fread(inodeBitmap,sizeof(int)*128,1,file);
    //将文件指针移动到数据块位图
    fseek(file,super_block->first_blk_of_databitmap*512,SEEK_SET);
    int*dataBitmap=malloc(sizeof(int)*512);
    //读出数据块位图
    fread(dataBitmap,sizeof(int)*512,1,file);
    //找到一个空的inode,并把所创建的文件相关信息写入inode
    int inode_free=allocateInode(inodeBitmap);
    //如果找不到空闲的inode，则打印出错误信息并返回错误
    if (inode_free==-1)
    {
        printf("无空闲inode可用!\n");
        return -errno;
    }
    //如果找得到空闲的inode
    //将文件指针移动到刚刚找到的空的inode上
    fseek(file,inode_free*64+super_block->first_inode*512,SEEK_SET);
    struct inode*new_inode=malloc(sizeof(struct inode));
    //对该inode赋值方便下次使用
    new_inode->st_ino=inode_free;
    new_inode->st_gid=getgid();
    new_inode->st_uid=getuid();
    new_inode->st_size=0;
    new_inode->st_nlink=1;
    timespec_get(&new_inode->st_atim,TIME_UTC);
    new_inode->st_mode=S_IRUSR|S_IWUSR|S_IXUSR|__S_IFREG;
    //找到一个空的数据块
    int new_data=allocateData(dataBitmap);
    //如果找不到空闲数据块
    if (new_data==-1)
    {
        printf("找不到空闲数据块!\n");
        return ENOSPC;
    }
    //一共有15866个数据块
    //假设直接数据块用866个 一级用1000个 二级用5000个 三级用9000
    //根据不同的数据块号选择不同的选址方式
    //直接选址
    if (new_data<866)
    {
        memset(new_inode->addr,-1,7);
        new_inode->addr[0]=new_data;
    }
    //一次间接寻址
    else if (new_data<1866)
    {
        memset(new_inode->addr,-1,7);
        new_inode->addr[4]=new_data;
        //该数据块为一级间接数据块
        struct Data_block*first=malloc(sizeof(struct Data_block));
        //再找一个空闲的数据块
        int first_blk=allocateData(dataBitmap);
        //如果找不到空闲的数据块，则返回错误
        if (first_blk==-1)
        {
            printf("找不到空闲数据块!\n");
            return ENOSPC;
        }
        first->next=first_blk;
        //将下列设置为0表明该数据块是间接数据块
        first->buffer[0]='\0';
        first->data_size=0;
        //移动文件指针
        fseek(file,super_block->first_blk*512+new_data*512,SEEK_SET);
        //把该数据块写入磁盘
        fwrite(first,sizeof(struct Data_block),1,file);
    }
    //二次间接寻址
    else if (new_data<6866)
    {
        memset(new_inode->addr,-1,7);
        new_inode->addr[5]=new_data;
        //该数据块为二次间接数据块
        struct Data_block*second=malloc(sizeof(struct Data_block));
        //再找一个新的数据块
        int second_blk=allocateData(dataBitmap);
        //如果找不到新的数据块，则返回错误
        if (second_blk==-1)
        {
            printf("找不到空闲数据块!\n");
            return ENOSPC;
        }
        second->next=second_blk;
        //将下列值赋值为0表明该数据块是间接数据块
        second->data_size=0;
        second->buffer[0]='\0';
        //移动文件指针
        fseek(file,super_block->first_blk*512+new_data*512,SEEK_SET);
        //把二级间接数据块写入文件中
        fwrite(second,sizeof(struct Data_block),1,file);
        //该数据块为一级间接数据块
        struct Data_block*first=malloc(sizeof(struct Data_block));
        //再找一个新的数据块
        int first_blk=allocateData(dataBitmap);
        //如果找不到新的数据块，则返回错误
        if (first_blk==-1)
        {
            printf("找不到空闲数据块!\n");
            return ENOSPC;
        }
        first->next=first_blk;
        //将下列值赋值为0表明该数据块是间接数据块
        first->buffer[0]='\0';
        first->data_size=0;
        //再次移动文件指针
        fseek(file,super_block->first_blk*512+second_blk*512,SEEK_SET);
        //把一级数据块写入磁盘
        fwrite(first,sizeof(struct Data_block),1,file);
    }
    //三次间接寻址
    else
    {
        memset(new_inode->addr,-1,7);
        new_inode->addr[6]=new_data;
        //找到一个新的数据块
        int third_blk=allocateData(dataBitmap);
        //如果找不到新的数据块，则返回错误
        if (third_blk==-1)
        {
            printf("找不到空闲数据块!\n");
            return ENOSPC;
        }
        //该数据块为三次间接数据块
        struct Data_block*third=malloc(sizeof(struct Data_block));
        third->next=third_blk;
        //将下列值赋值为0，表明该数据块为间接数据块
        third->buffer[0]='\0';
        third->data_size=0;
        //移动文件指针
        fseek(file,new_data*512+super_block->first_blk*512,SEEK_SET);
        //将三级间接数据块写入磁盘中
        fwrite(third,sizeof(struct Data_block),1,file);
        //再找一个新的数据块
        int second_blk=allocateData(dataBitmap);
        //如果找不到新的数据块，则返回错误
        if (second_blk==-1)
        {
            printf("找不到空闲数据块!\n");
            return ENOSPC;
        }
        //该数据块为二次间接数据块
        struct Data_block*second=malloc(sizeof(struct Data_block));
        second->next=second_blk;
        //将下列值赋值为0，表明该数据块为间接数据块
        second->data_size=0;
        second->buffer[0]='\0';
        //移动文件指针
        fseek(file,third_blk*512+super_block->first_blk*512,SEEK_SET);
        //将二级间接数据块写入磁盘
        fwrite(second,sizeof(struct Data_block),1,file);
        //再找一个新的数据块
        int first_blk=allocateData(dataBitmap);
        //如果找不到新的数据块，则返回错误
        if (first_blk==-1)
        {
            printf("找不到空闲数据块!\n");
            return ENOSPC;
        }
        //该数据块为一次间接数据块
        struct Data_block*first=malloc(sizeof(struct Data_block));
        first->next=first_blk;
        //将下列值赋值为0，表明该数据块为间接数据块
        first->data_size=0;
        first->buffer[0]='\0';
        //移动文件指针
        fseek(file,second_blk*512+super_block->first_blk*512,SEEK_SET);
        //将一级间接块写入磁盘
        fwrite(first,sizeof(struct Data_block),1,file);
    }
    //移动文件指针，将该inode写入磁盘
    fseek(file,inode_free*64+super_block->first_inode*512,SEEK_SET);
    fwrite(new_inode,sizeof(struct inode),1,file);
    
    //接下来要在对应的目录项中增加该文件
    //找到该路径所对应的inode号
    int inode=path_to_inode(filepath);
    //将文件指针移动到所对应的inode区,读出inode
    fseek(file,inode*64+super_block->first_inode*512,SEEK_SET);
    struct inode*Inode=malloc(sizeof(struct inode));
    fread(Inode,sizeof(struct inode),1,file);
    //将父目录的连接数加1
    Inode->st_nlink++;
    //将更新后的父目录inode写入磁盘中
    fwrite(Inode,sizeof(struct inode),1,file);
    //根据inode读出目录项所在的数据块
    //如果是直接寻址
    if (Inode->addr[0]!=-1)
    {
        int address=Inode->addr[0];
        if (Inode->addr[1]!=-1)
            address+=Inode->addr[1];
        if (Inode->addr[2]!=-1)
            address+=Inode->addr[2];
        if (Inode->addr[3]!=-1)
            address+=Inode->addr[3];
        //移动文件指针
        fseek(file,address*512+super_block->first_blk*512,SEEK_SET);
        //读出该数据块
        struct Data_block*data=malloc(sizeof(struct Data_block));
        struct directory_entry*dir=malloc(sizeof(struct directory_entry));
        fread(data,sizeof(struct Data_block),1,file);
        dir=(struct directory_entry*)data;
        for (int i=0;i<32;++i)
        {
            if (dir->filename[0]=='\0')
            {
                //找到空的目录项，则把该文件信息添加到该目录项中
                //如果flag=1表明此时有扩展名
                if (flag==1)
                {
                    strcpy(dir->filename,fname);
                    strcpy(dir->extension,fextension);
                }
                else
                    strcpy(dir->filename,fname);
                //把该目录项写进磁盘,即更新目录项
                fseek(file,address*512+super_block->first_blk*512+i*16,SEEK_SET);
                fwrite(dir,sizeof(struct directory_entry),1,file);
                break;
            }
            dir++;
        }
    } 
    //如果是一级间接寻址
    else if (Inode->addr[4]!=-1)
    {
        //移动文件指针到一级间接数据块
        fseek(file,Inode->addr[4]*512+super_block->first_blk*512,SEEK_SET);
        //读出该数据块的内容
        struct Data_block*first=malloc(sizeof(struct Data_block));
        fread(first,sizeof(struct Data_block),1,file);
        //移动文件指针到目录项所在数据块
        struct Data_block*direct=malloc(sizeof(struct Data_block));
        struct directory_entry* dir=malloc(sizeof(struct directory_entry));
        fseek(file,first->next*512+super_block->first_blk*512,SEEK_SET);
        //将目录项读出来
        fread(direct,sizeof(struct Data_block),1,file);
        dir=(struct directory_entry*)direct;
        //找到空目录项
        for (int i=0;i<32;++i)
        {
            if (dir->filename[0]!='\0')
            {
                //如果找到空的目录项，则填写相关信息
                //如果flag=1表明此时有扩展名
                if (flag==1)
                {
                    strcpy(dir->filename,fname);
                    strcpy(dir->extension,fextension);
                }
                else
                    strcpy(dir->filename,fname);
                //把该目录项写进磁盘,即更新目录项
                fseek(file,first->next*512+super_block->first_blk*512+i*16,SEEK_SET);
                fwrite(dir,sizeof(struct directory_entry),1,file);
                break;
            }
            dir++;
        }
    }
    //如果是二级间接寻址
    else if (Inode->addr[5]!=-1)
    {
        //移动文件指针到二级间接数据块
        fseek(file,Inode->addr[5]*512+super_block->first_blk*512,SEEK_SET);
        //将二级间接数据块读出来
        struct Data_block*second=malloc(sizeof(struct Data_block));
        fread(second,sizeof(struct Data_block),1,file);
        //将文件指针移动到一级间接数据块
        fseek(file,second->next*512+super_block->first_blk*512,SEEK_SET);
        //将一级间接数据块读出来
        struct Data_block*first=malloc(sizeof(struct Data_block));
        fread(first,sizeof(struct Data_block),1,file);
        //移动到目录项所在的数据块中
        fseek(file,first->next*512+super_block->first_blk*512,SEEK_SET);
        struct Data_block* direct=malloc(sizeof(struct Data_block));
        struct directory_entry*dir=malloc(sizeof(struct directory_entry));
        //将目录项读出来
        fread(direct,sizeof(struct Data_block),1,file);
        dir=(struct directory_entry*)direct;
        //找到空的目录项，填写相关信息，并把该目录项写入磁盘中
        for (int i=0;i<32;++i)
        {   
            //如果找到空的目录项
            if (dir->filename[0]!='\0')
            {
                //如果找到空的目录项，则填写相关信息
                //如果flag=1表明此时有扩展名
                if (flag==1)
                {
                    strcpy(dir->filename,fname);
                    strcpy(dir->extension,fextension);
                }
                else
                    strcpy(dir->filename,fname);
                //把该目录项写进磁盘,即更新目录项
                fseek(file,first->next*512+super_block->first_blk*512+i*16,SEEK_SET);
                fwrite(dir,sizeof(struct directory_entry),1,file);
                break;
            }
            dir++;
        }
    }
    //如果是三级间接寻址
    else if (Inode->addr[6]!=-1)
    {
        //移动文件指针到三级间接数据块
        fseek(file,Inode->addr[6]*512+super_block->first_blk*512,SEEK_SET);
        //读出三级间接数据块
        struct Data_block*third=malloc(sizeof(struct Data_block));
        fread(third,sizeof(struct Data_block),1,file);
        //移动文件指针到二级间接数据块
        fseek(file,third->next*512+super_block->first_blk*512,SEEK_SET);
        //读出二级间接数据块
        struct Data_block*second=malloc(sizeof(struct Data_block));
        fread(second,sizeof(struct Data_block),1,file);
        //移动文件指针到一级间接数据块
        fseek(file,second->next*512+super_block->first_blk*512,SEEK_SET);
        //读出一级间接数据块
        struct Data_block*first=malloc(sizeof(struct Data_block));
        fread(first,sizeof(struct Data_block),1,file);
        //移动文件指针到目录项所在的数据块
        fseek(file,first->next*512+super_block->first_blk*512,SEEK_SET);
        //找到空目录项
        struct Data_block*direct=malloc(sizeof(struct Data_block));
        struct directory_entry*dir=malloc(sizeof(struct directory_entry));
        fread(direct,sizeof(struct Data_block),1,file);
        dir=(struct Data_block*)direct;
        //找到空的目录项，填写相关信息，并把该目录项写入磁盘中
        for (int i=0;i<32;++i)
        {   
            //如果找到空的目录项
            if (dir->filename[0]!='\0')
            {
                //如果找到空的目录项，则填写相关信息
                //如果flag=1表明此时有扩展名
                if (flag==1)
                {
                    strcpy(dir->filename,fname);
                    strcpy(dir->extension,fextension);
                }
                else
                    strcpy(dir->filename,fname);
                //把该目录项写进磁盘,即更新目录项
                fseek(file,first->next*512+super_block->first_blk*512+i*16,SEEK_SET);
                fwrite(dir,sizeof(struct directory_entry),1,file);
                break;
            }
            dir++;
        }
    }
    return res;
}
// 此函数应将新目录添加到根级别，并应更新 .directory 文件
// 成功时为 0
// -ENAMETOOLONG，如果名称超过 8 个字符
// -EPERM 如果目录不在根目录下
// -EEXIST（如果目录已存在）
static int SFS_mkdir(const char*path,mode_t mode)
{   
    int res=0;
    printf("正在调用SFS_mkdir函数!\n");
    //首先判断目录是否存在
    FILE*temp=fopen(path,"r+");
    //如果temp不是NULL，则说明该目录已经存在
    if (temp!=NULL)
    {
        printf("该目录已存在!n");
        //关闭文件
        fclose(temp);
        //返回错误
        return -EEXIST;
    }
    //关闭文件
    fclose(temp);
    //再判断目录是否创建在根目录下
    char*filepath=malloc(strlen(path)+1);
    char*fname=malloc(strlen(path)+1);
    strcpy(filepath,path);
    //对filepath分割两次后，如果返回值不为NULL则说明该目录不是创建在根目录下
    filepath=strtok(filepath,"/");
    //将filepath保留一份
    strcpy(fname,filepath);
    filepath=strtok(filepath,"/");
    if (filepath!=NULL)
    {
        printf("目录不在根目录下!\n");
        //释放内存
        free(filepath);
        free(fname);
        //返回错误
        return -EPERM;
    }
    //再判断名称是否超过8个字符
    if (strlen(fname)>8)
    {
        printf("目录名称超过8个字符!\n");
        //释放内存
        free(filepath);
        free(fname);
        //返回错误
        return -ENAMETOOLONG;
    }
    //打开磁盘并读出超级块
    FILE *file = fopen("/home/lei/Desktop/libfuse-master/sfs/filesystem", "r+");
    struct sb*super_block=malloc(sizeof(struct sb));
    fread(super_block,sizeof(struct sb),1,file);
    //读出inode位图
    int *inodeBitmap=malloc(sizeof(int)*128);
    fseek(file,super_block->fisrt_blk_of_inodebitmap*512,SEEK_SET);
    fread(inodeBitmap,sizeof(int)*128,1,file);
    //首先在根目录的目录项添加新增加的目录
    //读出根目录的inode
    struct inode*Inode=malloc(sizeof(struct inode));
    //移动文件指针到inode所在位置
    fseek(file,super_block->first_inode*512,SEEK_SET);
    fread(Inode,sizeof(struct inode),1,file);
    //找到对应的数据块
    struct Data_block*data=malloc(sizeof(struct Data_block));
    //移动文件指针
    fseek(file,super_block->first_blk*512+Inode->addr[0]*512,SEEK_SET);
    //读出根目录目录项
    fread(data,sizeof(struct Data_block),1,file);
    struct directory_entry*dir=malloc(sizeof(struct directory_entry));
    dir=(struct directory_entry*)data;
    //再找到一个空的inode
    int inode=allocateInode(inodeBitmap);
    //如果找不到空闲inode，则返回错误
    if (inode==-1)
    {
        printf("找不到空闲Inode!\n");
        //返回错误
        return -errno;
    }
    //找到空的目录项
    for (int i=0;i<32;++i)
    {   
        //如果找到空的目录项
        if (dir->filename[0]!='\0')
        {
            strcpy(dir->filename,fname);
            dir->f_inode=inode;
            //将目录项写进磁盘
            fseek(file,super_block->first_blk*512+16*i,SEEK_SET);
            fwrite(dir,sizeof(struct directory_entry),1,file);
            break;
        }
        dir++;
    }
    struct inode*new_inode=malloc(sizeof(struct inode));
    //更新该inode
    new_inode->st_gid=getgid();
    new_inode->st_uid=getuid();
    new_inode->st_ino=inode;
    new_inode->st_nlink=2;
    timespec_get(&new_inode->st_atim,TIME_UTC);
    new_inode->st_mode=S_IRUSR|S_IWUSR|S_IXUSR|__S_IFDIR;
    new_inode->st_size=0;
    //读出数据块位图
    int *dataBitmap=malloc(sizeof(int)*512);
    fseek(file,super_block->first_blk_of_databitmap*512,SEEK_SET);
    fread(dataBitmap,sizeof(int)*512,1,file);
    //找到一个空的数据块
    int new_data=allocateData(dataBitmap);
    //如果找不到空闲数据块
    if (new_data==-1)
    {
        printf("找不到空闲数据块!\n");
        return ENOSPC;
    }
    //一共有15866个数据块
    //假设直接数据块用866个 一级用1000个 二级用5000个 三级用9000
    //根据不同的数据块号选择不同的选址方式
    //直接选址
    if (new_data<866)
    {
        memset(new_inode->addr,-1,7);
        new_inode->addr[0]=new_data;
        //更新数据块内容
        //移动文件指针到直接数据块
        fseek(file,super_block->first_blk+new_data*512,SEEK_SET);
        struct directory_entry*dir1=malloc(sizeof(struct directory_entry));
        //为新增的目录的每个目录项赋初值，并写入磁盘里
        for(int i=0;i<32;++i)
        {
            dir1->extension[0]='\0';
            dir1->f_inode=-1;
            dir1->filename[0]='\0';
            dir1->spare[0]='\0';
            //将文件信息写入磁盘中
            fwrite(dir1,sizeof(struct directory_entry),1,file);
            dir1++;
        }
    }
    //一次间接寻址
    else if (new_data<1866)
    {
        memset(new_inode->addr,-1,7);
        new_inode->addr[4]=new_data;
        //该数据块为一级间接数据块
        struct Data_block*first=malloc(sizeof(struct Data_block));
        //再找一个空闲的数据块
        int first_blk=allocateData(dataBitmap);
        //如果找不到空闲的数据块，则返回错误
        if (first_blk==-1)
        {
            printf("找不到空闲数据块!\n");
            return ENOSPC;
        }
        first->next=first_blk;
        //将下列设置为0表明该数据块是间接数据块
        first->buffer[0]='\0';
        first->data_size=0;
        //移动文件指针
        fseek(file,super_block->first_blk*512+new_data*512,SEEK_SET);
        //把该数据块写入磁盘
        fwrite(first,sizeof(struct Data_block),1,file);
        //移动文件指针到直接数据块
        fseek(file,first_blk*512+super_block->first_blk*512,SEEK_SET);
        struct directory_entry*dir1=malloc(sizeof(struct directory_entry));
        //为新增的目录的每个目录项赋初值，并写入磁盘里
        for(int i=0;i<32;++i)
        {
            dir1->extension[0]='\0';
            dir1->f_inode=-1;
            dir1->filename[0]='\0';
            dir1->spare[0]='\0';
            //将文件信息写入磁盘中
            fwrite(dir1,sizeof(struct directory_entry),1,file);
            dir1++;
        }
    }
    //二次间接寻址
    else if (new_data<6866)
    {
        memset(new_inode->addr,-1,7);
        new_inode->addr[5]=new_data;
        //该数据块为二次间接数据块
        struct Data_block*second=malloc(sizeof(struct Data_block));
        //再找一个新的数据块
        int second_blk=allocateData(dataBitmap);
        //如果找不到新的数据块，则返回错误
        if (second_blk==-1)
        {
            printf("找不到空闲数据块!\n");
            return ENOSPC;
        }
        second->next=second_blk;
        //将下列值赋值为0表明该数据块是间接数据块
        second->data_size=0;
        second->buffer[0]='\0';
        //移动文件指针
        fseek(file,super_block->first_blk*512+new_data*512,SEEK_SET);
        //把二级间接数据块写入文件中
        fwrite(second,sizeof(struct Data_block),1,file);
        //该数据块为一级间接数据块
        struct Data_block*first=malloc(sizeof(struct Data_block));
        //再找一个新的数据块
        int first_blk=allocateData(dataBitmap);
        //如果找不到新的数据块，则返回错误
        if (first_blk==-1)
        {
            printf("找不到空闲数据块!\n");
            return ENOSPC;
        }
        first->next=first_blk;
        //将下列值赋值为0表明该数据块是间接数据块
        first->buffer[0]='\0';
        first->data_size=0;
        //再次移动文件指针
        fseek(file,super_block->first_blk*512+second_blk*512,SEEK_SET);
        //把一级数据块写入磁盘
        fwrite(first,sizeof(struct Data_block),1,file);
        //移动文件指针到直接数据块
        fseek(file,super_block->first_blk*512+first_blk*512,SEEK_SET);
        struct directory_entry*dir1=malloc(sizeof(struct directory_entry));
        //为新增的目录的每个目录项赋初值，并写入磁盘里
        for(int i=0;i<32;++i)
        {
            dir1->extension[0]='\0';
            dir1->f_inode=-1;
            dir1->filename[0]='\0';
            dir1->spare[0]='\0';
            //将文件信息写入磁盘中
            fwrite(dir1,sizeof(struct directory_entry),1,file);
            dir1++;
        }
    }
    //三次间接寻址
    else
    {
        memset(new_inode->addr,-1,7);
        new_inode->addr[6]=new_data;
        //找到一个新的数据块
        int third_blk=allocateData(dataBitmap);
        //如果找不到新的数据块，则返回错误
        if (third_blk==-1)
        {
            printf("找不到空闲数据块!\n");
            return ENOSPC;
        }
        //该数据块为三次间接数据块
        struct Data_block*third=malloc(sizeof(struct Data_block));
        third->next=third_blk;
        //将下列值赋值为0，表明该数据块为间接数据块
        third->buffer[0]='\0';
        third->data_size=0;
        //移动文件指针
        fseek(file,new_data*512+super_block->first_blk*512,SEEK_SET);
        //将三级间接数据块写入磁盘中
        fwrite(third,sizeof(struct Data_block),1,file);
        //再找一个新的数据块
        int second_blk=allocateData(dataBitmap);
        //如果找不到新的数据块，则返回错误
        if (second_blk==-1)
        {
            printf("找不到空闲数据块!\n");
            return ENOSPC;
        }
        //该数据块为二次间接数据块
        struct Data_block*second=malloc(sizeof(struct Data_block));
        second->next=second_blk;
        //将下列值赋值为0，表明该数据块为间接数据块
        second->data_size=0;
        second->buffer[0]='\0';
        //移动文件指针
        fseek(file,third_blk*512+super_block->first_blk*512,SEEK_SET);
        //将二级间接数据块写入磁盘
        fwrite(second,sizeof(struct Data_block),1,file);
        //再找一个新的数据块
        int first_blk=allocateData(dataBitmap);
        //如果找不到新的数据块，则返回错误
        if (first_blk==-1)
        {
            printf("找不到空闲数据块!\n");
            return ENOSPC;
        }
        //该数据块为一次间接数据块
        struct Data_block*first=malloc(sizeof(struct Data_block));
        first->next=first_blk;
        //将下列值赋值为0，表明该数据块为间接数据块
        first->data_size=0;
        first->buffer[0]='\0';
        //移动文件指针
        fseek(file,second_blk*512+super_block->first_blk*512,SEEK_SET);
        //将一级间接块写入磁盘
        fwrite(first,sizeof(struct Data_block),1,file);
    }
    //将文件指针移动到新找到的inode处并更新该inode
    fseek(file,super_block->first_inode*512+inode*64,SEEK_SET);
    //把该inode写进磁盘中
    fwrite(new_inode,sizeof(struct inode),1,file);
    //成功则返回0
    return res;
}

// 删除空目录
// 0 阅读成功
// -ENOTEMPTY 如果目录不为空
// -ENOENT 如果未找到目录
// -ENOTDIR 如果路径不是目录
static int SFS_rmdir(const char*path)
{
    int res=0;
    //打印函数调用消息
    printf("正在调用SFS_rmdir函数!\n");
    //首先看目录是否能够打开
    FILE *temp=open(path,"r+");
    //如果目录返回为NULL，说明没有该目录，返回错误
    if (temp==NULL)
    {
        //打印错误信息
        printf("未找到该目录!\n");
        //关闭文件
        fclose(temp);
        return -ENOENT;
    }
    fclose(temp);
    //根据路径找到对应的inode
    int inode=path_to_inode(path);
    //打开磁盘读出超级块
    FILE *file = fopen("/home/lei/Desktop/libfuse-master/sfs/filesystem", "r+");
    struct sb*super_block=malloc(sizeof(struct sb));
    //读出超级块
    fread(super_block,sizeof(struct sb),1,file);
    //移动文件指针到所删除目录的inode区
    fseek(file,super_block->first_inode*512+inode*64,SEEK_SET);
    //读出所对应的inode
    struct inode*Inode=malloc(sizeof(struct inode));
    fread(inode,sizeof(struct inode),1,file);
    //先判断删除的是否为目录
    //如果相与为0，则说明不是目录，返回错误
    if (Inode->st_mode&__S_IFDIR==0)
    {   
        //打印错误信息
        printf("所删除的路径不是目录!\n");
        //释放内存
        free(super_block);
        free(Inode);
        //返回错误
        return -ENOTDIR;
    }
    //接下来判断目录是否为空
    //如果是直接寻址
    if (Inode->addr[0]!=-1)
    {   
        int  address=0;
        address+= Inode->addr[0];
        if (Inode->addr[1]!=-1)
            address+=Inode->addr[1];
        if (Inode->addr[2]!=-1)
            address+=Inode->addr[2];
        if (Inode->addr[3]!=-1)
            address+=Inode->addr[3];
        //移动文件指针到直接数据块
        fseek(file,super_block->first_blk*512+address*512,SEEK_SET);
        //读出直接数据块
        struct Data_block*data=malloc(sizeof(struct Data_block));
        fread(data,sizeof(struct Data_block),1,file);
        struct directory_entry*dir=malloc(sizeof(struct directory_entry));
        dir=(struct directory_entry*)data;
        //检查目录是否为空
        int flag=0;
        for (int i=0;i<32;++i)
        {
            if (dir->filename[0]!=0)
                break;
            flag++;
            dir++;
        }
        //如果flag不等于32 说明该目录非空，返回错误
        if (flag!=32)
        {
            //打印错误信息
            printf("该目录非空!\n");
            //释放内存
            free(super_block);
            free(Inode);
            free(data);
            free(dir);
            //返回错误
            return -ENOTEMPTY;
        }
    }
    //一级间接寻址
    else if (Inode->addr[4]!=-1)
    {
        //移动文件指针到一级间接数据块
        fseek(file,super_block->first_blk*512+Inode->addr[4]*512,SEEK_SET);
        //读出一级间接数据块
        struct Data_block*first=malloc(sizeof(struct Data_block));
        fread(first,sizeof(struct Data_block),1,file);
        //移动文件指针到直接数据块
        fseek(file,super_block->first_blk*512+first->next*512,SEEK_SET);
        //读出直接数据块
        struct Data_block*data=malloc(sizeof(struct Data_block));

    }
    //二级间接寻址s
    else if (Inode->addr[5]!=-1)
    {

    }
    //三级间接寻址
    else if (Inode->addr[6]!=-1)
    {

    }
}