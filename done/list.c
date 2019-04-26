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
int is_empty_list(const list_t* this){// what does well formed mean??
	M_REQUIRE_NON_NULL(this);
	return (this->front==this->back&&this->back==NULL);
	
}

void init_list(list_t* this){// shoud not allocate memory!!!
//	assert (M_REQUIRE_NON_NULL(this)); should we assert??
	
	this->front= NULL;
	this->back=NULL;
}
void clear_list(list_t* this){// should we do this=NULL?
	//M_REQUIRE_NON_NULL(this); // should we assert ?
	free(this->front);
	free(this->back);
	init_list(this);
	
}


node_t* push_back(list_t* this, const list_content_t* value){//do we use Least recently used?
	//M_REQUIRE_NON_NULL(this);
	//M_REQUIRE_NON_NULL(value);
	node_t* n; // should we do a pointer and calloc??
	n= malloc(sizeof(*n));
	if(n!=NULL){
	n->value = *value;
	n->previous=this->back;
	n->next=NULL;
	this->back->next=n;
	this->back=n;
	}
	return n;
	
}

node_t* push_front(list_t* this, const list_content_t* value){
	//assert (M_REQUIRE_NON_NULL(this));
	//M_REQUIRE_NON_NULL(value);
	node_t* n; 
	n= malloc(sizeof(*n));
		if(n!=NULL){
	n->value = *value;
	n->previous=NULL;
	n->next=this->front;
	this->front->previous=n;
	this->front=n;
	}
	return n;
}

void pop_back(list_t* this){
	//M_REQUIRE_NON_NULL(this);
	if(!is_empty_list(this)){
		node_t* last = this->back;
		this->back= last->previous;
		free(last);
	}
}
void pop_front(list_t* this){
	//M_REQUIRE_NON_NULL(this);
	if(!is_empty_list(this)){
		node_t* first = this->front;
		this->front= first->next;
		free(first);
	}
}


void move_back(list_t* this, node_t* node){
	node->previous->next=node->next;
	node->next->previous=node->previous;
	assert (NULL != push_back(this, &node->value));// should we assert?? 
}
int print_list(FILE* stream, const list_t* this){
	M_REQUIRE_NON_NULL(this);
	M_REQUIRE_NON_NULL_CUSTOM_ERR(stream, ERR_IO);
	uint32_t sum=0;
	sum+=fprintf(stream, "(");// how to check if correct number read? assert or if <0 and do what???
	for_all_nodes(n, this){
		sum+=print_node(stream, n->value);
		if(n->next!=NULL)
		sum+=fprintf(stream, ", ");
	}
	sum+=fprintf(stream, ")");
	return sum;
}

int print_reverse_list(FILE* stream, const list_t* this){
	M_REQUIRE_NON_NULL(this);
	M_REQUIRE_NON_NULL_CUSTOM_ERR(stream, ERR_IO);
	uint32_t sum=0;
	sum+=fprintf(stream, "(");// how to check if correct number read? assert or if <0 and do what???
	for_all_nodes_reverse(n, this){
		sum+=print_node(stream, n->value);
		if(n->previous!=NULL)
		sum+=fprintf(stream, ", ");
	}
	sum+=fprintf(stream, ")");
	return sum;
}
