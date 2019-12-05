#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iconv.h>

#define QQWRY "qqwry.dat"
#define REDIRECT_MODE_1 0x01
#define REDIRECT_MODE_2 0x02
#define MAXBUF 255

/*unsigned long getValue( 获取文件中指定的16进制串的值，并返回
        FILE *fp, 指定文件指针
        unsigned long start, 指定文件偏移量
        int length) 获取的16进制字符个数/长度
*/
static unsigned long getValue(FILE *fp, unsigned long start, int length)
{
        unsigned long variable=0;
        long val[length],i;

        fseek(fp,start,SEEK_SET);
        for(i=0;i<length;i++)
        {
                /*过滤高位，一次读取一个字符*/
                val[i]=fgetc(fp)&0x000000FF;
        } 
        for(i=length-1;i>=0;i--)
        {
                /*因为读取多个16进制字符，叠加*/
                variable=(variable << 8) +val[i];
        } 
        return variable;
};

static int code_convert(char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
        iconv_t cd;
        char **pin = &inbuf;
        char **pout = &outbuf;
 
        cd = iconv_open("UTF-8","GB2312");
        if (cd==0)
                return -1;
        memset(outbuf,0,outlen);
        if (iconv(cd,pin,&inlen,pout,&outlen) < 0)
                return -1;
        iconv_close(cd);
        return 0;
}

/*int getString( 获取文件中指定的字符串，返回字符串长度
        FILE *fp, 指定文件指针
        unsigned long start, 指定文件偏移量
        char **string) 用来存放将读取字符串的字符串空间的首地址
*/
static int getString(FILE *fp, unsigned long start, char **string)
{
        unsigned int i=0;
        unsigned char val;
        fseek(fp,start,SEEK_SET);
        /*读取字符串，直到遇到0x00为止*/
        do
        {
                val=fgetc(fp);
                /*依次放入用来存储的字符串空间中*/
                *(*string+i)=val;
                i++;
        }while(val!=0x00);
        /*返回字符串长度*/
        return i;
};


/*void getAddress( 读取指定IP的国家位置和地域位置
        FILE *fp, 指定文件指针
        unsigned long start, 指定IP在索引中的文件偏移量
        char **country, 用来存放国家位置的字符串空间的首地址
        char **location) 用来存放地域位置的字符串空间的首地址
*/
static void getAddress(FILE *fp, unsigned long start, char **country, char **location)
{
        unsigned long redirect_address,counrty_address,location_address;
        char val;
        
        start+=4;
        fseek(fp,start,SEEK_SET);
        /*读取首地址的值*/
        val=(fgetc(fp)&0x000000FF);

        if(val==REDIRECT_MODE_1)
        {
                /*重定向1类型的*/
                redirect_address=getValue(fp,start+1,3);
                fseek(fp,redirect_address,SEEK_SET);
                /*混合类型，重定向1类型进入后遇到重定向2类型
                  读取重定向后的内容，并设置地域位置的文件偏移量*/
                if((fgetc(fp)&0x000000FF)==REDIRECT_MODE_2)
                {
                        counrty_address=getValue(fp,redirect_address+1,3);
                        location_address=redirect_address+4;
                        getString(fp,counrty_address,country);
                }
                /*读取重定向1后的内容，并设置地域位置的文件偏移量*/
                else
                {
                        counrty_address=redirect_address;
                        location_address=redirect_address+getString(fp,counrty_address,country);
                }
        }
        /*重定向2类型的*/
        else if(val==REDIRECT_MODE_2)
        {
                counrty_address=getValue(fp,start+1,3);
                location_address=start+4;
                getString(fp,counrty_address,country);
        }
        else
        {
                counrty_address=start;
                location_address=counrty_address+getString(fp,counrty_address,country);
        }
        
        /*读取地域位置*/
        fseek(fp,location_address,SEEK_SET);
        if((fgetc(fp)&0x000000FF)==REDIRECT_MODE_2||(fgetc(fp)&0x000000FF)==REDIRECT_MODE_1)
        {
        location_address=getValue(fp,location_address+1,3);
        }
        getString(fp,location_address,location);

        return;
};


