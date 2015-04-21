/*- -*- mode: c; c-basic-offset: 8; -*-
 *
 * This file implements a hash table, which is an efficient data structure for
 * maintaining a collection of "key"-to-"value" mappings.  This hash table, in
 * particular, maintains mappings from strings to opaque objects. 
 *
 * This file is placed in the public domain by its authors, Alan L. Cox and
 * Kaushik Kumar Ram.
 */

#include <assert.h>
#include <stdlib.h> /* For malloc, free, and calloc. */
#include <string.h> /* For strcmp. */

/*
 * Including "hash_table.h" here enforces consistency between the interface
 * and the implementation.  Any inconsistency will result in a compilation
 * error.
 */
#include "hash_table.h"

/*
 * Requires:
 *  Nothing.
 *
 * Effects:
 *  Computes the Fowler / Noll / Vo (FNV) hash of the string "key".
 */
static unsigned int
hash_value(const char *key)
{
	unsigned int c, hval = 33554467U;	/* A "magic" number. */

	/*
	 * The following cast is meant to prevent the sign extension of
	 * "*string"'s value before it is assigned to "c".  For example,
	 * suppose that "*string" is 0xbc.  Then, without the cast, "c" would
	 * be assigned 0xffffffbc instead of 0x000000bc.  The former value,
	 * 0xffffffbc, is problematic because "hval ^= c" is only supposed
	 * to change the least significant 8 bits of "hval".
	 */ 
	while ((c = *(const unsigned char *)key++) != 0) {
		hval *= 0x01000193U;		/* A "magic" prime number. */
		hval ^= c;
	}
	return (hval);
}

/*
 * A hash table mapping:
 *
 *  Stores the association or mapping from a string "key" to its opaque
 *  "value".  Each mapping is an element of a singly-linked list.  This list
 *  is called a collision chain.
 */
struct hash_table_mapping {
	/*
	 * The "key" is a pointer to a string.
	 */
	const char *key;
	/* 
	 * The "value" is a pointer to an object of an unknown type, because
	 * the hash table doesn't need to know the type in order to store and
	 * return the pointer.
	 */
	void *value;
	/*
	 * The pointer to the next element in the same collision chain.
	 */
	struct hash_table_mapping *next; 
};

/*
 * A hash table:
 *
 *  Stores a collection of "key"-to-"value" mappings.  Each mapping is an
 *  element of a collision chain.  For efficiency, the number of collision
 *  chains is kept in proportion to the number of mappings so that the average
 *  length of a collision chain remains constant no matter how many mappings
 *  the hash table contains.
 */
struct hash_table {
	/* 
	 * The array of collision chains.  Really, this is a pointer to an
	 * array of pointers.  Each element of that array is the head of a
	 * collision chain.
	 */
	struct hash_table_mapping **head;
	/*
	 * The number of collision chains.
	 */
	unsigned int size;
	/*
	 * The number of mappings in the hash table.
	 */
	unsigned int occupancy;		
	/*
	 * The upper bound on the average collision chain length that is
	 * allowed before the number of collision chains is increased.
	 */
	double load_factor;
};

/*
 * The initial number of collision chains in a hash table:
 */
#define	INITIAL_SIZE	2

/*
 * Requires:
 *  "load_factor" must be greater than zero.
 *
 * Effects:
 *  Creates a hash table with the upper bound "load_factor" on the average
 *  length of a collision chain.  Returns a pointer to the hash table if it
 *  was successfully created and NULL if it was not.
 */
struct hash_table * 
hash_table_create(double load_factor)
{
	struct hash_table *ht;

	assert(load_factor > 0.0);
	ht = malloc(sizeof(struct hash_table));
	if (ht == NULL)
		return (NULL);
	/* 
	 * Allocate and initialize the hash table's array of pointers.  In
	 * contrast to malloc(), calloc() initializes each of the returned
	 * memory locations to zero, effectively initializing the head of
	 * every collision chain to NULL.  
	 */
	ht->head = calloc(INITIAL_SIZE, sizeof(struct hash_table_mapping *));
	if (ht->head == NULL) {
		free(ht);
		return (NULL);
	}
	ht->size = INITIAL_SIZE;
	ht->occupancy = 0;
	ht->load_factor = load_factor;
	return (ht);
}

