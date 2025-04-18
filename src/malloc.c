/*
Zubair Rashaad
1002051693
*/

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)     ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr) - 1)

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes     */
   struct _block *next;  /* Pointer to the next _block of allocated memory      */
   bool   free;          /* Is this _block free?                                */
   char   padding[3];    /* Padding: IENTRTMzMjAgU3jMDEED                       */
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;

   while (curr) 
   {
      if (curr->free && curr->size >= size) 
      {
         num_reuses++;  
         return curr;  
      }

      *last = curr;
      curr = curr->next;
   }

   return curr;

#if defined FIT && FIT == 0
   /* First fit */
   //
   // While we haven't run off the end of the linked list and
   // while the current node we point to isn't free or isn't big enough
   // then continue to iterate over the list.  This loop ends either
   // with curr pointing to NULL, meaning we've run to the end of the list
   // without finding a node or it ends pointing to a free node that has enough
   // space for the request.
   // 
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

// \TODO Put your Best Fit code in this #ifdef block
#if defined BEST && BEST == 0
   //tracking the best fit block
   struct _block *best_fit = NULL;
   //we start with the max so we can find the smallest one
   size_t best_size = SIZE_MAX;

   while (curr) 
   {  
      //we choose the block only if it fits and is also the best fit so far
      if (curr->free && curr->size >= size && curr->size < best_size) 
      {
          best_fit = curr;
          best_size = curr->size;
      }

      //keeping track of the previous block
      *last = curr;
      //this is the best fitting block found
      curr = curr->next;
  }

  curr = best_fit;
#endif

// \TODO Put your Worst Fit code in this #ifdef block
#if defined WORST && WORST == 0
   
   //pointer that will hold the largest fitting block to return
   struct _block *worst_fit = NULL;
   //we start with 0 because we are looking for a block larger than 0
   size_t worst_size = 0;  

   while (curr) 
   {  
      //block has to be free and large enough
   
      if (curr->free && curr->size >= size && curr->size > worst_size) 
      {
         //update when we find a larger block
         worst_fit = curr;
         worst_size = curr->size;
      }

      //this is used to updated the current block
      *last = curr;
      //we move to the next block until curr becomes NULL
      curr = curr->next;
   }

   //holding the largest block found
   curr = worst_fit;
#endif

// \TODO Put your Next Fit code in this #ifdef block
#if defined NEXT && NEXT == 0
   
   //we store the last place we found a block
   struct _block *next_fit = *last;  

   
   while (next_fit)
   {
      //we check if the current block is free and if it is large enough
      if (next_fit->free && next_fit->size >= size) 
      {
         //if we find a block that is free and large enough we return it
        return next_fit;
      }
      //we update last to the current block
      *last = next_fit;
      //then we move to the next block
      next_fit = next_fit->next;
   }

   //if we weren't able to find it, we search from the beginning of heaplist
   curr = heapList;
   while (curr)
   {
      if (curr->free && curr->size >= size) 
      {
         return curr;
      }
      curr = curr->next;
   }
#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }
  

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to previous _block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update _block metadata:
      Set the size of the new block and initialize the new block to "free".
      Set its next pointer to NULL since it's now the tail of the linked list.
   */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;

   num_grows++;  
   
   //updating max heap 
   if (max_heap < size)
   {
      max_heap =  size;
   }

   num_blocks++;  
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free _block.  If a free block isn't found then we need to grow our heap. */

   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);

   /* TODO: If the block found by findFreeBlock is larger than we need then:
            If the leftover space in the new block is greater than the sizeof(_block)+4 then
            split the block.
            If the leftover space in the new block is less than the sizeof(_block)+4 then
            don't split the block.
   */

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }

   //here we split when the block is free and make sure there is enough space leftover
   if (next->free && next->size > size + sizeof(struct _block) + 4)
   {
      //this is the start of the leftover memory, hence our new block
      struct _block *split_block = (struct _block *)((char *)BLOCK_DATA(next) + size);

      //here we initialize our new free block and it gets the leftover size
      split_block->size = next->size - size - sizeof(struct _block);

      //we inherit the next pointer of the original block and also mark the block as free
      split_block->next = next->next;
      split_block->free = true;

      //here we shrink the original block 
      next->size = size;
      //here we point to the newly split block
      next->next = split_block;

      // here we keep track and increment the number of times blocks were split
      num_splits++;
      num_blocks++;

   }



   
   /* Mark _block as in use */
   next->free = false;

   num_mallocs++;
   num_requested += size;

   /* Return data address associated with _block to the user */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   num_frees++;

   /* TODO: Coalesce free _blocks.  If the next block or previous block 
            are free then combine them with this block being freed.
   */
   
   //here we check if there's a free block after the current one
   if(curr->next && curr->next->free)
   {
      //we increase the current blocks size to fit the next block
      curr->size += sizeof(struct _block) + curr->next->size;
      //here we skip over the next block since its now part of the current
      curr->next = curr->next->next;

      //we increment the number of coalesces operations and decrement the number of blocks
      num_coalesces++;
      
   }

   num_blocks--;
   
}

void *calloc( size_t nmemb, size_t size )
{
   //computing the memory we need before passing to malloc
   size_t total_size = nmemb * size;

   void *ptr = malloc(total_size);

   //if the memory allocation was unsuccessful
   if(ptr == NULL)
   {
      return NULL;
   }
   //if memory allocation was successful, allocated memory is set to 0
   else
   {
      memset(ptr,0,total_size);
   }

   return ptr;
}

void *realloc( void *ptr, size_t size )
{

   //when ptr is null it behaves like malloc so we use it to allocate a new block of memory
   if (ptr == NULL) 
   {
      return malloc(size);
   }

  //when the size is 0 it acts like free so we call it to free the memory block pointed at
  if (size == 0) 
  {
      free(ptr);
      return NULL;
  }

  //we allocated a new block of memory 
  void *temp_ptr = malloc(size);

  //
  if (temp_ptr == NULL) 
  {
      return NULL;
  }

  //we get the size of the previous block of memory
  size_t old_size = BLOCK_HEADER(ptr)->size;

  // we calculate the minimum of the old block and new block size
  size_t min_new_size;
  
  //only the amount of memory valid in the new block is copied from the old block
  if (old_size < size) 
  {
      min_new_size = old_size;
  } 
  else 
  {
      min_new_size = size;
  }

 //data from the old block is copied to the new one
  memcpy(temp_ptr, ptr, min_new_size);

  
  free(ptr);

  
  return temp_ptr;
}



/* vim: IENTRTMzMjAgU3ByaW5nIDIwM002= ----------------------------------------*/
/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/