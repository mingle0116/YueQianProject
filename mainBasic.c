#include"stdio.h"
#include"sys/types.h"
#include"fcntl.h"
#include"unistd.h"
#include"linux/input.h"
#include"stdlib.h"
#include"sys/mman.h"
#include"string.h"

#define LCD_HEIGHT 480
#define LCD_WIDTH 800

int lcd; //lcd 的文件描述符
int ts_fd; //touch 的文件描述符
int * mmap_start; //lcd 映射内存
int touch_x,touch_y ;//存放存放坐标
int x,y;
int flag;//操作的动作 标识符 left:1 right:2
struct input_event buf; //触摸屏的结构体
int num_images=8;//相册的图片数
int current_image_index = 0; // 用于跟踪当前显示的图片索引
char *image_paths[] = {
    "/AlbumProject/image/mimiA.bmp", 
    "/AlbumProject/image/pandaC.bmp",
    "/AlbumProject/image/dogA.bmp", 
    "/AlbumProject/image/tigerD.bmp",
    "/AlbumProject/image/faguoA.bmp",
    "/AlbumProject/image/duolaB.bmp",
    "/AlbumProject/image/kgrA.bmp", 
    "/AlbumProject/image/rabbitA.bmp"
};

//函数声明
int init(); //初始化函数，负责
int show_bmp(char * bmp_path);
int get_xy();//获取坐标，判断左划还是右划，返回left:1 right:2 exit:0
int get_images(char * image_path[], int flag);

//主函数
int main(){

    init();

     while(1)
    {          
       flag=get_xy();
    //    printf("%d---flag\n",flag);
       get_images(image_paths,flag);

       if(flag==0){
         printf("bye,have a good day!\n");
         break;
       }

    }
    close(ts_fd);
    close(lcd);
    munmap(mmap_start,LCD_WIDTH*LCD_HEIGHT*4);
    return 0;
}






int init(){

   lcd = open("/dev/fb0", O_RDWR);
   ts_fd=open("/dev/input/event0", O_RDONLY);
   printf("lcd----(%d)------ts_fd-------%d\n",lcd,ts_fd);
   if(lcd==-1){
    printf("open lcd file error.\n");
    return -1;
   }else{
     printf("open lcd file success\n");
   }
   if(ts_fd==-1){
    printf("open event0 file error\n");
    return -2;
   }else{
     printf("open event0 file success\n");
   }

   mmap_start=(int *)mmap(NULL,LCD_HEIGHT*LCD_WIDTH*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcd,0);
   if (mmap_start==MAP_FAILED)
   {
    printf("内存映射失败\n");
    return -3;
   }
   
   return 0;

}
int show_bmp(char * bmp_path){
//第一步：打开lcd的设备文件
   
    int picture=open(bmp_path,O_RDONLY);
    
     if(picture == -1)
    {
        printf("open picture error!\n");
        return -1;
    }
    else
    {
        printf("open picture success!\n");
    }
  
    int bmp_w,bmp_h;
    lseek(picture,18,SEEK_SET);//跳到19前面
    read(picture,&bmp_w,4);
    read(picture,&bmp_h,4);
    //判断图片的大小
    if(bmp_h>480||bmp_w>800){
        printf("图片尺寸大于屏幕尺寸，没办法完整显示出来，抱歉。麻烦你换张图片\n");
        return -2;
    }

    int num;//存放每行需要补的字节数
    if(bmp_w*3%4==0){
        num=0;
    }else{
        num=4-(bmp_w*3%4);
    }

    int max=bmp_w*bmp_h*3+num*bmp_h;
    char picBuf[max];   //char 类型的
    lseek(picture,54,SEEK_SET);

    int read_ret=read(picture,picBuf,sizeof(picBuf));

    if(read_ret == -1){
        printf("write lcd error!\n");
        return -3;
    }
    else
    {
        printf("write lcd success!\n");
    }

    // int k;
    // for(k=0;k<800*480;k++){
    //      *(mmap_start+k) =0xffffffff ;
    // }


    //改造像素点
    int n,y,x;
    for(y=0,n=0;y<bmp_h;y++){
        for( x=0;x<bmp_w;x++,n+=3){
                      
                  *(mmap_start+800*(479-y)+x) = (picBuf[n]<<0) + (picBuf[n+1]<<8) + (picBuf[n+2]<<16);
                   
            }            
             n+=num;  
    }
    
    //第三步：关闭lcd的设备文件

    close(picture);
    return 0;

}

int get_xy()
{
    struct input_event buf;
    int x1, y1;       // 保存第一次的坐标
    int x2, y2;       // 保存最后一次的坐标
    int tmp_x, tmp_y; // 保存临时坐标
    x1 = y1 = x2 = y2 = -1;
    while (1)
    {
        read(ts_fd, &buf, sizeof(buf)); // 读取触摸屏数据
        if (buf.type == EV_ABS)         // 触摸屏事件
        {
            if (buf.code == ABS_X) // X轴
            {
                tmp_x = buf.value*800/1024;
            }
            else if (buf.code == ABS_Y) // Y轴
            {
                tmp_y = buf.value*480/600;
            }
        }
        // 判断手指松开
        if (buf.type == EV_KEY && buf.code == BTN_TOUCH)
        {
            if (buf.value == 1) // 按下
            {
                x1 = tmp_x;
                y1 = tmp_y;
            }
            else if (buf.value == 0) // 松开,保存最后一次坐标
            {
                x2 = tmp_x;
                y2 = tmp_y;
                break;
            }
            


        }
    }

    int var_x = abs(x2 - x1); // x轴变化量
    int var_y = abs(y2 - y1); // y轴变化量
    // 判断是滑动还是点击
    if ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) >= 200) // 滑动
    {
        if (x1 > x2 && var_x > var_y) // 左
        {
            printf("left\n");
            return 1;

        }
        else if (x1 < x2 && var_x > var_y) // 右
        {
            printf("right\n");
            return 2;

        }
    }
    //退出  (右上角（100*100）退出)
    if (x1>=700&&x1<800&&y1>=0&&y1<100)
    {
        printf("bye\n");
        return 0;
    }
}

int get_images(char * image_path[], int flag)
{

    //int num_images = sizeof(image_paths) / sizeof(image_paths[0]);   每个路径的字节数不同，不能通过均分来计算图片数量
    // int current_image_index = 0; // 用于跟踪当前显示的图片索引
    // 如果flag=2，滑向前一张图片(右划)
    if(flag==2||flag==1||flag==0){
        memset(mmap_start,0,LCD_WIDTH*LCD_HEIGHT*4);
        if (flag == 2)
        {
            current_image_index--;
            if (current_image_index < 0)
            {
                current_image_index = num_images - 1; // 如果到达第一张图片，循环显示最后一张
            }
        }
    // 如果flag=1，滑向后一张图片
        else if (flag == 1)
        {
            current_image_index++;
            if (current_image_index >= num_images)
            {
                current_image_index = 0; // 如果到达最后一张图片，循环显示第一张
            }
        }else if(flag==0){
            printf("谢谢使用，退出相册\n");
            return 0;
        }
        char *current_image_path = image_paths[current_image_index];
        show_bmp(current_image_path);
    }
   

    

   
}


