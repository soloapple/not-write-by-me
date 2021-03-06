#include <malloc.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
//#include "webadmin.h"
//#include "cgi.h"

#define		MALLOC_FALSE			-26		// 内存分配失败.
#define		OPEN_FILE_READ_FALSE	-29		// 读打开文件失败
#define		STD_BUF			1024	// 数组缓冲区长度
#define OK							0

typedef struct __conf_item
{
    char *item_name;
    char *item_value;
    struct __conf_item *item_next;
} conf_item;

typedef struct __query_conf
{
    char *label_name;
    conf_item *label_item;
    struct __query_conf *label_next;
} query_conf;

void free_item(conf_item **item)
{
	conf_item *item_tmp = NULL;
	if ((item == NULL) || (*item == NULL))
		return;

	item_tmp = *item;
	while (item_tmp != NULL)
	{
		*item = item_tmp;
		item_tmp = item_tmp->item_next;

		if ((*item)->item_name != NULL)
			free((*item)->item_name);
		if ((*item)->item_value != NULL)
			free((*item)->item_value);
		free(*item);
	}

	*item = NULL;
}

query_conf *find_label(query_conf *p_query_conf, char *label_name)
{
	query_conf *que = NULL;

	if ((p_query_conf == NULL) || (label_name == NULL))
		return NULL;

	for (que = p_query_conf; que != NULL; que = que->label_next)
	{
		if (strcmp(que->label_name, label_name) == 0)
			break;
	}

	return que;
}

char *get_value_from_label(query_conf *que, char *item_name)
{
	conf_item *item = NULL;
	char *res = NULL;
	if ((que == NULL) || (item_name == NULL))
		return NULL;

	item = que->label_item;
	while (item != NULL)
	{
		if (strcmp(item->item_name, item_name) == 0)
		{
			res = item->item_value;
			break;
		}
		item = item->item_next;
	}
	return res;
}

char* trim(char *str)
{
	if (str == NULL)
		return NULL;

	char *base = str;
	char *curr = str;
	while (*curr != '\0')
	{
		if (isspace(*curr))
		{
			++curr;
			continue;
		}
		*base++ = *curr++;
	}
	*base = '\0';

	return str;
}

char * inet_ultoa(unsigned int u, char * s)
{
    static char ss[20];

    if (s == NULL)
        s = ss;
    sprintf(s, "%d.%d.%d.%d",
            (unsigned int)(u>>24)&0xff, (unsigned int)(u>>16)&0xff,
            (unsigned int)(u>>8)&0xff, (unsigned int)u&0xff);
    return s;
}

unsigned int inet_atoul(const char * s)
{
    int i;
    int u[4];
    unsigned int rv;

    if(sscanf(s, "%d.%d.%d.%d", &u[0], &u[1], &u[2], &u[3]) == 4) {
        for (i = 0, rv = 0; i < 4; i++) {
            rv <<= 8;
            rv |= u[i] & 0xff;
        }
        return rv;
    } else
        return 0xffffffff;
}

char *pre_deal_with_line(char *line)
{
    char *ptr = NULL;

	if (line == NULL)
		return NULL;

    if ((ptr = strstr(line, "\n")) != NULL)
    {
        if (ptr - line > 1)
        {
            if (*(ptr - 1) == '\r')
                *(ptr - 1) = '\0';
        }
        *ptr = '\0';
    }

	if ((ptr = strstr(line, "#")) != NULL)
		*ptr = '\0'; 

    trim(line);

    if (line[0] == '\0')
         return NULL;
        
    return line;
}




static query_conf *deal_with_label_line(char *read_line, query_conf **p_conf_head, query_conf **p_conf_tail)
{
    char name[64] = {0};
    query_conf *p_conf_que = NULL;

    if (strncmp(read_line, "[", 1) == 0)
    {
        if (strncmp(read_line, "[/", 2) == 0)
            return NULL;

        sscanf(read_line+1, "%[^]]", name);
        if (name[0] == '\0')
            return NULL;

        p_conf_que = (query_conf *)malloc(sizeof(query_conf));
        p_conf_que->label_name = strdup(name);
        p_conf_que->label_item = NULL;
        p_conf_que->label_next = NULL;

        if ((*p_conf_head) == NULL)
        {
            (*p_conf_head) = p_conf_que;
        }
        else
        {
            (*p_conf_tail)->label_next = p_conf_que;
        }
        (*p_conf_tail) = p_conf_que;

    }

    return *p_conf_tail;
}

