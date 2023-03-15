/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#define MAX 50
#define NTHREADS 10
#define SBUFSIZE 500

typedef struct{

	int *buf;
	int n;
	int front;
	int rear;
	sem_t mutex;
	sem_t slots;
	sem_t items;

}sbuf_t;

typedef struct item{
	int ID;
	int left_stock;
	int price;
	int readcnt;
	sem_t mutex;


}Item;

typedef struct items_BT{

	int item_cnt;
	Item *root[MAX];

}Items_BT;


sbuf_t sbuf;
static sem_t mutex;
static sem_t w;
static sem_t fd_mutex;
int connfd_cnt = 0;
static int byte_cnt;
Items_BT stock_info;



void echo(int connfd);


void sbuf_init(sbuf_t *sp, int n)
{
	sp->buf = Calloc(n, sizeof(int));
	sp->n = n;
	sp->front = sp->rear = 0;
	Sem_init(&sp->mutex,0,1);
	Sem_init(&sp->slots,0,n);
	Sem_init(&sp->items, 0, 0);
}

void sbuf_deinit(sbuf_t *sp)
{
	Free(sp->buf);
}
void sbuf_insert(sbuf_t *sp, int item)
{
	P(&sp->slots);
	P(&sp->mutex);
	sp->buf[(++sp->rear)%(sp->n)] = item;
	V(&sp->mutex);
	V(&sp->items);
}
int sbuf_remove(sbuf_t *sp)
{
	int item;
	P(&sp->items);
	P(&sp->mutex);
	item = sp->buf[(++sp->front)%(sp->n)];
	V(&sp->mutex);
	V(&sp->slots);
	return item;
}



void create_BT(FILE *fp,Items_BT *items)
{
	Item *new_node = NULL;
	int id, left, price;
	items = (Items_BT *)malloc(sizeof(Items_BT));
	items->item_cnt = 0;

	stock_info.item_cnt = 0;

	
	fp = fopen("./stock.txt", "r");


	while(1)
	{

		int idx = stock_info.item_cnt;
		int ret;
		ret = fscanf(fp,"%d %d %d", &id, &left, &price);
		if(ret <0)
			break;

		new_node = (Item *)malloc(sizeof(Item));
		new_node->ID = id;
		new_node->left_stock = left;
		new_node->price = price;
		new_node->readcnt = 0;
		Sem_init(&new_node->mutex,0,1);

		stock_info.root[++idx] = new_node;
		stock_info.item_cnt++;
		


	}
	fclose(fp);

}



void show(char buf[])
{
	int cnt = stock_info.item_cnt;
	char con_str[MAXLINE];
	
	for(int i=1;i<=cnt;i++)
	{
		char str[MAXLINE];
		stock_info.root[i]->readcnt++;
		sprintf(str,"%d %d %d\n",stock_info.root[i]->ID, stock_info.root[i]->left_stock, stock_info.root[i]->price);
		if(i == 1)
			strcpy(con_str,str);
		else
			strcat(con_str,str);
	}
	strcpy(buf,con_str);

}
void sell(int id, int sell_cnt)
{

	int cnt = stock_info.item_cnt;
	int i;
	for(i = 1; i<=cnt;i++)
	{
		if(stock_info.root[i]->ID == id)
		{
			P(&stock_info.root[i]->mutex);
			stock_info.root[i]->left_stock += sell_cnt;

			stock_info.root[i]->readcnt++;
			V(&stock_info.root[i]->mutex);
			break;
		}
	}
	return;


}

int buy(int id, int buy_cnt)
{
	int cnt = stock_info.item_cnt;
	int i;
	int flag = 1;
	for(i = 1; i<=cnt;i++)
	{
		if(stock_info.root[i]->ID == id)
		{
			
			P(&stock_info.root[i]->mutex);
			if(stock_info.root[i]->left_stock >=  buy_cnt){
				stock_info.root[i]->left_stock -= buy_cnt;
			}
			else
				flag = 0;
		
			stock_info.root[i]->readcnt++;
			V(&stock_info.root[i]->mutex);
			break;
		}
	}
	return flag;
}
void save_stock(void)
{

	FILE *fp =fopen("./stock.txt","w");
	Item *tmp =NULL;
	for(int i =1;i<=stock_info.item_cnt;i++)
	{
		tmp = stock_info.root[i];

		fprintf(fp,"%d %d %d\n",tmp->ID, tmp->left_stock, tmp->price);
	}

	fclose(fp);

}
static void init_echo_cnt(void)
{
	Sem_init(&mutex,0,1);
	Sem_init(&w,0,1);
	byte_cnt = 0;
}



void echo_cnt(int connfd)
{
	int n;
	char buf[MAXLINE];
	rio_t rio;
	static pthread_once_t once =PTHREAD_ONCE_INIT;

	Pthread_once(&once, init_echo_cnt);
	Rio_readinitb(&rio,connfd);
	while((n = Rio_readlineb(&rio,buf,MAXLINE)) !=0){

		char ret_buf[MAXLINE];
		printf("server received %d bytes on fd %d\n", n, connfd);
		if(!strcmp(buf,"show\n")){
			show(ret_buf);
		}


		else if(!strcmp(buf,"exit\n"))
		{
			break;
		}

		else{
			char *ptr = strtok(buf, " ");
			if(!strcmp(ptr, "buy"))
			{
				int id, cnt;
				int flag;
				id = atoi(strtok(NULL, " "));
				cnt = atoi(strtok(NULL, " "));
				flag = buy(id,cnt);
						
				if(flag)
					strcpy(ret_buf,"[buy] success\n");
				else
					strcpy(ret_buf,"Not enough left stock\n");
			}
			else if(!strcmp(ptr, "sell"))
			{

				int id, cnt;
				id = atoi(strtok(NULL, " "));
				cnt = atoi(strtok(NULL, " "));
				sell(id,cnt);
				strcpy(ret_buf, "[sell] success\n");
			}
		}
				
				

		Rio_writen(connfd,ret_buf,MAXLINE);
	}

}

void *thread(void *vargp)
{
	Pthread_detach(pthread_self());
	while(1)
	{
		int connfd= sbuf_remove(&sbuf);
		echo_cnt(connfd);
		P(&fd_mutex);
		Close(connfd);
		connfd_cnt--;
		V(&fd_mutex);
		if(connfd_cnt == 0)
		{	
			save_stock();
			return NULL;
		}
	}


}
int main(int argc, char **argv) 
{
    int i, listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
	pthread_t tid;
	


	FILE *fp = NULL;
	Items_BT *bt = NULL;

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
	create_BT(fp, bt);

    listenfd = Open_listenfd(argv[1]);
	sbuf_init(&sbuf, SBUFSIZE);
	for(i=0;i<NTHREADS;i++)
		Pthread_create(&tid,NULL,thread,NULL);


	Sem_init(&fd_mutex, 0, 1);
	while(1)
	{

		clientlen=sizeof(struct sockaddr_storage);
		connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
		P(&fd_mutex);
		sbuf_insert(&sbuf,connfd);
		connfd_cnt++;
		V(&fd_mutex);


	}
		
    exit(0);
}
/* $end echoserverimain */





