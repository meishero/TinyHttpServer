#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#define SIZE 100

static void mymath(char* arg)
{
	char* data[2];
	data[2] = NULL;
	int i = 1;
	
	char *cur = arg + strlen(arg) -1;//-1
	while(cur > arg){
		if(*cur == '='){
			data[i--] = cur+1;
		}
		if(*cur == '&'){
			*cur = '\n';
		}
		cur--;
	}
	printf("<html\n>");
	printf("<h1>\n");
	printf("%s + %s = %d\n", data[0], data[1], atoi(data[0])+atoi(data[1]));
	printf("</h1>\n");
	printf("</html>\n");
	

}
int main()
{
	char method[SIZE];
	char arg[SIZE];
	char content_len[SIZE];
	int len = -1;
	
	if( getenv("REQUEST_METHOD") ){
		strcpy(method, getenv("REQUEST_METHOD"));
	}
	if( strcasecmp(method, "GET")  == 0){
		if( getenv( "QUERY_STRING")){
			strcpy(arg, getenv("QUERY_STRING"));
		}
	}else{//POST
		if(getenv("CONTENT_LENGTH") ){
			strcpy(content_len, getenv("CONTENT_LENGTH"));
			len = atoi(content_len);
		}
		int i = 0;
		while(i<len){
			read(0, &arg[i], 1);
			i++;
		}
		arg[i] = '\0';
		mymath(arg);
	}
	return 0;

}