/*
 * Requires:
 *  Nothing.
 *
 * Effects:
 *  Destroys a hash table, but first if the function pointer "destructor" is
 *  not NULL calls "destructor" on each mapping in the hash table.  The
 *  mapping's key, its value, and the latest value of the pointer "cookie"
 *  are passed to "destructor" as arguments.  The return value is assigned to
 *  "cookie", becoming "cookie"'s latest value.  In other words,
 *	cookie = destructor(key, value, cookie);
 *  The order in which the mappings are passed to "destructor" is not
 *  defined.  Returns "cookie"'s latest value.
 */
void *
hash_table_destroy(struct hash_table *ht,
    void *(*destructor)(const char *key, void *value, void *cookie),
    void *cookie)
{
	struct hash_table_mapping *elem, *next;
	unsigned int index;

	/*
	 * Iterate over the collision chains.
	 */
	for (index = 0; index < ht->size; index++) {
		/*
		 * Destroy each of the mappings in a collision chain.
		 */
		for (elem = ht->head[index]; elem != NULL; elem = next) {
			next = elem->next;
			if (destructor != NULL)
				cookie = (*destructor)(elem->key, elem->value,
				    cookie);
			free(elem);
		}
	}
	free(ht->head);
	free(ht);
	return (cookie);
}

/*
 * Requires:
 *  "new_size" must be greater than zero.
 *
 * Effects:
 *  Grows (or shrinks) the number of collision chains in the hash table "ht"
 *  to "new_size".  Returns 0 if the increase (or decrease) was successful and
 *  -1 if it was not.
 */
static int
hash_table_resize(struct hash_table *ht, unsigned int new_size)
{
	struct hash_table_mapping **new_head;
	struct hash_table_mapping *elem, *next;
	unsigned int index, new_index;

	assert(new_size > 0);
	/*
	 * Does the hash table already have the desired number of collision
	 * chains?  If so, do nothing.
	 */
	if (ht->size == new_size)
		return (0);
	/* 
	 * Allocate and initialize the hash table's new array of pointers.
	 * In contrast to malloc(), calloc() initializes each of the returned
	 * memory locations to zero, effectively initializing the head of
	 * every collision chain to NULL.
	 */
	new_head = calloc(new_size, sizeof(struct hash_table_mapping *));
	if (new_head == NULL)
		return (-1);
	/*
	 * Iterate over the collision chains.
	 */
	for (index = 0; index < ht->size; index++) {
		/*
		 * Transfer each of the mappings in the old collision chain to
		 * its new collision chain.  Typically, this results in two
		 * shorter collision chains.
		 */
		for (elem = ht->head[index]; elem != NULL; elem = next) {
			/*
			 * The next mapping pointer is overwritten when the
			 * mapping is inserted into its new collision chain,
			 * so the next mapping pointer must be saved in order
			 * that the loop can go on.
			 */
			next = elem->next;
			/*
			 * Compute the array index of the new collision chain
			 * into which the mapping should be inserted.
			 */
			new_index = hash_value(elem->key) % new_size;
			/*
			 * Insert the mapping into its new collision chain.
			 */
			elem->next = new_head[new_index];
			new_head[new_index] = elem;
		}
	}
	/*
	 * Free the old array and replace it with the new array.
	 */
	free(ht->head);
	ht->head = new_head;
	ht->size = new_size;
	return (0);
}

/*
 * Requires:
 *  "key" is not already in "ht".
 *  "value" is not NULL.
 *
 * Effects:
 *  Creates an association or mapping from the string "key" to the pointer
 *  "value" in the hash table "ht".  Returns 0 if the mapping was successfully
 *  created and -1 if it was not.
 */
