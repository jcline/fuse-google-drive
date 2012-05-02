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

#ifndef _STACK_H
#define _STACK_H

struct stack_t {
	struct stack_t *top;

	size_t reserved;
	size_t size;

	void *store;
};

int stack_init(struct stack_t *stack, size_t size);
void stack_destroy(struct stack_t *stack);


void *stack_peek(struct stack_t *stack);
void *stack_pop(struct stack_t *stack);
void stack_push(struct stack_t *stack, void *item);

int stack_resize(struct stack_t *stack, size_t size);

#endif