char **mSplit(	char *str, 			// 要分解的字符串
				char *sep, 			// 分隔符字符串
				int max_strs, 		// 应该返回的字符数目
				int *toks, 			// 实际分解的字符数目
				char meta			// str中的转意字符
			 )
{
    char **retstr;      /* 2D array which is returned to caller */
    char *idx;          /* index pointer into str */
    char *end;          /* ptr to end of str */
    char *sep_end;      /* ptr to end of seperator string */
    char *sep_idx;      /* index ptr into seperator string */
    int len = 0;        /* length of current token string */
    int curr_str = 0;       /* current index into the 2D return array */
    char last_char = (char) 0xFF;
	int k;

    *toks = 0;
    if (!str) 
		return NULL;


    sep_end = sep + strlen(sep);
    end = str + strlen(str);

    //
    //	删除尾部空格
    //
    while(isspace((int) *(end - 1)) && ((end - 1) >= str))
        *(--end) = '\0';

    sep_idx = sep;
    idx = str;
	//
	//	分配指针数组
	//
    if((retstr = (char **) malloc((sizeof(char **) * max_strs))) == NULL)
        return NULL;


    max_strs--;

    while(idx < end)
    {
        while(sep_idx < sep_end)
        {
            //
            //	判断是否分割符
            //
            if((*idx == *sep_idx) && (last_char != meta))
            {
                if(len > 0)
                {
                    if(curr_str <= max_strs)
                    {
                        //
                        //	分配内存。
                        //
                        if((retstr[curr_str] = (char *) malloc((sizeof(char) * len) + 1)) == NULL)
                        {
                            for(k=0;k<curr_str;k++);
                            	free(retstr[k]);
                            free(retstr);
                            return NULL;
                        }
						//
						//	复制字段数据
						//	
                        memcpy(retstr[curr_str], (idx - len), len);
                        retstr[curr_str][len] = 0;
                        len = 0;
                        curr_str++;
                        last_char = *idx;
                        idx++;
                    }
                    //
                    //	判断是否超过了指定的字段数
                    //
                    if(curr_str >= max_strs)
                    {
                        while(isspace((int) *idx))
                            idx++;
						//
						// 将剩余的数据全部符给最后一个字段。
						//
                        len = end - idx;

                        if((retstr[curr_str] = (char *) malloc((sizeof(char) * len) + 1)) == NULL)
                        {
                            for(k=0;k<curr_str;k++);
                            	free(retstr[k]);
                            free(retstr);
                            return NULL;
                        }

                        memcpy(retstr[curr_str], idx, len);
                        retstr[curr_str][len] = 0;

                        *toks = curr_str + 1;

                        return retstr;
                    }
                }
                else
                {
                    //
                    //	空的字段，丢掉。
                    //
                    last_char = *idx;
                    idx++;
                    sep_idx = sep;
                    len = 0;
                }
            }
            else
            {
                //
                //	不等于当前分割符，使用下一个分割符进行判断。
                //
                sep_idx++;
            }
        }

        sep_idx = sep;
        //
        //	记录当前字段数据长度
        //
        len++;
        last_char = *idx;
        idx++;
    }
    if(len > 0)
    {
        if((retstr[curr_str] = (char *) malloc((sizeof(char) * len) + 1)) == NULL)
		{
            for(k=0;k<curr_str;k++);
            	free(retstr[k]);
            free(retstr);
            return NULL;
		}
        memcpy(retstr[curr_str], (idx - len), len);
        retstr[curr_str][len] = 0;
        *toks = curr_str + 1;
    }

    return retstr;
}


/*
 * parse line of label items such as 'name = value'
 */
static conf_item *deal_with_item_line(char *read_line, query_conf **p_conf_que, conf_item **p_item_tail)
{
    char name[64] = {0};
    char value[512] = {0};
    char *p_equal_sign = NULL;

	if ((read_line == NULL) || (p_conf_que == NULL) || (*p_conf_que == NULL) || (p_item_tail == NULL))
		return NULL;

    if ((p_equal_sign = strchr(read_line, '=')) != NULL)
    {
        name[0] = '\0';
        value[0] = '\0';

        if ((*p_conf_que)->label_name[0] == '\0')
            return NULL;

        if ( p_equal_sign[1] == '\0')
        {
            sscanf(read_line, "%[^=]", name);
            sprintf(value, "%s", "");
        }
        else
        {
            sscanf(read_line, "%[^=]", name);
            p_equal_sign += 1;
            sscanf(p_equal_sign, "%[^ ]", value);
        }

        conf_item *p_item_node = (conf_item *)malloc(sizeof(conf_item));
        p_item_node->item_name = strdup(name);
        p_item_node->item_value = strdup(value);

        if ((*p_conf_que)->label_item == NULL)
            (*p_conf_que)->label_item = p_item_node;
        else
            (*p_item_tail)->item_next = p_item_node;
        *p_item_tail = p_item_node;
    }

    return *p_item_tail;
}




char* line_from_buf(char *cursor, char *store, int storesz)
{
    if ((cursor == NULL) || (store == NULL) || (storesz <= 0))
        return NULL;
    if (*cursor == '\0')
    {
        store[0] = '\0';
        return NULL;
    }

    char *ptr = strstr(cursor, "\n");
    int size = 0;
    if (ptr != NULL)
    {
        if (ptr - cursor > storesz - 1)
            return NULL;
        memcpy(store, cursor, ptr - cursor + 1);
        ptr += 1;
        if (*ptr == '\0')
            return NULL;
    }
    else
    {
        size = strlen(cursor);
        if (size > storesz - 1)
            return NULL;
        strcpy(store, cursor);
//        ptr = store + size + 1;
    }

    return ptr;
}



query_conf * load_configuration(const char *filepath)
{
    FILE *fp = NULL;

    char read_line[2048] = {0};
    //int  line_sz = 2048;
    char *line_flg = NULL;

    query_conf *p_conf_head = NULL;
    query_conf *p_conf_tail = NULL;
    conf_item *p_item_tail = NULL;

    if (filepath == NULL)
        return NULL;

    if ((fp = fopen(filepath, "r")) == NULL)
    {
        return NULL;
    }

    char *pBuf = NULL;
	struct stat fs;
	size_t filesz = 0;
	fstat(fp->_fileno, &fs);
	filesz = fs.st_size;
	if (filesz == 0)
    {
        fclose(fp);
		return NULL;
    }
	pBuf = (char*)malloc(filesz);
	if (pBuf == NULL)
    {
        fclose(fp);
		return NULL;
    }
	long nread = fread(pBuf, filesz, 1, fp);
	if (nread != 1)
	{
        fclose(fp);
		free(pBuf);
		return NULL;
	}

    //bool bIsGeneral = false;
    char *pFlg = strstr(pBuf, "[");
    if (pFlg == NULL)
    {
        //bIsGeneral = true;
    }
    else 
        pFlg = strstr(pFlg, "]");
    if (pFlg == NULL)
    {
        //bIsGeneral = true;
        p_conf_head = (query_conf *)malloc(sizeof(query_conf));
        char *general_name = (char*)malloc(32);
        strcpy(general_name, "default");
        p_conf_head->label_name = general_name;
        p_conf_head->label_item = NULL;
        p_conf_head->label_next = NULL;
        p_conf_tail = p_conf_head;
    }

    //while (feof(fp) == 0)
    pFlg = pBuf;
    while ((pFlg = line_from_buf(pFlg, read_line, sizeof(read_line))) != NULL)
    {
        //memset(read_line, 0, sizeof(read_line));
        //fgets(read_line, line_sz, fp);

        line_flg = pre_deal_with_line(read_line);
        if (line_flg == NULL)
            continue;
        puts(read_line);

        if (strncmp(read_line, "[", 1) == 0)
        {
            if (deal_with_label_line(read_line, &p_conf_head, &p_conf_tail) == NULL)
                continue;
        }

        if (strstr(read_line, "=") != NULL)
        {
            if (deal_with_item_line(read_line, &p_conf_tail, &p_item_tail) == NULL)
                continue; 
        }

        memset(read_line, 0, sizeof(read_line));
    }

    if (p_item_tail != NULL)
		p_item_tail->item_next = NULL;
    if (p_conf_tail != NULL)
		p_conf_tail->label_next = NULL;

    //fclose(fp);
    return p_conf_head;
}