int
hash_table_insert(struct hash_table *ht, const char *key, void *value)
{
	struct hash_table_mapping *elem;
	unsigned int index;

	assert(hash_table_lookup(ht, key) == NULL);
	assert(value != NULL);
	/*
	 * Should the number of collision chains be increased?
	 */
	if (ht->occupancy >= ht->load_factor * ht->size) {
		/*
		 * Try to double the number of collision chains.
		 */
		if (hash_table_resize(ht, ht->size * 2) == -1)
			return (-1);
	}
	/*
	 * Allocate memory for a new mapping and initialize it.
	 */
	elem = malloc(sizeof(struct hash_table_mapping));
	if (elem == NULL)
		return (-1);
	elem->key = key;
	elem->value = value;
	/*
	 * Compute the array index of the collision chain in which the mapping
	 * should be inserted.
	 */
	index = hash_value(key) % ht->size;
	/*
	 * Insert the new mapping into that collision chain.
	 */
	elem->next = ht->head[index];
	ht->head[index] = elem;
	ht->occupancy++;
	return (0);
}

/*
 * Requires:
 *  Nothing.
 *
 * Effects:
 *  Calls "function" on each mapping in the hash table, passing the mapping's
 *  key, its value, and the latest value of the pointer "cookie" as the
 *  arguments.  The return value is assigned to "cookie", becoming "cookie"'s
 *  latest value.  In other words,
 *	cookie = function(key, value, cookie);
 *  The order in which the mappings are passed to "function" is not defined.
 *  Returns "cookie"'s latest value.
 */
void *
hash_table_iterate(struct hash_table *ht,
    void *(*function)(const char *key, void *value, void *cookie),
    void *cookie)
{
	struct hash_table_mapping *elem;
	unsigned int index;

	/*
	 * Iterate over the collision chains.
	 */
	for (index = 0; index < ht->size; index++) {
		/*
		 * Call "function" on each mapping in the collision chain.
		 */
		for (elem = ht->head[index]; elem != NULL; elem = elem->next)
			cookie = (*function)(elem->key, elem->value, cookie);
	}
	return (cookie);
}

/*
 * Requires:
 *  Nothing.
 *
 * Effects:
 *  Searches the hash table "ht" for the string "key".  If "key" is found,
 *  returns its associated value.  Otherwise, returns NULL.
 */
void *
hash_table_lookup(struct hash_table *ht, const char *key)
{
	struct hash_table_mapping *elem;
	unsigned int index;

	/*
	 * Compute the array index of the collision chain in which "key"
	 * should be found.
	 */
	index = hash_value(key) % ht->size;
	/* 
	 * Iterate over that collision chain.
	 */
	for (elem = ht->head[index]; elem != NULL; elem = elem->next) {
		/*
		 * If "key" matches the element's key, then return the
		 * element's value.
		 */
		if (strcmp(elem->key, key) == 0)
			return (elem->value);
	}
	/*
	 * Otherwise, return NULL to indicate that "key" was not found.
	 */
	return (NULL);
}

/*
 * Requires:
 *  Nothing.
 *
 * Effects:
 *  Searches the hash table "ht" for the string "key".  If "key" is found,
 *  removes its mapping from "ht".  If the function pointer "destructor" is
 *  not NULL, calls "destructor" with the mapping's key, its value, and the
 *  pointer "cookie" as the arguments.  The return value from "destructor" is
 *  assigned to "cookie".  In other words,
 *	cookie = destructor(key, value, cookie);
 *  Returns "cookie"'s value.
 */
void *
hash_table_remove(struct hash_table *ht, const char *key,
    void *(*destructor)(const char *key, void *value, void *cookie),
    void *cookie)
{
	struct hash_table_mapping *elem, *prev;
	unsigned int index;
	
	/*
	 * Compute the array index of the collision chain in which "key"
	 * should be found.
	 */
	index = hash_value(key) % ht->size;
	/*
	 * Iterate over that collision chain.
	 */
	for (elem = ht->head[index]; elem != NULL; elem = elem->next) {
		/*
		 * If "key" matches the element's key, then remove the
		 * mapping from the hash table.
		 */
		if (strcmp(elem->key, key) == 0) {
			/*
			 * First, remove the mapping from its collision
			 * chain.
			 */
			if (elem == ht->head[index])
				ht->head[index] = elem->next;
			else
				prev->next = elem->next;
			ht->occupancy--;
			/*
			 * Then, call "destructor", and free the mapping.
			 */
			if (destructor != NULL)
				cookie = (*destructor)(elem->key, elem->value,
				    cookie);
			free(elem);
			return (cookie);
		}
		prev = elem;
	}
	return (cookie);
}