/*void getHead( 读取索引部分的范围（在文件头中，最先的2个8位16进制）
        FILE *fp, 指定文件指针
        unsigned long *start, 文件偏移量，索引的起止位置
        unsigned long *end) 文件偏移量，索引的结束位置
*/
static void getHead(FILE *fp,unsigned long *start,unsigned long *end)
{
        /*索引的起止位置的文件偏移量，存储在文件头中的前8个16进制中
          设置偏移量为0，读取4个字符*/
        *start=getValue(fp,0L,4);
        /*索引的结束位置的文件偏移量，存储在文件头中的第8个到第15个的16进制中
          设置偏移量为4个字符，再读取4个字符*/
        *end=getValue(fp,4L,4);
};


/*unsigned long searchIP( 搜索指定IP在索引区的位置，采用二分查找法；
             返回IP在索引区域的文件偏移量
             一条索引记录的结果是，前4个16进制表示起始IP地址
             后面3个16进制，表示该起始IP在IP信息段中的位置，文件偏移量
        FILE *fp,
        unsigned long index_start, 索引起始位置的文件偏移量
        unsigned long index_end, 索引结束位置的文件偏移量
        unsigned long ip) 关键字，要索引的IP
*/
static unsigned long searchIP(FILE *fp, unsigned long index_start, \

            unsigned long index_end, unsigned long ip)
{
        unsigned long index_current,index_top,index_bottom;
        unsigned long record;
        index_bottom=index_start;
        index_top=index_end;
        /*此处的7，是因为一条索引记录的长度是7*/
        index_current=((index_top-index_bottom)/7/2)*7+index_bottom;
        /*二分查找法*/
        do{
                record=getValue(fp,index_current,4);
                if(record>ip)
                {
                        index_top=index_current;
                        index_current=((index_top-index_bottom)/14)*7+index_bottom;
                }
                else
                {
                        index_bottom=index_current;
                        index_current=((index_top-index_bottom)/14)*7+index_bottom;
                }
        }while(index_bottom<index_current);
        /*返回关键字IP在索引区域的文件偏移量*/
        return index_current;
};

/*判断一个字符是否为数字字符，
  如果是，返回0
  如果不是，返回1*/
static int beNumber(char c)
{
        if(c>='0'&&c<='9')
                return 0;
        else
                return 1;
};


/*函数的参数是一个存储着IP地址的字符串首地址
  返回该IP的16进制代码
  如果输入的IP地址有错误，函数将返回0*/
static unsigned long getIP(char *ip_addr)
{
        unsigned long ip=0;
        unsigned long i,j=0;
        /*依次读取字符串中的各个字符*/
        for(i=0;i<strlen(ip_addr);i++)
        {
                /*如果是IP地址间隔的‘.’符号
                  把当前读取到的IP字段的值，存入ip变量中
                  （注意，ip为叠加时，乘以16进制的0x100）
                  并清除临时变量的值*/
                if(*(ip_addr+i)=='.')
                {
                        ip=ip*0x100+j;
                        j=0;
                }
                /*往临时变量中写入当前读取到的IP字段中的字符值
                  叠加乘以10，因为输入的IP地址是10进制*/
                else
                {
                        /*判断，如果输入的IP地址不规范，不是10进制字符
                          函数将返回0*/
                        if(beNumber(*(ip_addr+i))==0)
                                j=j*10+*(ip_addr+i)-'0';
                        else
                                return 0;
                }
        }
        /*IP字段有4个，但是‘.’只有3个，叠加第四个字段值*/
        ip=ip*0x100+j;
        return ip;
};

static void getLocation(char *ip_addr, char *location_addr)
{
    FILE *fp; 
    unsigned long index_start,index_end,current;
    if((fp=fopen(QQWRY,"rb"))==NULL)
    {
        printf("[-] Error : Can not open the file %s.\n",QQWRY);
        return;
    }
    unsigned int ip;
    ip=getIP(ip_addr);
    if(ip==0)
    {
        printf("[-] Error : the IP Address inputed.\n");
        return;
    } 
    getHead(fp, &index_start, &index_end);
    current=searchIP(fp,index_start, index_end, ip);

    char *ori_country;
    char *ori_location;
    ori_country=(char*)malloc(MAXBUF);
    ori_location=(char*)malloc(MAXBUF);
    char *country;
    char *location;
    country=(char*)malloc(MAXBUF);
    location=(char*)malloc(MAXBUF);

    getAddress(fp,getValue(fp,current+4,3),&ori_country,&ori_location);
    code_convert(ori_country, MAXBUF, country, MAXBUF);
    code_convert(ori_location, MAXBUF, location, MAXBUF);
    strcat(country, location);
    strcpy(location_addr, country);

    fclose(fp);
    free(ori_country);
    free(ori_location);
    free(country);
    free(location);
}