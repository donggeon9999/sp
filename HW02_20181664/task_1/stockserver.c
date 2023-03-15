/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#define MAX 50
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

typedef struct {
	int maxfd;
	fd_set read_set;
	fd_set ready_set;
	int nready;
	int maxi;
	int clientfd[FD_SETSIZE];
	rio_t clientrio[FD_SETSIZE];


}pool;




Items_BT stock_info;
void echo(int connfd);


void save_stock(void)
{
	FILE *fp = fopen("./stock.txt","w");
	Item *tmp = NULL;
	for(int i =1; i<=stock_info.item_cnt;i++)
	{
		tmp= stock_info.root[i];
		fprintf(fp,"%d %d %d\n",tmp->ID,tmp->left_stock, tmp->price);
	}
	fclose(fp);

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
		
		stock_info.root[++idx] = new_node;
		stock_info.item_cnt++;
		


	}

}

void init_pool(int listenfd, pool *p)
{

	int i;
	p->maxi = -1;
	for(i =0;i<FD_SETSIZE;i++)
		p->clientfd[i] = -1;
	p->maxfd = listenfd;
	FD_ZERO(&p->read_set);
	FD_SET(listenfd, &p->read_set);


}

void add_client(int connfd, pool *p){

	int i;
	p->nready--;
	for(i =0;i<FD_SETSIZE;i++)
	{
		if(p->clientfd[i] <0)
		{
			p->clientfd[i] = connfd;
			Rio_readinitb(&p->clientrio[i],connfd);

			FD_SET(connfd, &p->read_set);

			if(connfd>p->maxfd)
				p->maxfd = connfd;
			if(i>p->maxi)
				p->maxi = i;
			break;


		}
	}

	if(i == FD_SETSIZE)
		app_error("add_client error : Too many clients\n");


}
void show(char buf[])
{
	int cnt = stock_info.item_cnt;
	char con_str[MAXLINE];
	strcpy(con_str,"show\n");
	for(int i=1;i<=cnt;i++)
	{
		char str[MAXLINE];
		sprintf(str,"%d %d %d\n",stock_info.root[i]->ID, stock_info.root[i]->left_stock, stock_info.root[i]->price);
	//	printf("str : %s", str);
		if(i == 1)
			strcpy(con_str,str);
		else
			strcat(con_str,str);
	}
	strcpy(buf,con_str);
//	printf("%s",buf);




}
void sell(int id, int sell_cnt)
{
	int cnt = stock_info.item_cnt;
	int i;
	char buf[MAXLINE];
	for(i = 1; i<=cnt;i++)
	{
		if(stock_info.root[i]->ID == id)
		{
			stock_info.root[i]->left_stock += sell_cnt;
			break;
		}
	}
	show(buf);
	return;


}

int buy(int id, int buy_cnt)
{
	int cnt = stock_info.item_cnt;
	int i;
	int flag = 1;
	
	char buf[MAXLINE];
	for(i = 1; i<=cnt;i++)
	{
		if(stock_info.root[i]->ID == id)
		{
			if(stock_info.root[i]->left_stock >=  buy_cnt)
				stock_info.root[i]->left_stock -= buy_cnt;
			else
				flag = 0;
			break;
		}
	}
	show(buf);
	return flag;
}

int is_connect(pool *p)
{
	int i, connfd;
	
	for(i=0;(i<=p->maxi) && (p->nready >0 );i++){

		connfd = p->clientfd[i];
			
		if(connfd >0)
			return 1;
	}
	return 0;

}
void check_clients(pool *p)
{

	int i, connfd, n;
	char buf[MAXLINE];
	char ret_buf[MAXLINE];
	rio_t rio;
	for(i=0;(i<=p->maxi) && (p->nready >0 );i++){

		connfd = p->clientfd[i];
		rio = p->clientrio[i];

		if((connfd >0) && (FD_ISSET(connfd, &p->ready_set))){

			p->nready--;
			if((n = Rio_readlineb(&rio, buf, MAXLINE)) !=0){


				printf("Server received %d bytes\n",n);
				if(!strcmp(buf,"show\n")){
					show(ret_buf);}
				else if(!strcmp(buf,"exit\n"))
				{
					Close(connfd);
					FD_CLR(connfd, &p->read_set);
					p->clientfd[i] = -1;
					save_stock();

					if(!is_connect(p))
					{
						save_stock();
					}
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
					if(!strcmp(ptr, "sell"))
					{

						int id, cnt;
						id = atoi(strtok(NULL, " "));
						cnt = atoi(strtok(NULL, " "));
						sell(id,cnt);
						strcpy(ret_buf, "[sell] success\n");
					}
				}
				//printf("%s %d",ret_buf, sizeof(ret_buf));
				Rio_writen(connfd,ret_buf,MAXLINE);
				ret_buf[0] = '\0';
				
			}
			else{
				Close(connfd);
				FD_CLR(connfd, &p->read_set);
				p->clientfd[i] = -1;
				save_stock();
				if(!is_connect(p))
				{
					save_stock();
				}
					

			}


		
		}

	}

}

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
	static pool pool;

	FILE *fp = NULL;
	Items_BT *bt = NULL;
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
	create_BT(fp, bt);



		


    listenfd = Open_listenfd(argv[1]);
	init_pool(listenfd, &pool);


    while (1) {
		pool.ready_set = pool.read_set;
		pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);
		if(FD_ISSET(listenfd, &pool.ready_set)){

			clientlen = sizeof(struct sockaddr_storage);
			connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
			add_client(connfd, &pool);

		}
		check_clients(&pool);

    }
    exit(0);
}
/* $end echoserverimain */





