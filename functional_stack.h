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

#ifndef _FUNCTIONAL_STACK_H
#define _FUNCTIONAL_STACK_H

#include "stack.h"

struct fstack_item_t {
	void *data;
	void (*func1)(void*);
	void (*func2)();
	// if 1, call func1 before func2, if 2, func2 then func1
	char order;
};
int fstack_init(struct stack_t *stack, size_t size);
void fstack_destroy(struct stack_t *stack);

void *fstack_peek(struct stack_t *stack);
void *fstack_pop(struct stack_t *stack);
void fstack_push(struct stack_t *stack, void *item);

int fstack_resize(struct stack_t *stack, size_t size);

#endif
