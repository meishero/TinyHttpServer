#include<stdio.h>
#include<pthread.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include<errno.h>

#define SIZE 1024

int recv_n(int sock, char* buf, int len)
{
	char* cur = buf;
	int left = len;
	while(len>0){
		int ret = recv(sock, cur, len, 0);
		if( ret <= 0){
			return -1;
		}
		cur += ret;
		left -= ret;
	}
	return len;
}
int send_n(int sock, char* buf, int len)
{
	char* cur = buf;
	int left = len;
	while(len>0)
	{
		int ret = send(sock, buf, len, 0);
		if( ret<= 0 ){
			return -1;
		}
		cur += ret;
		left -= ret;
	}
	return len;
}
void error_deal(const char* fun, int  line, const char* reason)
{
	
	printf("error:%s,%s,%s", line, fun, reason);
	exit(5);
}
static void echo_www(int sock, const char* path, ssize_t size)
{
	//printf("echo_www\n");
	
	int fd = open(path, O_RDONLY);
	if( fd < 0){
		//????
	}
	
	//printf("open file success\n");
	//printf("get a new client: %d  %s\n",sock, path);

	char* status_line = "HTTP/1.1 200 OK\r\n\r\n";
	//sprintf(status_line, "HTTP/1.0 200 OK\r\n\r\n");
	send(sock, status_line, strlen(status_line), 0);
	//printf("send head success\n");

	int filesize = 0;
	if((filesize = sendfile(sock, fd, NULL, size)) < 0 ){
		//????	
	}
	
	//write(sock, "hello word\n", 10);
	//printf("send file success size is : %d\n", filesize);

	close(fd);
}
		
static int get_line(int sock, char buf[], int buflen)
{
	if(!buf || buflen < 0)
	{
		return -1;
	}
	int i = 0;
	char c = '\0';
	int ret = 0;
	//\n->\n.\r->\n. \r\n->\n
	while((i<buflen-1) && c != '\n')
	{
		ret = recv(sock, &c, 1, 0);
		if( ret>0 )
		{
			if(c == '\r'){
				ret = recv(sock, &c, 1, MSG_PEEK);
				if(ret > 0 && c == '\n' )
					recv(sock, &c, 1, 0);
				else{
					c = '\n';
				}
			}
			buf[i++] = c;
		}
		else
		{
			c = '\n';
		}
	}
	buf[i] = '\0';
	return i;
}

static void clear_header(int sock)
{
	char buf[SIZE];
	int len = SIZE;
	int ret = -1;
	do{
		ret = get_line(sock, buf, len);
		printf("head info%s", buf);
	}while((ret > 0) && (strcmp(buf, "\n") != 0));
	
}


static void exe_cgi(int sock, const char* method, const char* path,\
		const char* query_string)
{
	char buf[SIZE];
	int cgi_output[2];
	int cgi_input[2];
	int content_length = -1;
	char method_env[SIZE];
	char query_string_env[SIZE];
	char content_length_env[SIZE];
	int ret = -1;
	

	printf("querystring:%s\n",query_string);

	if( strcasecmp(method, "GET") == 0)	{
		clear_header(sock);
	}else{
		do{
			memset(buf, '\0', sizeof(buf));
			ret = get_line(sock, buf, sizeof(buf));
			if( strncasecmp(buf, "Content-Length: ", 16) == 0){
				content_length = atoi(&buf[16]);
			}
	printf("head:%s  %d\n", buf,content_length);
		}while(ret > 0 &&  strcmp(buf, "\n") != 0 );
		
		printf("success\n");
		if(content_length < 0){
		//	error_deal(__FUNCTION__, __LINE__, "content-length");
		}
	}
	printf("content%d\n",content_length);
	strcpy(buf, "HTTP/1.1 200 OK\r\n\r\n");
	send(sock, buf, strlen(buf), 0);
	printf("bufsend:%s",buf);
	if(pipe(cgi_input) < 0 || pipe(cgi_output) < 0 )
		printf("pipe error\n");	
	
	pid_t id = fork();
	if(	id < 0 )
		printf("fork error\n");


	if(id == 0){    //child
		close(cgi_output[0]);
		close(cgi_input[1]);

		dup2(cgi_output[1], 1);
		dup2(cgi_input[0], 0);
		
		//??putenv??
		sprintf(method_env, "REQUEST_METHOD=%s", method);
		putenv(method_env);
		
		if(strcasecmp(method, "GET") == 0){
			sprintf(query_string_env, "QUERY_STRING=%s", query_string);
			putenv(query_string_env);
		}else{
			sprintf(content_length_env, "CONTENT_LENGTH=%d", content_length);
			putenv(content_length_env);
		}

		execl(path, path, NULL);
		exit(1);

	}else{   //father
		close(cgi_output[1]);
		close(cgi_input[0]);
		
		char buftmp[SIZE];
		char c = '\0';
		int i = 0;
		if( strcasecmp(method, "POST") == 0 ){
		//	recv_n(sock, buftmp, content_length );		  
			for(; i< content_length; i++)
			{
				recv(sock,&c,1,0);
				write(cgi_input[1], &c ,1);
			}
		//	write(cgi_input[1], buftmp, content_length);
			printf("buftmp%s",buftmp);
		}
		//printf("%s\n", buftmp);
	//	char c = '\0';
		while((ret = read(cgi_output[0], &c, 1)) > 0){
			send(sock, &c, 1, 0);
		}
		close(cgi_input[1]);
		close(cgi_output[0]);
		
		waitpid(id, NULL, 0);
	}
}

