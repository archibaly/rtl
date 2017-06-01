#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rtl_list.h>

struct student {
	char name[32];
	int age;
	struct rtl_list_head list;
};

int add(struct rtl_list_head *head, const char *name, int age)
{
	struct student *s;

	s = malloc(sizeof(struct student));
	if (!s)
		return -1;

	strncpy(s->name, name, sizeof(s->name));
	s->age = age;

	rtl_list_add(&s->list, head);

	return 0;
}

int del(struct rtl_list_head *head, const char *name)
{
	struct student *pos;
	struct student *tmp;

	rtl_list_for_each_entry_safe(pos, tmp, head, list) {
		if (strcmp(pos->name, name) == 0) {
			rtl_list_del(&pos->list);
			return 0;
		}
	}

	return -1;
}

struct student *find(struct rtl_list_head *head, const char *name)
{
	struct student *pos;

	rtl_list_for_each_entry(pos, head, list) {
		if (strcmp(pos->name, name) == 0) {
			return pos;
		}
	}

	return NULL;
}

int main()
{
	struct rtl_list_head head;

	rtl_list_head_init(&head);

	add(&head, "jacky", 25);
	add(&head, "tom", 13);

	struct student *s;
	s = find(&head, "jacky");

	if (s) {
		printf("s->name = %s, s->age = %d\n", s->name, s->age);
		del(&head, "jacky");
		free(s);
	} else {
		printf("can not find!\n");
	}

	s = find(&head, "tom");
	if (s) {
		printf("s->name = %s, s->age = %d\n", s->name, s->age);
		del(&head, "tom");
		free(s);
	} else {
		printf("can not find!\n");
	}

	return 0;
}
