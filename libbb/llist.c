/* vi: set sw=4 ts=4: */
/*
 * linked list helper functions.
 *
 * Copyright (C) 2003 Glenn McGrath
 * Copyright (C) 2005 Vladimir Oleynik
 * Copyright (C) 2005 Bernhard Reutner-Fischer
 * Copyright (C) 2006 Rob Landley <rob@landley.net>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */
#include "libbb.h"

/* Add data to the end of the linked list.  */
void FAST_FUNC llist_add_to_end(llist_t **list_head, void *data)
{
	while (*list_head)
		list_head = &(*list_head)->link;
	*list_head = xzalloc(sizeof(llist_t));
	(*list_head)->data = data;
	/*(*list_head)->link = NULL;*/
}

/* Remove first element from the list and return it */
void* FAST_FUNC llist_pop(llist_t **head)
{
	void *data = NULL;
	llist_t *temp = *head;

	if (temp) {
		data = temp->data;
		*head = temp->link;
		free(temp);
	}
	return data;
}