void free_configuration(query_conf **pque)
{
	query_conf *que = NULL;

	if ((pque == NULL) || (*pque == NULL))
		return;
	
	que = *pque;
	while (que != NULL)
	{
		*pque = que;
		que = que->label_next;

		free_item(&((*pque)->label_item));
	}
	*pque = NULL;
}

struct sock_conf
{
	unsigned int s_ip;
	unsigned short s_port;
};

struct sock_conf sc;

static int    cgiHeaderFlag = 0;
int cgiPrintf(char *fmt, ...)
{
  va_list argp;

  if (!cgiHeaderFlag)
    return 0;

  va_start(argp, fmt);
  vfprintf(stdout, fmt, argp);
  va_end(argp);

  fprintf(stdout, "\n");

  fflush(stdout);

  return 1;
}



int getField( int Index,		// 字段号
			  char ***pFields,	// 字段默认值
   			  char *fileName,	// 配置文件名
			  char *c			// 字段之间的分割符
			)
{
	FILE *pf;
	char buf[STD_BUF];
	char **fields ;
	int max = 10;
	int num = 0;
	int ret = OK;
	fields = (char**)malloc(max*sizeof(char*));
	if (fields == NULL)
	{
		//
		// 内存分配失败.
		//
		return MALLOC_FALSE;	
	}
	
	if (Index < 0)
	{
		Index = 0;
	}
		
	pf = fopen(fileName,"r");
	if (pf == NULL)
		return OPEN_FILE_READ_FALSE;
	
	if (c == NULL)
		c = "\t ";
		
	while(fgets(buf, STD_BUF, pf) != NULL)	
	{
		char *index = buf;
	    	//
	    	// 去掉空白符
	    	//
	    	while(*index == ' ' || *index == '\t')
	    		index++;
            	//
            	//	注释行，空行去掉。以字符“#”和“;”开始的行为注释行。
            	//
            	if((*index != '#') && (*index != 0x0a) && (*index != ';') && (index != NULL))
            	{
            		char **toks;
            		int num_toks;
					int k;
            		//
            		//
            		//	分解字段
            		//
            		toks = mSplit(index, c, 50, &num_toks, 0);
					printf("toks:%s\n", *toks);
            		//
            		//	判断是否存在相同的主键值.
            		//
            		if ((toks)&&(Index<num_toks))
            		{
	            		if (max == (num+1))
						{
							char **p;
							p = (char**)realloc(fields,(max+max)*sizeof(char*));
							if (p == NULL)
							{
								ret = MALLOC_FALSE;
								k=0;
								while(fields[k])
								{
									free(fields[k++]);
								}
								free(fields);
								fclose(pf);
								return ret;
							}
							fields = p;
						}
           				for (k=0;k<num_toks;k++)
						{            		
            				if (k==Index)
            				{
	            				fields[num]	  = toks[Index];
	            				fields[num+1] = NULL;
	            				num++;
	            			}
	            			else
	            			{
    	        				free(toks[k]);
    	        			}
        	    		}
            			free(toks);
	            		continue;
            		}
            		for(k=0;k<num_toks;k++)
            			free(toks[k]);
            		if (toks) free(toks);
            	}
        }
        fclose(pf); 
		(*pFields) = fields;

        return OK;
}

int getFieldValue( int keyIndex,	// 主键序号
					char *key,		// 主键值
					int	 numField , // 字段个数
					char ***pFields,	// 字段默认值
					char *fileName,	// 配置文件名
					char *c			// 字段之间的分割符
				)
{
	FILE *pf;
	char buf[STD_BUF];
	char **fields = *pFields;
	int line = 0;
	
	//
	//	必须有主键,用于确定唯一的记录
	//
	if ((keyIndex < 0)&&(key))
	{
		//return NO_KEY;
		return -1;
	}		
	//puts("~~~~~~~~~~~~~~~~~~~~~~~~1");
	pf = fopen(fileName,"r");
	if (pf == NULL)
		return OPEN_FILE_READ_FALSE;
	
	//puts("~~~~~~~~~~~~~~~~~~~~~~~~2");
	if (c == NULL)
		c = "\t ";
	while(fgets(buf, STD_BUF, pf) != NULL)	
	{
	//puts("~~~~~~~~~~~~~~~~~~~~~~~~2.1");
		char *index = buf;
	    	//
	    	// 去掉空白符
	    	//
	    	while(*index == ' ' || *index == '\t')
	    		index++;
            	//
            	//	注释行，空行去掉。以字符“#”和“;”开始的行为注释行。
            	//
            	if((*index != '#') && (*index != 0x0a) && (*index != ';') && (index != NULL))
            	{
            		char **toks;
            		int num_toks;
					int k;
            		//
            		//	分解字段
            		//
            		toks = mSplit(index, c, 50, &num_toks, 0);
					printf("%d\n", num_toks);
            		//
            		//	判断是否存在相同的主键值.
            		//
            		line++;
	//puts("~~~~~~~~~~~~~~~~~~~~~~~~2.2");
	//printf("line:%d\n", line);
					int j=0;
					for(j;j<num_toks;j++)
						printf("[%s] ", toks[j]);
						printf("\n");
					//printf("%s %s %s %s %s\n", toks[0], toks[1], toks[2], toks[3], toks[4]);
            		if ((key==NULL && line==keyIndex)||((toks)&&(key)&&(strcmp(toks[keyIndex],key)==0)))
            		{
	puts("~~~~~~~~~~~~~~~~~~~~~~~~3");
						printf("%s %s %s\n", toks[0], toks[1], toks[2]);
						printf("keyIndex:%d toks:%s key:%s\n", keyIndex, toks[keyIndex], key);
	            		if (fields == NULL)
						{
	            			fields = (char**)malloc((1+num_toks)*sizeof(char*));
	            			if (fields == NULL)
	            			{
			            		//
			            		// 内存分配失败.
			            		//
			            		for(k=0;k<num_toks;k++)
			            			free(toks[k]);
			            		free(toks);
								fclose(pf);
	            				//return MALLOC_FALSE;	
	            				return -1;	
	            			}
						}
	puts("~~~~~~~~~~~~~~~~~~~~~~~~4");
	            		//
	            		//	字段数必须相等才能赋值.
	            		//
	            		//if ( num_toks <= numField )
						{
	            			for (k=0;k<num_toks;k++) {
	            				fields[k] = toks[k];
								printf("k[%d]:[%s]\n", k,fields[k]);
							}	
	            			fields[num_toks]=NULL;
	            			free(toks);
							fclose(pf);
							*pFields = fields;
	            			return OK;
						}
	puts("~~~~~~~~~~~~~~~~~~~~~~~~5");
					}
					for(k=0;k<num_toks;k++)
						free(toks[k]);
					if (toks) free(toks);
				}
	puts("~~~~~~~~~~~~~~~~~~~~~~~~6");
	}
	fclose(pf); 
	if (fields) free(fields);
	puts("~~~~~~~~~~~~~~~~~~~~~~~~7");
	//return NO_FIND_RECORD;
	return -1;
}

