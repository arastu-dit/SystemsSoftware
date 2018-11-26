#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
typedef struct login_pack
{
	char user[20];
	char pass[20];
} login_pack;
typedef struct header
{
	char size[10];
	char name[50];
} header;
typedef struct thread_arg
{
	int sock;
	pthread_mutex_t *locka, *lockl;
} thread_arg;
int login_req(int sock, pthread_mutex_t *lock)
{
	login_pack received;
	char user[20], pass[20], ans[2]; 
	FILE *data;
	int flag = 0;
	pthread_mutex_lock(lock);
	data = fopen("accounts","r");
	if(recv(sock,&received,sizeof(login_pack),0) < 0)
	{
		fprintf(stderr,"recv failed\n");
		return 0;
	}
	while(fscanf(data,"%s %s", user, pass)!=EOF)
	{
		if(strcmp(user,received.user) == 0 && strcmp(pass,received.pass) == 0)
		{
			fclose(data);
			pthread_mutex_unlock(lock);
			sprintf(ans,"%d",1);
			write(sock,ans, sizeof(char)*2);
			return 1;
		}
	}
	fclose(data);
	pthread_mutex_unlock(lock);
	sprintf(ans,"%d",0);
	write(sock,ans, sizeof(char)*2);
	return 0;
}
int check_validity(char *path)
{
	int i;
	char folder[30];
	if(path[0]!='/')
	{
		fprintf(stderr,"Invalid path\n");
		return 0;
	}
	for(i = 1; path[i]!='\0'&&path[i]!='/';i++)
		folder[i-1]=path[i];
	folder[i-1]='\0';
	if(path[i]=='/')
	{
		if(!(!strcmp(folder,"Sales") ||!strcmp(folder,"Promotions")||
			!strcmp(folder,"Offers") || !strcmp(folder,"Marketing")))
		{
			fprintf(stderr,"Invalid path\n");
			return 0;
		}
	}
	for(;path[i]!='\0';i++)
	{
		if(path[i]=='.'||path[i]=='/')
		{
			fprintf(stderr,"Invalid path\n");
			return 0;
		}
	}
	return 1;
}
void recv_file(int sock, pthread_mutex_t *lock)
{
	header h;
	int size;
	int r;
	char ans[2],buffer[512];
	FILE *file,*data;
	if(recv(sock,&h,sizeof(header),0)<0)
	{
		printf("Unsucessful transaction\n");
		fprintf(stderr, "recv failed");
		return;
	}
	if(!check_validity(h.name))
	{
		sprintf(ans,"%d",0);
		write(sock,ans, sizeof(char)*2);
		return;
	}
	file = fopen(h.name,"w");
	if(file == NULL)
	{
		printf("Error opening file %s\n",h.name);
		sprintf(ans,"%d",0);
		write(sock,ans, sizeof(char)*2);
		return;
	}
	sprintf(ans,"%d",1);
	write(sock,ans, sizeof(char)*2);
	size = atoi(h.size);
	while(size > 0)
	{
		
		r = recv(sock,buffer,size <512 ? size:512,0);
		if(r <= 0)
		{
			printf("Unsucessful transaction\n");
			fprintf(stderr, "recv failed");
			return;
		}
		printf("receiving\n");
		size -= r;
		fwrite(buffer,sizeof(char),r, file);
	}
	fclose(file);
	sprintf(ans,"%d",1);
	write(sock,ans, sizeof(char)*2);
	pthread_mutex_lock(lock);
	data = fopen("log","a");
	fprintf(data,"Added file %s in time %ld\n",h.name,time(NULL));
	fclose(data);
	pthread_mutex_unlock(lock);
	return;
}

thread_arg* create_arg(int sock, pthread_mutex_t *locka, pthread_mutex_t *lockl)
{
	thread_arg *new_a = (thread_arg*)malloc(sizeof(thread_arg));
	new_a->sock = sock;
	new_a->locka = locka;
	new_a->lockl = lockl;
	return new_a;
}
void *thread(void *arg)
{
	thread_arg *args = (thread_arg*)arg;
	if(!login_req(args->sock,args->locka))
	{
		close(args->sock);
		return NULL;
	}
	printf("loggin approved\n");
	recv_file(args->sock,args->lockl);
	free(arg);
	return NULL;
}
void server()
{
	thread_arg *arg;
	pthread_t threadid;
	pthread_mutex_t locka = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t lockl = PTHREAD_MUTEX_INITIALIZER;
	int socket_desc , client_sock , c;
    struct sockaddr_in server , client;     
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
        return;
    }     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed. Error");
        return;
    }
	for(;;)
	{
		//Listen
		listen(socket_desc , 10);
		 
		//Accept and incoming connection
		c = sizeof(struct sockaddr_in);
		 
		//accept connection from an incoming client
		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
		if (client_sock < 0)
		{
			perror("accept failed");
			continue;
		}
		arg = create_arg(client_sock, &locka, &lockl);
		pthread_create(&threadid,NULL,thread, (void*)arg);
	}
}
int main()
{
	server();
	return 0;
}
