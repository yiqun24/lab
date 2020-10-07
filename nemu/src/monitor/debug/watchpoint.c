#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool()
{
	int i;
	for (i = 0; i < NR_WP; i++)
	{
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}
WP *new_wp()
{
	WP *p, *q;
	p = free_;
	if (p == NULL)
		assert(0);
	free_ = free_->next;
	p->next = NULL;
	q = head;
	if (q == NULL)
	{
		head = p;
		q = head;
	}
	else
	{
		while (q->next != NULL)
			q = q->next;
		q->next = p;
	}
	return p;
}

void free_wp(WP *wp) //?
{
	WP *p, *q;
	p = head;
	if (head == NULL)
		assert(0);
	if (head->NO == wp->NO)
		head = head->next;
	else
	{
		while (p->next != NULL && p->next->NO != wp->NO)
			p = p->next;
		if (p->next == NULL)
			assert(0);
		else
		{
			p->next = p->next->next;
		}
	}
	q = free_;
	if (q == NULL)
		free_ = wp;
	else
	{
		while (q->next != NULL)
			q = q->next;
		q->next = wp;
	}
	wp->next = NULL;
	wp->exp[0] = '\0';
	wp->value = 0;
}
bool check_wp()
{
	uint32_t current_value;
	bool success, flag = true;
	WP *p = head;
	while (p != NULL)
	{
		current_value = expr(p->exp, &success);
		if (!success)
			assert(0);
		if (current_value != p->value)
		{
			printf("value of watchpoint %d has changed\n", p->NO);
			printf("old value:  %x   %d  \n", p->value, p->value);
			p->value = current_value;
			printf("new value:  %x   %d \n", p->value, p->value);
			flag = false;
		}
		p = p->next;
	}
	return flag;
}
void delete_wp(int n)
{
	WP *p = &wp_pool[n];
	free_wp(p);
}
void info_wp()
{
	WP *p = head;
	while (p != NULL)
	{
		printf("watchpoint %d : %s = %d\n", p->NO, p->exp, p->value);
		p = p->next;
	}
}