int delRecord(int keyIndex,char *key,char *fileName,char *c)
{
	char tempFile[STD_BUF];
	char buf[STD_BUF];
	FILE *pf,*pTmp;
		
	pf = fopen(fileName,"r");
	if (pf==NULL)
		return OPEN_FILE_READ_FALSE;
	if (c == NULL)
		c = "\t ";
	//
	//	生成临时文件
	//
	sprintf(tempFile,"%s.temp",fileName);
	pTmp = fopen(tempFile,"w");
	while((pTmp)&&(fgets(buf, STD_BUF, pf) != NULL))	
	{
		char *index = buf;
	    	//
	    	// 去掉空白符
	    	//
	    	while(*index == ' ' || *index == '\t')
	    		index++;
            	//
            	//	注释行，空行去掉。以字符“#”和“;”开始的行为注释行。
            	//
            	if((*index != '#') && (*index != 0x0a) && (*index != ';') && (index != NULL))
            	{
            		char **toks;
            		int num_toks;
					int k;
            		//
            		//	分解字段
            		//
             		toks = mSplit(index, c, 5, &num_toks, 0);
            		//
            		//	判断是否是要删除的记录.
            		//
            		if (( !toks ) || strcmp(toks[keyIndex],key))
            		{
						//
						//	如果不是复制数据
						//
						fprintf(pTmp,"%s\n",buf);
            		}
            		for(k=0;k<num_toks;k++)
            			free(toks[k]);
            		if (toks) free(toks);
            	}
            	else
            	{
            		fprintf(pTmp,buf);
            	}
        }
        fclose(pf);
	if (pTmp)
	{
		fclose(pTmp);
		unlink(fileName);		// 删除原文件:可能出现同步问题.
		rename(tempFile,fileName);	// 改名为新文件
		return OK;
	}
	//return OPEN_TEMP_FILE_FALSE ;
	return -1;
}

int updateRecord(int keyIndex,int numRecord,char **Record,char *fileName,char *c)
{
	char tempFile[STD_BUF];
	char buf[STD_BUF];
	FILE *pf,*pTmp;
	char *key = NULL;
	int  IsModify = 0;
	unsigned int line = 0;
		
	if (keyIndex >= 0)
	{
		//
		//	获取主键值.
		//
		key = Record[keyIndex];
	}
		
	
	pf = fopen(fileName,"r");
	if (pf==NULL)
		return OPEN_FILE_READ_FALSE;
	if (c == NULL)
		c = "\t ";
	//
	//	生成临时文件
	//
	sprintf(tempFile,"%s.temp",fileName);
	pTmp = fopen(tempFile,"w");
	while((pTmp)&&(fgets(buf, STD_BUF, pf) != NULL))	
	{
		char *index = buf;
	    	//
	    	// 去掉空白符
	    	//
	    	while(*index == ' ' || *index == '\t')
	    		index++;
            	//
            	//	注释行，空行去掉。以字符“#”和“;”开始的行为注释行。
            	//
            	if((*index != '#') && (*index != 0x0a) && (*index != ';') && (index != NULL))
            	{
            		char **toks;
            		int num_toks;
					int k;
					
					line++;
					if (NULL == key)
					{
						//
						// 以记录号进行修改
						//
						if (line == -keyIndex)
						{
							for(k=0;k<numRecord;k++)
	            			{
								if (k==0)
								{
									fprintf(pTmp,"%s",Record[k]);
								}
								else
								{
									fprintf(pTmp,"%c%s",c[0],Record[k]);
								}
								IsModify = 1;
	            			}
	            			fprintf(pTmp,"\n");
						}
						else
							fprintf(pTmp,"%s",buf);

						continue;
					}
            		//
            		//	分解字段
            		//
            		toks = mSplit(index, c, numRecord+1, &num_toks, 0);
            		//
            		//	判断是否是要修改的记录.
            		//

					printf("<<<toks[%s] keyindex:[%d] key:[%s]\n", toks[keyIndex], keyIndex, key);
            		if (( toks ) && (strcmp(toks[keyIndex],key)==0))
            		{
						for(k=0;k<num_toks;k++)
            			{
							if (k==0)
							{
								if (Record[k])
									fprintf(pTmp,"%s",Record[k]);
								else
									fprintf(pTmp,"%s",toks[k]);
							}
							else
							{
								if (Record[k])
									fprintf(pTmp,"%c%s",c[0],Record[k]);
								else
									fprintf(pTmp,"%c%s",c[0],toks[k]);
							}
							IsModify = 1;
            			}
            			fprintf(pTmp,"\n");
            		}
            		else
            		{
						//
						//	如果不是复制数据
						//
						fprintf(pTmp,"%s\n",buf);
            		}
            		for(k=0;k<num_toks;k++)
            			free(toks[k]);
            		if (toks) free(toks);
            	}
            	else
            	{
            		fprintf(pTmp,buf);
            	}
        }
        fclose(pf);
	if (pTmp)
	{
		fclose(pTmp);
		if (IsModify)
		{
			unlink(fileName);		// 删除原文件:可能出现同步问题.
			rename(tempFile,fileName);	// 改名为新文件
			return OK;
		}
		else
		{
			unlink(tempFile);
	//		return NO_FIND_RECORD;
			return -1;
		}
	}
//	return OPEN_TEMP_FILE_FALSE ;
	return -1;
}

