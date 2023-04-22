#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define ENTITY_SIZE 1000000
#define CHUNKED -2
struct headers {
	char* n;
	char* v;
};

struct headers h[100];
char sl[1001];
char hbuf[5000];
char chunk[10];
char check[3];

struct sockaddr_in remote_addr;
unsigned char* ip;
char* buf_dest;
char* request = "GET / HTTP/1.1\r\nHost:www.google.com\r\n\r\n";
unsigned char entity[ENTITY_SIZE+1];

int main() {
	int i,j,s,t;
	int length = -1;
	int chunked = 0;
	int chunk_size = -3;
	if (-1 == (s = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("Socket fallita");
		printf("%d\n", errno);
		return 1;
	}
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(80);
	ip = (unsigned char*) &remote_addr.sin_addr.s_addr;
	ip[0]=142; ip[1]=250; ip[2]=200; ip[3]=36;
	t = connect(s, (struct sockaddr*) &remote_addr, sizeof(struct sockaddr_in));
	if (t == -1) {
		perror("Connect fallita\n");
		return 1;
	}
	
	for (t=0; request[t]; t++);
	write(s, request, t);
	for (i=0; i<1000 && read(s,sl+i,1) && (sl[i]!='\n' || sl[(i)?i-1:i]!='\r'); i++) {};
	sl[i]=0;
	printf("Status Line ---> %s\n", sl);

	h[0].n = &hbuf[0];
	for (j=0,i=0; i<5000 && read(s,hbuf+i,1); i++) {
		if (hbuf[i]=='\n' && hbuf[(i)?i-1:i]=='\r') {
			hbuf[i-1]=0;
			if (h[j].n[0] == 0) break;
			h[++j].n = hbuf+i+1;
		}
		else if (hbuf[i]==':' && !h[j].v) {
			hbuf[i]=0;
			h[j].v = hbuf+i+1;
		}
	}
	for (i=0; h[i].n[0]; i++) {
		printf("h[%d].n ---> %s, h[%d].v ---> %s\n", i, h[i].n, i, h[i].v);
		if (!strcmp(h[i].n, "Content-Length")) length=atoi(h[i].v);
		if (!strcmp(h[i].n, "Transfer-Encoding") && !strcmp(h[i].v, " chunked")) length=CHUNKED;
	}
	
	if (length==-1) length = 5000;
	// i per chunk_size, j per entity
	if (length==CHUNKED) {
		for (i=0, j=0; chunk_size!=0; ) {
			for (i=0; read(s,chunk+i,1) && chunk[i]!='\n' && chunk[(i)?i-1:i]!='\r'; i++);	
			chunk[i-1]=0;
			chunk_size = (int) strtol(chunk, NULL, 16);
			for (i=0; i<chunk_size && (t=read(s,entity+j,chunk_size-i)); j+=t, i+=t);
			entity[j]=0;
			for (i=0; i<2 && (t=read(s,check+i,2-i)); i+=t);
			if (check[0]!='\r' || check[1]!='\n') {printf("Errore in lettura chunk-data\n"); return -1;}
		}
	}
	else {
		for (i=0; i<length && (t=read(s,entity+i,ENTITY_SIZE-i)); i+=t);
		entity[i]=0;
	}
	printf("%s\n", entity);
	return 1;
}
