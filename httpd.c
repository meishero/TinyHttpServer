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

static void echo_www(int sock, const char* path, ssize_t size)
{
	printf("echo_www\n");
	
	int fd = open(path, O_RDONLY);
	if( fd < 0){
		//????
	}
	
	printf("open file success\n");
	printf("get a new client: %d  %s\n",sock, path);

	//char* status_line = "HTTP/1.0 200 OK\r\n\r\n";
	//sprintf(status_line, "HTTP/1.0 200 OK\r\n\r\n");
	//send(sock, status_line, strlen(status_line), 0);
	//printf("send head success\n");

	int filesize = 0;
	if((filesize = sendfile(sock, fd, NULL, size)) < 0 ){
		//????	
	}
	
	//write(sock, "hello word\n", 10);
	printf("send file success size is : %d\n", filesize);

	//close(fd);
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
	}while((ret > 0) && (strcmp(buf, "\n") != 0));
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
	printf("get a new line : %s", buf);
	printf("line length is %d\n", ret);
	
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
	sprintf(path, "htdoc%s", url);
	if( path[strlen(path)-1] == '/'){
		strcat(path,"index.html");
	}

	printf("path: %s\n",path);
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
		}else{
			//clear_header(sock);
			printf("path: %s\n",path);
			echo_www(sock, path, st.st_size);
			printf("fuck\n");
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