int delMultiRecord(int keyIndex,int numDel,char **delKey,char *fileName,char *c)
{
	char tempFile[STD_BUF];
	char buf[STD_BUF];
	int lines = 0;
	FILE *pf,*pTmp;
		
	pf = fopen(fileName,"r");
	if (pf==NULL)
		return OPEN_FILE_READ_FALSE;
	if (c ==NULL)
		c = "\t ";
	//
	//	生成临时文件
	//
	sprintf(tempFile,"%s.temp",fileName);
	pTmp = fopen(tempFile,"w");
	while((pTmp)&&(fgets(buf, STD_BUF, pf) != NULL))	
	{
		char *index = buf;
	    	//
	    	// 去掉空白符
	    	//
	    	while(*index == ' ' || *index == '\t')
	    		index++;
            	//
            	//	注释行，空行去掉。以字符“#”和“;”开始的行为注释行。
            	//
            	if((*index != '#') && (*index != 0x0a) && (*index != ';') && (index != NULL))
            	{
            		char **toks;
            		int num_toks;
					int k;
            		lines++;
            		if (keyIndex < 0)
            		{
            			//
            			//	按行号进行删除
            			//
						for(k=0;k<numDel;k++)
						{
							if(lines == atoi(delKey[k]))
								break;
						}
						if (k>=numDel)
            			{
							//
							//	如果不是复制数据
							//
							fprintf(pTmp,"%s\n",buf);							
            			}
            		}
            		else
            		{
            			//
            			//	分解字段
            			//
            			toks = mSplit(index, c, keyIndex+2, &num_toks, 0);
            			//
            			//	判断是否是要删除的记录.
            			//
            			if (toks && (num_toks>=keyIndex))
						{
							
							//
							//	判断该记录是否是要删除的记录
							//
							for(k=0;k<numDel;k++)
							{
								if(strcmp(toks[keyIndex],delKey[k])==0)
									break;
							}
							if (k>=numDel)
            				{
								//
								//	如果不是复制数据
								//
								fprintf(pTmp,"%s\n",buf);
    	        			}
	            			for(k=0;k<num_toks;k++)
            					free(toks[k]);
            				if (toks) free(toks);
						}
					}
            	}
            	else
            	{
            		fprintf(pTmp,buf);
            	}
        }
        fclose(pf);
	if (pTmp)
	{
		fclose(pTmp);
		unlink(fileName);		// 删除原文件:可能出现同步问题.
		rename(tempFile,fileName);	// 改名为新文件
		return OK;
	}
	//return OPEN_TEMP_FILE_FALSE ;
	return -1;
}

int ExistRecord(int keyIndex,char *key,char *fileName,char *c)
{
	char buf[STD_BUF];
	FILE *pf;
		
	pf = fopen(fileName,"r");
	if (pf==NULL)
		return OPEN_FILE_READ_FALSE;
	if (c==NULL)
		c = "\t ";
	while((fgets(buf, STD_BUF, pf) != NULL))	
	{
		char *index = buf;
	    //
	    // 去掉空白符
	    //
	    while(*index == ' ' || *index == '\t')
	    	index++;
        //
        //	注释行，空行去掉。以字符“#”和“;”开始的行为注释行。
        //
        if((*index != '#') && (*index != 0x0a) && (*index != ';') && (index != NULL))
        {
			char **toks;
			int num_toks;
			int k;
			int ret = 0 ;
			//
            //	分解字段
            //
            toks = mSplit(index, c, keyIndex+2, &num_toks, 0);
            //
            //	判断是否是存在指定的记录.
            //
            if (strcmp(toks[keyIndex],key)==0)
            {
				fclose(pf);
				ret = 1;
			}
			if (toks)
			{
				for (k=0;k<num_toks;k++)
					free(toks[k]);
				free(toks);
			}
			if (ret)
				return 1;
        }
	}
    fclose(pf);
	return 0 ;
}

int addRecord(int keyIndex,int numRecord,char **Record,char *fileName,char *c)
{
	FILE *pf;
	char buf[STD_BUF];
	char *key = NULL;
	int i;
	
	if (keyIndex >= 0)
	{
		//
		//	获取主键值.
		//
		key = Record[keyIndex];
		if (key == NULL)
			return -1;
			//return NO_KEY;
	}
	if ( c == NULL)
		c = "\t ";
	//
	//	如果有主键,判断是否有相同主键记录
	//
	if (key)
	{
		pf = fopen(fileName,"r");
		if (pf==NULL)
			return OPEN_FILE_READ_FALSE;
			
		while(fgets(buf, STD_BUF, pf) != NULL)	
		{
			char *index = buf;
		    	//
		    	// 去掉空白符
		    	//
		    	while(*index == ' ' || *index == '\t')
        	    		index++;
	            	//
	            	//	注释行，空行去掉。以字符“#”和“;”开始的行为注释行。
	            	//
	            	if((*index != '#') && (*index != 0x0a) && (*index != ';') && (index != NULL))
	            	{
	            		char **toks;
	            		int num_toks;
						int k;
	            		//
	            		//	分解字段
	            		//
	            		toks = mSplit(index, c , 5, &num_toks, 0);
	            		//
	            		//	判断是否存在相同的主键值.
	            		//
	//					printf("toks[%d]:[%s] key:[%s]\n",keyIndex, toks[keyIndex], key);
	            		if ((toks)&&(strcmp(toks[keyIndex],key)==0))
	            		{
		            		for(k=0;k<num_toks;k++)
		            			free(toks[k]);
		            		free(toks);
							fclose(pf);
		            		//return KEY_EXIST;
		            		return -1;
	            		}
	            		if (toks)
						{
							for(k=0;k<num_toks;k++)
	            				free(toks[k]);
	            			if (toks) free(toks);
						}
	            	}
	        }
	        fclose(pf);  
 	}
 	pf = fopen(fileName,"a");
 	if (pf == NULL)
 		return -1;
 		//return OPEN_FILE_WRITE_FALSE;
 	
	fprintf(pf,"\n");
 	for (i=0;i<numRecord;i++)
 	{
		if (i==0)
		{
			if (Record[i])
				fprintf(pf,"%s",Record[i]);
			else
				fprintf(pf,"未设置");
		}
		else
		{
			if (Record[i])
				fprintf(pf,"%c%s",c[0],Record[i]);
			else
				fprintf(pf,"%c未设置",c[0]);
		}
 	}
 	fclose(pf);
	return OK;
}


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