static void* accept_request(void* arg)
{
	int sock = *(int*)arg;
	char buf[SIZE];
	//int len = sizeof(buf)/sizeof(buf[0]);

	char method[SIZE/10];
	char url[SIZE];
	char path[SIZE];
	
	int i, j;
	int cgi = 0;
	char* query_string = NULL;

	int ret = -1;
	//do{
	//	ret = get_line(sock, buf, len);
	//	printf("%s\n", buf);
	//}while((ret>0)&& strcmp(buf, "\n")!=0);
	//
	ret = get_line(sock, buf, SIZE);
	if(ret <= 0)
	{
		//???
		printf("getline frist line error");
	}
	//printf("get a new line : %s", buf);
	//printf("line length is %d\n", ret);
	
	//get method
	i = 0;
	j = 0;
	while((i<sizeof(method)-1) && (j<sizeof(buf)-1) && (buf[j] != ' ') ){
		method[i] = buf[j];
		i++;
		j++;
	}
	method[i] = '\0';

	printf("method:%s\n", method);

	if(strcasecmp(method,"GET") != 0 &&\
			strcasecmp(method, "POST") != 0){
		//????
	}

	while( isspace(buf[j]) ){
		j++;
	}
	
	//get url
	i = 0;
	while( (i< sizeof(url)-1) && (j< sizeof(buf)) &&\
			( !isspace(buf[j]) )){
		url[i] = buf[j];
		j++;
		i++;
	}
	url[i] = '\0';

	printf("url:%s\n",url);
	
	//get cgi
	if(strcasecmp(method, "POST") == 0){
		cgi = 1;
		//query_string = url;
		//while(*query_string != '\0' && *query_string != '?'){
		//	query_string++;
		//}
		//if(*query_string == '?'){
		//	cgi = 1;
		//	*query_string = '\0';
		//	query_string++;
		//}	

	}
	if( strcasecmp(method, "GET") == 0){
		query_string = url;
		while(*query_string != '\0' && *query_string != '?'){
			query_string++;
		}
		if(*query_string == '?'){
			cgi = 1;
			*query_string = '\0';
			query_string++;
		}	
	}
	printf("query string : %s \n", query_string);

	//strcat url
	sprintf(path, "%s", url);
	if( path[strlen(path)-1] == '/'){
		strcpy(path,"htdoc/index.html");
	}
	else{
		strcpy(path,"cgi/cgi_math");
	}

	printf("path:%s\n",path);
	struct stat st;
	if( stat(path, &st) < 0){
		printf("stat error\n");
	}else{
		if( S_ISDIR(st.st_mode) ){
			printf("if\n");
		}else if( (st.st_mode & S_IXUSR) || \
			 	  (st.st_mode & S_IXGRP) || \
				  (st.st_mode & S_IXOTH) ){
			cgi = 1;
			printf("cgi =1!\n");
		}else{
			printf("else\n");
		}
		
		if(cgi){
			printf("cgi!!!\n");
			exe_cgi(sock, method,path, query_string);
			
		}else{
			clear_header(sock);
			printf("path: %s\n",path);
			echo_www(sock, path, st.st_size);
			//printf("fuck\n");
		}
	}

	printf("#############################################\n");
	close(sock);
	return (void*)0;
}
static int startup(const char* _ip, const char* _port)
{
   int sock = socket(AF_INET, SOCK_STREAM, 0);
   if( sock <0 ){
		perror("socket");
		exit(2);
   }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_port = htons(atoi(_port));
	local.sin_addr.s_addr = inet_addr(_ip);
	
	if( bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0  ){
		perror("bind");
		exit(3);
	}

	if( listen(sock,5) < 0 ){
		perror("listen");
		exit(4);
	}

	return sock;
}


int main(int argc, char* argv[])
{
	if(argc != 3){
		printf("usage: %s, [ip][port]\n",argv[0]);
		exit(1);	
	}
	
	struct sockaddr_in peer;
	socklen_t len = sizeof(peer);

	int listen_sock = startup(argv[1], argv[2]);

	int done = 0;
	while(!done)
	{
		int new_sock = accept(listen_sock, (struct sockaddr*)&peer, &len);
		if(new_sock > 0){
			pthread_t id;
			printf("newsock:%d\n", new_sock);
			pthread_create(&id, NULL,accept_request, (void*)&new_sock );
			pthread_detach(id);
		}
	}

	return 0;
	
}


