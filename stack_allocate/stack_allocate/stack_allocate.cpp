// stack_allocate.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "io.h"
#include "string.h"

//#pragma  comment(  linker,  "/subsystem:\"windows\"  /entry:\"wmainCRTStartup\""  )

#define RAM_ID_HEAD (0x353ab756)
#define RAM_ID_TAIL (0xacb8562d)

#define ROM_ID_HEAD (0x38ab1ef0)
#define ROM_ID_TAIL (0x6184abe6)

static unsigned int mark[4]  = {RAM_ID_HEAD,RAM_ID_TAIL,ROM_ID_HEAD,ROM_ID_TAIL};

static unsigned int endfile_t[3] = {0xf1f2f3f4,0xe1e2e3e4,0xd1d2d3d4};

//const char *to_search="..\\d200_drone\\opk\\*ok"; 
static char name_buffer[20][200];//0x420000(rom addr) , 0x20400000(ram addr) , output path ,control

static unsigned int base_rom_addr = 0;
static unsigned int base_ram_addr = 0;
static unsigned int current_allocate_rom_addr = 0;
static unsigned int current_allocate_ram_addr = 0;

static unsigned int cnt = 0;
static unsigned int enter_cnt = 0;
static char config = 0;
static unsigned char flag_bh = 1;

int stack_rom_allocate(char *file_name,char *);
void arom_write(unsigned int * data);

FILE *fp_ok_read;
FILE *fp_create;

int Tchar_to_char(_TCHAR * tchar,char * buffer)
{  
    int i = 0;
	memset(buffer,0,200);

	while(*tchar != '\0')
	{
       buffer[i] = *tchar;
	   tchar ++;
	   i++;
	}
	return i;
}

int _tmain(int argc, _TCHAR* argv[])
{
	/* init */
	cnt = 0;
	enter_cnt = 0;
    /* get base addr */
    
    if(argc != 6)
	{
		printf("stack allocate fail,only accept 5 params , now is %d\r\n",argc - 1);
		return (-1);
	}

    for(int i = 1;i<argc;i++)
	{
       Tchar_to_char(argv[i],name_buffer[i-1]);
	}

    if(sscanf(name_buffer[0],"0x%x",&base_rom_addr) != 1)
	{
		printf("can not transfer base rom addr\n");
		return 0;
	}

	printf("base rom:0x%08x\n",base_rom_addr);

    if(sscanf(name_buffer[1],"0x%x",&base_ram_addr) != 1)
	{
		printf("can not transfer base ram addr\n");
		return 0;
	}

    if(name_buffer[3][0] == '-' && name_buffer[3][1] == 'b')
	{
		//bin file
		config = 0;
	}else if(name_buffer[3][0] == '-' && name_buffer[3][1] == 'h')
	{
       //head file
		config = 1;
	}else
	{
      //unsupply param
		printf("unsupply param %s\n", name_buffer[3]);
		return 0;
	}
	/*-----------------------------------*/
	printf("ver : 0.1.5_AT_201809190959\r\n");
	printf("base ram : 0x%08x\n",base_ram_addr);
    printf("out path : %s\n",name_buffer[2]);
	printf("config : %s \n",config?".h":".b");
	printf("ok file path : %s\r\n",name_buffer[4]);
    /* addr == config */

    current_allocate_rom_addr = base_rom_addr;
    current_allocate_ram_addr = base_ram_addr;
    /* end */

    /* create file */
    fp_create = fopen(name_buffer[2],"wb+");

	if(fp_create == NULL)
	{
		printf("create file %s fail\n",name_buffer[2]);
		return 0;
	}

	/*-----------*/
	int handle;
	struct _finddata_t fileinfo;

	char find_path[200];

	sprintf(find_path,"%s/*ok",name_buffer[4]);

	handle=_findfirst(find_path,&fileinfo);

	if(handle == (-1))
	{
	  printf("can not find *.ok files\n");
	  return 0;
	}
	/* at first time */
	if( stack_rom_allocate(fileinfo.name,name_buffer[4]) != 0 )
	{
		printf("Allocate fail\n");
		return 0;
	}

	while(!_findnext(handle,&fileinfo)) 
	{
		if( stack_rom_allocate(fileinfo.name,name_buffer[4]) != 0 )
		{
			printf("Allocate fail\n");
			return 0;
		}
	}

	/* write in end identify */
	arom_write(&endfile_t[0]);
	arom_write(&endfile_t[1]);
	arom_write(&endfile_t[2]);
	/*--------------------*/
	arom_write(&current_allocate_rom_addr);
	arom_write(&current_allocate_ram_addr);
	unsigned int tail = 0xFDEA3B59;
	arom_write(&tail);
	/*--------------------*/
	if( config == 1 )
	{
		flag_bh = 2;
		arom_write(&endfile_t[2]);
	}
	/*-----------------------*/

	fclose(fp_create);
    fclose(fp_ok_read);
	/* open */
    printf("Allocate ok    ROM : 0x%08x  RAM : 0x%08x \r\n",current_allocate_rom_addr,current_allocate_ram_addr);
	/* ok */
	return 0;
}