#define DAY_TIME_SEC 60*60*24
#define BUF_SIZE		128
#define USER_LIMIT_CONF "./user_limit.conf"

//添加用户密码超时时间记录
int user_passwd_limit_time_add(char *user_name)
{
	int limit_day = 0;

	if (!user_name)
		return 0;

	if ( (limit_day = user_passwd_get_limit_date()) < 0)
		return 0;

	//查看是否存在同名用户
	int ret = ExistRecord(0, user_name, USER_LIMIT_CONF, NULL);
	if (ret == 1) {
		printf("user has exist\n");
		return 0;
	}
	else 
	{
		char *pFields[2];
		pFields[0] = user_name;	
	printf("field0:%s\n", pFields[0]);
		
		char buf_usr_passwd_create[BUF_SIZE]= {0};
		char fmt_usr_passwd_create[BUF_SIZE]= {0};
		char webfmt_usr_passwd_create[BUF_SIZE]= {0};

		time_t t_usr_passwd_create = time(NULL);
		sprintf(buf_usr_passwd_create, "%d", t_usr_passwd_create);
		pFields[1] = buf_usr_passwd_create;

		struct tm *tmp;
		tmp = localtime(&t_usr_passwd_create);
		if (tmp == NULL) {
			perror("localtime");
			return -1;
		}

		if (strftime(fmt_usr_passwd_create, sizeof(fmt_usr_passwd_create), "%F", tmp) == 0) {
			fprintf(stderr, "strftime returned 0");
			return -1;
		}

		int x[3];

	printf("Result string is \"%s\"\n", fmt_usr_passwd_create);
		sscanf(fmt_usr_passwd_create, "%d-%d-%d", &x[0], &x[1], &x[2]);
		printf("@@@:%d %d %d", x[0], x[1], x[2]);
		
		sprintf(webfmt_usr_passwd_create, "%d年%d月%d日", x[0], x[1], x[2]);
	printf("Result string is \"%s\"\n", webfmt_usr_passwd_create);

	printf("field1:%s\n", pFields[1]);	

		char buf_usr_passwd_limit[BUF_SIZE]= {0};
		char fmt_usr_passwd_limit[BUF_SIZE]= {0};

		time_t t_usr_passwd_limit = t_usr_passwd_create + DAY_TIME_SEC * limit_day;
		sprintf(buf_usr_passwd_limit, "%d", t_usr_passwd_limit);
		pFields[2] = buf_usr_passwd_limit;

		tmp = localtime(&t_usr_passwd_limit);
		if (tmp == NULL) {
			perror("localtime");
			return -1;
		}

		if (strftime(fmt_usr_passwd_limit, sizeof(fmt_usr_passwd_limit), "%F", tmp) == 0) {
			fprintf(stderr, "strftime returned 0");
			return -1;
		}
		printf("Result string is \"%s\"\n", fmt_usr_passwd_limit);
		
		printf("field2:%s\n", pFields[2]);	

		ret = addRecord(0, 3, pFields, USER_LIMIT_CONF, NULL);
	}

	return 1;
}

//修改用户密码超时时间
int user_passwd_limit_time_modify(char *user_name)
{
	int limit_day = 0;

	if (!user_name)
		return 0;

	if ( (limit_day = user_passwd_get_limit_date()) < 0)
		return 0;

	int ret = ExistRecord(0, user_name, USER_LIMIT_CONF, NULL);
	if (ret == 0) {
		printf("user is not exist, can't modify!\n");
		return 0;
	}
	else 
	{
		char **pFields = NULL;
		
		ret = getFieldValue(0, user_name, 3, &pFields, USER_LIMIT_CONF, NULL);
		if (ret)
		{
			//dispOperateInfo(NULL,ret,NULL);
			//printf("*********************\n");
			return ret;
		}

		//printf("0:%s\n 1:%s\n 2:%s\n", pFields[0], pFields[1], pFields[1]);
		
		char buf_usr_passwd_now [BUF_SIZE]= {0};
		char fmt_usr_passwd_now[BUF_SIZE]= {0};

		time_t t_usr_passwd_now = time(NULL);
		sprintf(buf_usr_passwd_now, "%d", t_usr_passwd_now);
		pFields[1] = buf_usr_passwd_now;
		
		struct tm *tmp;
		tmp = localtime(&t_usr_passwd_now);
		if (tmp == NULL) {
			perror("localtime");
			exit(EXIT_FAILURE);
		}

		if (strftime(fmt_usr_passwd_now, sizeof(fmt_usr_passwd_now), "%F", tmp) == 0) {
			fprintf(stderr, "strftime returned 0");
			exit(EXIT_FAILURE);
		}

	printf("Result string is \"%s\"\n", fmt_usr_passwd_now);

		char buf_usr_passwd_limit[BUF_SIZE]= {0};
		char fmt_usr_passwd_limit[BUF_SIZE]= {0};

		time_t t_usr_passwd_limit = t_usr_passwd_now+ DAY_TIME_SEC * limit_day;
		sprintf(buf_usr_passwd_limit, "%d", t_usr_passwd_limit);
		pFields[2] = buf_usr_passwd_limit;

		tmp = localtime(&t_usr_passwd_limit);
		if (tmp == NULL) {
			perror("localtime");
			return -1;
		}

		if (strftime(fmt_usr_passwd_limit, sizeof(fmt_usr_passwd_limit), "%F", tmp) == 0) {
			fprintf(stderr, "strftime returned 0");
			return -1;
		}
		printf("Result string is \"%s\"\n", fmt_usr_passwd_limit);

		ret = updateRecord(0, 3, pFields, USER_LIMIT_CONF, NULL);

		return ret;
	}
	
	return -1;
}

