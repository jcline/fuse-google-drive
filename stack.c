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
#include "stack.h"
#include <stdlib.h>
#include <string.h>

int stack_init(struct stack_t *stack, size_t size)
{
	stack->size = 0;
	stack->store = NULL;
	stack->reserved = 0;
	int ret = stack_resize(stack, size);
	stack->top = stack->store;
	return ret;
}

void stack_destroy(struct stack_t *stack)
{
	stack->top = NULL;
	stack->size = 0;
	stack->reserved = 0;
	free(stack->store);
}


void *stack_peek(struct stack_t *stack)
{
	if(!stack->size)
		return NULL;
	return *(stack->top);
}

void *stack_pop(struct stack_t *stack)
{
	if(!stack->size)
		return NULL;
	void *ret = *stack->top;
	if(--stack->size > 0)
		--stack->top;
	return ret;
}

int stack_push(struct stack_t *stack, void *item)
{
	if(!(stack->reserved - ++stack->size))
		if(stack_resize(stack, stack->reserved+10))
			return --stack->size;
	if(stack->size == 0)
		*(stack->top) = item;
	else
		*(++stack->top) = item;
	return 0;
}

int stack_resize(struct stack_t *stack, size_t size)
{
	if(size <= stack->reserved)
		return 0;
	void **result = (void**) realloc(stack->store, sizeof(void*) * size);
	if(result)
	{
		memset(result + stack->reserved, 0, sizeof(void*) * (size - stack->reserved));
		stack->reserved = size;
		stack->store = result;
		stack->top = stack->store;
		if(stack->size)
		{
			stack->top += stack->size - 1;
		}
		return 0;
	}
	return 1;
}
