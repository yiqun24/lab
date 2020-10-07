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
	while (p->next->next != NULL)
	{
		p = p->next;
	}
	q = p->next;
	p->next = NULL;
	WP *f = head;
	if (f == NULL)
	{
		head = q;
		f = head;
	}
	else
	{
		while (f->next != NULL)
			f = f->next;
		f->next = q;
	}
	return q;
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
	wp->exp = NULL;
	wp->value = 0;
}
int check_wp()
{
	uint32_t current_value;
	bool success, flag = true;
	WP *p = head;
	printf("OK\n");
	if (p == NULL)
	{
			printf("OK\n");
		return 0;
	}
	else
	{
		while (p->next != NULL)
		{
			current_value = expr(p->exp, &success);
			if (!success)
				assert(0);
			if (current_value != p->value)
			{
				p->value = current_value;
				printf("value has changed:     %s         :     %d\n", p->exp, p->value);
				flag = false;
			}
		}
	}
	if (flag)
		return 0;
	else
		return -1;
}