//探测一下用户的密码是否超过有效期限
int user_passwd_limit_time_detect(char *user_name)
{
	if (!user_name)
		return 0;

	int ret = ExistRecord(0, user_name, USER_LIMIT_CONF, NULL);
	if (ret == 0) {
		printf("user is not exist, can't detect!\n");
		return 0;
	}
	else 
	{
		puts("~~~~~~~~~1");
		char **pFields = NULL;

		time_t t_usr_passwd_limit;
		time_t t_usr_passwd_now = time(NULL);

		ret = getFieldValue(0, user_name, 3, &pFields, USER_LIMIT_CONF, NULL);
		if (ret)
		{
			//dispOperateInfo(NULL,ret,NULL);
			//printf("*********************\n");
			return ret;
		}
		else
		{
			puts("~~~~~~~~~2");
			sscanf(pFields[2], "%d", &t_usr_passwd_limit);

			time_t t_x = t_usr_passwd_limit - t_usr_passwd_now;
			if (t_x < 0)
			{
				//printf("time has over\n");
				return -1;
			}
			else
			{
				//printf("still has time\n");
				return 1;
			}

		}
	}
	
	return -1;
}

//获取当前用户密码的最后更新时间,格式为 xxxx年xx月xx日
char *user_passwd_curr_time(char *user_name)
{
	if (!user_name)
		return NULL;

	int ret = ExistRecord(0, user_name, USER_LIMIT_CONF, NULL);
	if (ret == 0) {
		printf("user is not exist, can't get time!\n");
		return NULL;
	}
	else 
	{
		puts("~~~~~~~~~1");
		char **pFields = NULL;

		time_t t_usr_passwd_limit;
		time_t t_usr_passwd_now = time(NULL);

		ret = getFieldValue(0, user_name, 3, &pFields, USER_LIMIT_CONF, NULL);
		if (ret)
		{
			//dispOperateInfo(NULL,ret,NULL);
			return NULL;
		}
		else
		{
			int pdate[3];
			struct tm *tmp;
			char buf_usr_passwd_curr[BUF_SIZE]= {0};
			char fmt_usr_passwd_curr[BUF_SIZE]= {0};

			time_t t_usr_passwd_curr;
			sscanf(pFields[1], "%d", &t_usr_passwd_curr);

			char *webfmt_usr_passwd_curr = (char*)malloc(32);
			{
				if (webfmt_usr_passwd_curr == NULL) {
					free(webfmt_usr_passwd_curr);
				}
			}
			memset(webfmt_usr_passwd_curr, 0x00, 32);

			tmp = localtime(&t_usr_passwd_curr);
			if (tmp == NULL) {
				perror("localtime");
				return NULL;
			}

			if (strftime(fmt_usr_passwd_curr, sizeof(fmt_usr_passwd_curr), "%F", tmp) == 0) {
				fprintf(stderr, "strftime returned 0");
				return NULL;
			}

			printf("Result string is \"%s\"\n", fmt_usr_passwd_curr);
			sscanf(fmt_usr_passwd_curr, "%d-%d-%d", &pdate[0], &pdate[1], &pdate[2]);
			printf("@@@:%d %d %d", pdate[0], pdate[1], pdate[2]);

			sprintf(webfmt_usr_passwd_curr, "%d年%d月%d日", pdate[0], pdate[1], pdate[2]);
			printf("Result string is \"%s\"\n", webfmt_usr_passwd_curr);
			
			if (webfmt_usr_passwd_curr != NULL)
				return webfmt_usr_passwd_curr;	
		}
	}
	
	return NULL;
}

//获得当前用户密码的期限截止时间，格式为 xxxx年xx月xx日
char *user_passwd_limit_time(char *user_name)
{
	if (!user_name)
		return NULL;

	int ret = ExistRecord(0, user_name, USER_LIMIT_CONF, NULL);
	if (ret == 0) {
		printf("user is not exist, can't get time!\n");
		return NULL;
	}
	else 
	{
		puts("~~~~~~~~~1");
		char **pFields = NULL;

		time_t t_usr_passwd_limit;
		time_t t_usr_passwd_now = time(NULL);

		ret = getFieldValue(0, user_name, 3, &pFields, USER_LIMIT_CONF, NULL);
		if (ret)
		{
			//dispOperateInfo(NULL,ret,NULL);
			return NULL;
		}
		else
		{
			int pdate[3];
			struct tm *tmp;
			char buf_usr_passwd_curr[BUF_SIZE]= {0};
			char fmt_usr_passwd_curr[BUF_SIZE]= {0};

			time_t t_usr_passwd_curr;
			sscanf(pFields[2], "%d", &t_usr_passwd_curr);

			char *webfmt_usr_passwd_curr = (char*)malloc(32);
			{
				if (webfmt_usr_passwd_curr == NULL) {
					free(webfmt_usr_passwd_curr);
				}
			}
			memset(webfmt_usr_passwd_curr, 0x00, 32);

			tmp = localtime(&t_usr_passwd_curr);
			if (tmp == NULL) {
				perror("localtime");
				return NULL;
			}

			if (strftime(fmt_usr_passwd_curr, sizeof(fmt_usr_passwd_curr), "%F", tmp) == 0) {
				fprintf(stderr, "strftime returned 0");
				return NULL;
			}

			printf("Result string is \"%s\"\n", fmt_usr_passwd_curr);
			sscanf(fmt_usr_passwd_curr, "%d-%d-%d", &pdate[0], &pdate[1], &pdate[2]);
			printf("@@@:%d %d %d", pdate[0], pdate[1], pdate[2]);

			sprintf(webfmt_usr_passwd_curr, "%d年%d月%d日", pdate[0], pdate[1], pdate[2]);
			printf("Result string is \"%s\"\n", webfmt_usr_passwd_curr);
			
			if (webfmt_usr_passwd_curr != NULL)
				return webfmt_usr_passwd_curr;	
		}
	}
	
	return NULL;
}

//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

#define SYS_CONF "./system.conf"
#define FLG_PASS_LIMIT "pass_limit="

