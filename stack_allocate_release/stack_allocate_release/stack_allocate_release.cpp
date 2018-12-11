// stack_allocate_release.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "string.h"

#define RAM_ID_HEAD (0x353ab756)
#define RAM_ID_TAIL (0xacb8562d)

#define ROM_ID_HEAD (0x38ab1ef0)
#define ROM_ID_TAIL (0x6184abe6)

static unsigned int mark[4]  = {RAM_ID_HEAD,RAM_ID_TAIL,ROM_ID_HEAD,ROM_ID_TAIL};

static char name_buffer[4][200];//debug,release,output,control

static unsigned int data_b,release_b;

#pragma  comment(  linker,  "/subsystem:\"windows\"  /entry:\"wmainCRTStartup\""  )

FILE *fp_bebug = NULL,*fp_release = NULL,*fp_output;

int Tchar_to_char(_TCHAR * tchar,char * buffer)
{
    int i = 0;
	memset(buffer,0,sizeof(buffer));

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
	int i ,len_b,len_r;
	
	if(argc != 5)
	{
		printf("stack allocate fail,only accept 4 params , now is %d",argc - 1);
		return (-1);
	}

	for(i = 1;i<argc;i++)
	{
       Tchar_to_char(argv[i],name_buffer[i-1]);
	}

	fp_bebug = fopen(name_buffer[0],"rb");
    
	if(fp_bebug == NULL)
	{
		printf("%s open fail\n",name_buffer[0]);
		return 0;
	}

	fp_release = fopen(name_buffer[1],"rb");
    
	if(fp_release == NULL)
	{
		printf("%s open fail\n",name_buffer[1]);
		return 0;
	}

	fp_output = fopen(name_buffer[2],"wb+");
    
	if(fp_output == NULL)
	{
		printf("%s create fail\n",name_buffer[2]);
		return 0;
	}
	unsigned int size_file_d = 0 , size_file_r = 0;
    while(1)
	{
		len_b = fread(&data_b,1,sizeof(data_b),fp_bebug);
        len_r = fread(&release_b,1,sizeof(release_b),fp_release);

		size_file_d += len_b;
		size_file_r += len_r;

		if(len_b==0)
		{
			if(len_r==0)
			{
				/* ok */
				printf("allocate ok , %d ->%d,%s\n",size_file_d,size_file_r,name_buffer[2]);
				fclose(fp_bebug);
				fclose(fp_release);
                fclose(fp_output);
				return 0;
			}
			else
			{
				/* fail */
				printf("allocate fail -> Size are not same %d ->%d\n",size_file_d,len_r);
				return 0;
			}
		}
        
		if(data_b != release_b)
		{
			//
			if((release_b >> 24) == 0x2f)
			{
				/* ram 0x2f400000 */
				release_b  =  release_b - 0x2f400000;
				/* writen identify */
                fwrite(&mark[0],1,4,fp_output);
			    fwrite(&release_b,1,sizeof(release_b),fp_output);
                fwrite(&mark[1],1,4,fp_output);
			}else if((release_b >> 24) == 0x08)
			{
               /* rom 0x08420000 */
				release_b = release_b - 0x08420000;
				/* writen identify */
                fwrite(&mark[2],1,4,fp_output);
			    fwrite(&release_b,1,sizeof(release_b),fp_output);
                fwrite(&mark[3],1,4,fp_output);
			}else
			{
				printf("bad data OFFSET:0x%X 0x%X -> 0x%X\n",size_file_d , data_b,release_b);
				printf("allocate fail\n");
				fclose(fp_bebug);
				fclose(fp_release);
                fclose(fp_output);
				return 0;
			}
		}else
		{
			fwrite(&release_b,1,sizeof(release_b),fp_output);
		}
	}
   



	return 0;
}

