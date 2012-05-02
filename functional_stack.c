/*
	fuse-google-drive: a fuse filesystem wrapper for Google Drive
	Copyright (C) 2012  James Cline

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
 	published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include <stdlib.h>
#include <stdio.h>

#include "stack.h"
#include "functional_stack.h"

int fstack_init(struct stack_t *stack, size_t size)
{
	int ret = stack_init(stack, size);
}

void fstack_destroy(struct stack_t *stack)
{
	while(stack->size)
		free(stack_pop(stack));
	stack_destroy(stack);
}

void *fstack_peek(struct stack_t *stack)
{
	struct fstack_item_t *item;
	item = (struct fstack_item_t*) stack_peek(stack);
	if(item)
	{
		switch(item->order)
		{
			case 1:
				if(item->func1) item->func1(item->data);
				if(item->func2) item->func2(item->data);
				break;
			case 2:
				if(item->func2) item->func2(item->data);
				if(item->func1) item->func1(item->data);
				break;
		}
	}
	return item->data;
}

void *fstack_pop(struct stack_t *stack)
{
	struct fstack_item_t *item;
	item = (struct fstack_item_t*) stack_pop(stack);
	if(item)
	{
		switch(item->order)
		{
			case 1:
				if(item->func1) item->func1(item->data);
				if(item->func2) item->func2(item->data);
				break;
			case 2:
				if(item->func2) item->func2(item->data);
				if(item->func1) item->func1(item->data);
				break;
		}
	}
	void *data = item->data;
	free(item);
	return data;
}

int fstack_push(struct stack_t *stack, void *data, void (*func1)(void*),
                void (*func2)(), char order)
{
	struct fstack_item_t *item;
	item = (struct fstack_item_t*) malloc(sizeof(struct fstack_item_t));
	if(!item)
		return 1;
	item->data = data;
	item->func1 = func1;
	item->func2 = func2;
	if(order != 1 && order != 2)
	{
		fprintf(stderr, "order parameter must be either 1 or 2\n");
		free(item);
		return 2;
	}
	item->order = order;

	return stack_push(stack, item);
}

int fstack_resize(struct stack_t *stack, size_t size)
{
	return stack_resize(stack, size);
}
