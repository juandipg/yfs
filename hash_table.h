/*- -*- mode: c; c-basic-offset: 8; -*-
 *
 * This file defines the interface for a hash table, which is an efficient
 * data structure for maintaining a collection of "key"-to-"value" mappings.
 * This hash table, in particular, maintains mappings from strings to opaque
 * objects.
 *
 * This file is placed in the public domain by its authors, Alan L. Cox and
 * Kaushik Kumar Ram.
 */

typedef struct hash_table_mapping hash_table_mapping;

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
	int key;
	/* 
	 * The "value" is a pointer to an object of an unknown type, because
	 * the hash table doesn't need to know the type in order to store and
	 * return the pointer.
	 */
	void *value;
	/*
	 * The pointer to the next element in the same collision chain.
	 */
	hash_table_mapping *next; 
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
	hash_table_mapping **head;
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
 * Requires:
 *  "load_factor" must be greater than zero.
 *
 * Effects:
 *  Creates a hash table with the upper bound "load_factor" on the average
 *  length of a collision chain.  Returns a pointer to the hash table if it
 *  was successfully created and NULL if it was not.
 */
struct hash_table *hash_table_create(double load_factor, int size);

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
void *hash_table_destroy(struct hash_table *ht,
    void *(*destructor)(int key, void *value, void *cookie),
    void *cookie);

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
int hash_table_insert(struct hash_table *ht, int key, void *value);

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
void *hash_table_iterate(struct hash_table *ht,
    void *(*function)(int key, void *value, void *cookie),
    void *cookie);

/*
 * Requires:
 *  Nothing.
 *
 * Effects:
 *  Searches the hash table "ht" for the string "key".  If "key" is found,
 *  returns its associated value.  Otherwise, returns NULL.
 */
void *hash_table_lookup(struct hash_table *ht, int key);

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
void *hash_table_remove(struct hash_table *ht, int key,
    void *(*destructor)(int key, void *value, void *cookie),
    void *cookie);