int user_passwd_get_limit_date()
{
	int x = 0;
	int ndata = -1;
	char sdata[16] = {0};
	char **p_sys_conf = NULL;
	int ret = getField(0,&p_sys_conf, SYS_CONF,NULL);
	if ((ret==0)&&(p_sys_conf))
	{
		int i=0;
		char *ptmp;
		while(p_sys_conf[i])
		{
			ptmp = (char*)memmem(p_sys_conf[i], strlen(p_sys_conf[i]), FLG_PASS_LIMIT, strlen(FLG_PASS_LIMIT));
			if (ptmp != NULL) {
				sscanf(ptmp + strlen(FLG_PASS_LIMIT), "%s", sdata);
				printf("m:%s\n", sdata);
				
				for(x=0;x<strlen(sdata);x++){
					if (!isdigit(sdata[x]))
						return -1;
				}
				ndata = atoi(sdata);
			}
			free(p_sys_conf[i]);
			i++;
		}
		free(p_sys_conf);
	}

	return ndata;
}

int main(int argc, char**argv)
{

	/*
	   char *val = NULL;
	   char *path = "sock.conf";
	   struct	sockaddr_in s_addr;

	   query_conf *general = NULL;
	   query_conf *conf = NULL;

	   if ((conf = load_configuration(path)) == NULL)
	   {
	   return -1;
	   }

	   if ((general = find_label(conf, (char*)"socket")) == NULL)
	   {
	   free_configuration(&conf);
	   return -1;
	   }

	   if ((val = get_value_from_label(general, (char*)"synconf_ip")) != NULL)
	   sc.s_ip = inet_atoul(val);
	   printf("ip:%s\n", val);

	   if ((val = get_value_from_label(general, (char*)"synconf_port")) != NULL)
	   sc.s_port = inet_atoul(val);
	   printf("port:%s\n", val);

	   s_addr.sin_family = AF_INET;
	   s_addr.sin_port = htons(sc.s_port);
	   s_addr.sin_addr.s_addr = htonl(sc.s_ip);

	   free_configuration(&conf);
	 */

#if 0
	char ** user = NULL;
	int ret = getField(0,&user,"./user.conf",NULL);
	if ((ret==0)&&(user))
	{
		int i=0;
		while(user[i])
		{
			//printf("<option>%s\n",user[i]);
			free(user[i]);
			i++;
		}
		free(user);
	}

	char **pFields=NULL;
	char *username=NULL;
	ret = getFieldValue(2,username,5,&pFields,"./user.conf",NULL);
	if (ret)
	{
		//dispOperateInfo(NULL,ret,NULL);
		printf("*********************\n");
		return ret;
	}

	if (pFields)
	{
		if (pFields[0])
		{
			printf("zzz~0:%s\n", pFields[0]);
			//free(pFields[0]);
		}
		if (pFields[1]) 
		{
			printf("zzz~1:%s\n", pFields[1]);
			//free(pFields[1]);
		}
		if (pFields[2]) 
		{
			printf("zzz~2:%s\n", pFields[2]);
			free(pFields[2]);
		}
		if (pFields[3]) 
		{
			printf("zzz~3:%s\n", pFields[3]);
			//free(pFields[3]);
		}
		if (pFields[4]) 
		{
			printf("zzz~4:%s\n", pFields[4]);
			//free(pFields[4]);
		}

	}

	//char *Field = "admin";
	//ret = delRecord(1, Field, "./user.conf", NULL);

	char buf[16*2] = "5";
	pFields[0]=buf;
	pFields[1]="itor";
	pFields[2]="dasdjlajdlasjdl";
	pFields[3]=buf;
	pFields[4]=buf;

	ret = updateRecord(1, 5, pFields, "./user.conf", NULL);
	
	/*
	ret = ExistRecord(1, "admin", "./user.conf", NULL);
	if (ret == 1)
		printf("exist");
		*/

	//addRecord(1, 5, pFields, "./user.conf", NULL);
	
#endif

	/*
	struct  timeval  start;
	struct  timeval  end;
	unsigned long timer;

	gettimeofday(&start,NULL);
	printf("start.tv_sec:%d start.tv_usec:%d\n", start.tv_sec, start.tv_usec);
	printf("hello world!\n");
	sleep(5);
	gettimeofday(&end,NULL);
	printf("end.tv_sec:%d end.tv_usec:%d\n", end.tv_sec, end.tv_usec);
	timer = (end.tv_sec-start.tv_sec);
	long limit_time = 60*60*24*30; 
#define LIMITTIME 60*60*24*30
	if (timer > LIMITTIME)
		printf(" support limit\n");
	else
		printf("not supercate limit\n");

	printf("%d\n", LIMITTIME - timer);

	unsigned long timer2 = (end.tv_usec-start.tv_usec)*0.0000001;
	//timer = 1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec;
	printf("timer = %ld s\n",timer);
	printf("timer2 = %ld s\n",timer2);
	 */

	/*
	char outstr[200];
	time_t t1;
	time_t t2;
	struct tm *tmp;

	t1 = time(NULL);
	printf("%ld\n", t1);
	sleep(1);
	t2 = time(NULL);
	printf("%ld\n", t2);

	time_t x = t2 - t1;
	printf("%ld\n", x);

	tmp = localtime(&t1);
	if (tmp == NULL) {
		perror("localtime");
		exit(EXIT_FAILURE);
	}

	if (strftime(outstr, sizeof(outstr), argv[1], tmp) == 0) {
		fprintf(stderr, "strftime returned 0");
		exit(EXIT_FAILURE);
	}

	printf("Result string is \"%s\"\n", outstr);
	exit(EXIT_SUCCESS);
	*/

	
	int ret;	
	char *user_name = argv[1];
//	ret = user_passwd_limit_time_add(user_name);

	int n = user_passwd_get_limit_date();
	printf("%d\n", n);

	
	char *p = user_passwd_curr_time(user_name);
	printf("((((( date curr:%s\n", p);
	free(p);
	
	p = user_passwd_limit_time(user_name);
	printf("((((( date limit:%s\n", p);
	free(p);

	//ret = user_passwd_limit_time_detect(user_name);
	//ret = user_passwd_limit_time_modify(user_name);
	
	return ret;
}

















