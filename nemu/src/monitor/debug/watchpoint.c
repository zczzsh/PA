#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

WP* new_wp()
{
	WP* point=free_;
	free_=free_->next;
	point->next=NULL;
	return point;
}

void set_wp(char *args)
{
    WP* new=new_wp();
	strcpy(new->express,args);
	bool success=true;
	new->value=expr(args,&success);
	if(head==NULL)
	{
		head=new;
	}
	else
	{
		WP* p=head;
		while(p->next!=NULL) p=p->next;
		p->next=new;
	}
}

void free_wp(int N)
{
    WP* check=head;
	WP* front=head;
	if(check==NULL) return;
	if(check==head) head=check->next;
	if(check!=NULL && check->NO!=N)
	{
		front=check;
		check=check->next;
	}
	front->next=check->next;
	check->next=free_;
	free_=check;	
}

void print_wp()
{
	if(head!=NULL)
	{
		WP* p=head;
		while(p->next!=NULL)
		{
			printf("%d\t %s\t %d\n",p->NO,p->express,p->value);
			p=p->next;
		}
		printf("%d\t %s\t %d\n",p->NO,p->express,p->value);
	}
}

bool check_wp()
{
	if(head!=NULL)
	{
		WP* check=head;
		bool success=true;
		while(check->next!=NULL)
		{
            if(check->value!=expr(check->express,&success))
			  return true;
		}
	}
	return false;
}
/* TODO: Implement the functionality of watchpoint */


