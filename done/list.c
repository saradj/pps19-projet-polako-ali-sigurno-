/**
 * @file list.c
 * @brief Doubly linked lists for PPS-i7Cache project
 *
 * @author
 * @date 2019
 */
#include "list.h"
#include "error.h"
#include <stdlib.h>
#include <assert.h>

int is_empty_list(const list_t *this)
{
	if (this == NULL) // not well formed
		return 1;
	if (this->back == NULL)
	{
		if (this->front == NULL)
			return 1; // empty list
		return 1;	 // not well formed front not null and back null
	}
	else if (this->front == NULL)
		return 1; // not well formed front null back not null
	return 0;	 // well formed both front and back are non null
}

void init_list(list_t *this) // initialising the list to the empty list
{
	if (this != NULL)
	{
		this->front = NULL;
		this->back = NULL;
	}
}
void clear_list(list_t *this) // freeing all the nodes linked in the list
{
	if (this != NULL)
	{
		node_t *next = NULL;
		for (node_t *tmp = this->front; tmp != NULL; tmp = next)
		{
			next = tmp->next;
			free(tmp);
		}
		init_list(this);
	}
}

node_t *push_back(list_t *this, const list_content_t *value)
{
	if (this == NULL || value == NULL)
	{
		return NULL; // if the arguments are not valid return NULL, can not perform operation
	}
	node_t *n = malloc(sizeof(node_t));
	if (n != NULL)
	{
		n->value = *value;
		n->previous = this->back;
		n->next = NULL;
		if (is_empty_list(this)) // special case for the empty list
		{
			this->back = n;
			this->front = n;
		}
		else
		{
			this->back->next = n;
			this->back = n; // n is the last node in the list now
		}
	}
	return n;
}

node_t *push_front(list_t *this, const list_content_t *value)
{
	if (this == NULL || value == NULL)
	{
		return NULL; // if the arguments are not valid return NULL, can not perform operation
	}
	node_t *n = malloc(sizeof(*n));
	if (n != NULL)
	{
		n->value = *value;
		n->previous = NULL;
		n->next = this->front;
		if (is_empty_list(this)) // special case for the empty list
		{
			this->back = n;
			this->front = n;
		}
		else
		{
			this->front->previous = n;
			this->front = n; // n is the first node in the list now
		}
	}
	return n;
}

void pop_back(list_t *this)
{
	if (this != NULL)
	{
		if (!is_empty_list(this))
		{
			node_t *last = this->back;
			this->back = last->previous;
			if (last->previous == NULL) // if there was just one element left
				this->front = NULL;
			else
				last->previous->next = NULL;
			free(last);
		}
		else
			fprintf(stderr, "List is empty nothing to pop");
	}
}
void pop_front(list_t *this)
{
	//M_REQUIRE_NON_NULL(this);
	if (!is_empty_list(this))
	{
		node_t *first = this->front;
		this->front = first->next;
		if (first->next == NULL) // if there was just one element left
			this->back = NULL;
		else
			first->next->previous = NULL;
		free(first);
	}
	else
		fprintf(stderr, "List is empty nothing to pop");
}

void move_back(list_t *this, node_t *node)
{
	if (this != NULL && node != NULL)
	{
		if (node->next == NULL)
			return; // nothing to be done if one node is already last
		if (node->previous == NULL)
		{ // if it's the first node
			this->front = node->next;
			node->next->previous = NULL;
		}
		else
		{
			node->previous->next = node->next;
			node->next->previous = node->previous;
		}
		list_content_t val = node->value;
		free(node);							   // freeing the node and calling push back with it's value to create a new node and add it to the end
		assert(NULL != push_back(this, &val)); // asserting so that we exit if there was an error in push back
	}
}

int print_list(FILE *stream, const list_t *this)
{
	if (this == NULL || stream == NULL)
		return 0;
	list_content_t sum = 0;
	sum += fprintf(stream, "(");
	for_all_nodes(n, this)
	{
		sum += print_node(stream, n->value);
		if (n->next != NULL)
			sum += fprintf(stream, ", ");
	}
	sum += fprintf(stream, ")");
	return sum;
}

int print_reverse_list(FILE *stream, const list_t *this)
{
	if (this == NULL || stream == NULL)
		return 0;
	list_content_t sum = 0;
	sum += fprintf(stream, "("); // how to check if correct number read? assert or if <0 and do what???
	for_all_nodes_reverse(n, this)
	{
		sum += print_node(stream, n->value);
		if (n->previous != NULL)
			sum += fprintf(stream, ", ");
	}
	sum += fprintf(stream, ")");
	return sum;
}