int stack_rom_allocate(char *name,char *path)
{
	char file_name[100];
    int len = 0;
	unsigned int read_d = 0;
    unsigned int stack_size = 0;
	unsigned int rom_size = 0;
    unsigned int ram_tmp[2];
	unsigned int rom_tmp[2];

	sprintf(file_name,"%s/%s",path,name);

	fp_ok_read = fopen(file_name,"rb");

	if(fp_ok_read == NULL)
	{
		printf("can not open %s No such file\n",file_name);
		return (-1);//
	}

    /* open ok */

	while(1)
	{
        len = fread(&read_d,1,sizeof(read_d),fp_ok_read);

		if(len == 0)
			break;
          
		if( read_d == RAM_ID_HEAD )
		{
           fread(&ram_tmp[0],1,4,fp_ok_read);//ram_tmp
           fread(&ram_tmp[1],1,4,fp_ok_read);//ram_tmp

		   if(ram_tmp[1] == RAM_ID_TAIL)
		   {
               if(rom_size == 0)
			   {
				   /* stack size */
                   stack_size = ram_tmp[0];
			   }
			   
               ram_tmp[0] = ram_tmp[0] + current_allocate_ram_addr;//allocate ram

			   arom_write(&ram_tmp[0]);
			   rom_size += 4;
		   }else
		   {
				arom_write(&ram_tmp[0]);
				arom_write(&ram_tmp[1]);
				rom_size += 4;
				rom_size += 4;
		   }

		}else if( read_d == ROM_ID_HEAD )
		{
           fread(&rom_tmp[0],1,4,fp_ok_read);//ram_tmp
           fread(&rom_tmp[1],1,4,fp_ok_read);//ram_tmp

           if(rom_tmp[1] == ROM_ID_TAIL)
		   {
			   rom_tmp[0] = rom_tmp[0] + current_allocate_rom_addr;//allocate ram

			   arom_write(&rom_tmp[0]);
			   rom_size += 4;
		   }else
		   {
			   arom_write(&rom_tmp[0]);
			   arom_write(&rom_tmp[1]);
			   rom_size += 4;
		       rom_size += 4;
		   }
		}else
		{
            arom_write(&read_d);
			/* rom ++ */
		    rom_size += 4;
		}
	}

	if(stack_size!= 0)
	{
		printf("ROM : 0x%08x  RAM : 0x%08x  Stack : 0x%08x  ROMSize : 0x%08x  count : %03d  file : %s\n",current_allocate_rom_addr,current_allocate_ram_addr,stack_size,rom_size,cnt,file_name);
	}

    current_allocate_rom_addr += rom_size;
    current_allocate_ram_addr += stack_size;
	cnt ++;
	fclose(fp_ok_read);
	return 0;
}

void arom_write(unsigned int * data)
{
   char buffer[200];
   char line[2] = {0x0d,0x0a};
   char endfile[10] = {0x0d,0x0a,'}',';',0x0d,0x0a,0x0d,0x0a,0x0d,0x0a};
   if(config == 0)
   {   //bin file
       fwrite(data,1,4,fp_create);
   }else if(config == 1)
   {
       if(flag_bh == 1)
	   {
         flag_bh = 0;//first time
		 memset(buffer,0,200);
		 sprintf(buffer,"\n/*build time:17:05:15*/\n__attribute__((at(0x%08x))) const unsigned char __FS_rom[] = {\n",base_rom_addr);
         fwrite(buffer,1,strlen(buffer),fp_create);
	   }else if(flag_bh == 2)
	   {
         flag_bh = 0;//first time
         fwrite(endfile,1,10,fp_create);

		 return;
	   }else
	   {

	   }

	   sprintf(buffer,"0x%02x, 0x%02x, 0x%02x, 0x%02x, ",*data&0xff,(*data&0xff00)>>8,(*data&0xff0000)>>16,(*data&0xff000000)>>24);
	   fwrite(buffer,1,strlen(buffer),fp_create);
       enter_cnt++;
	   if(enter_cnt >=4 ) //new line
	   {
		   enter_cnt = 0;
           fwrite(line,1,2,fp_create);
	   }

   }else
   {
   }
}


