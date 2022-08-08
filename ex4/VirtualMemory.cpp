#include "VirtualMemory.h"
#include "PhysicalMemory.h"
//#include <cassert>
//#include <cstdio>
#define PTBR_WIDTH (VIRTUAL_ADDRESS_WIDTH - (TABLES_DEPTH  * OFFSET_WIDTH ))
#define LAST_LEVEL (TABLES_DEPTH - 1)
#define SUCSSES 1
#define FAIL 0
enum traversal_state {TOUCHED_BOTTOM , FOUND_EMPTY_FRAME , CLIMB_BACK_UP};
enum eviction_state {NEW_MAX_DIST , CONTINUE};

/*////////////////////////////////////////////////////////////////////////////
 * SUPPLEMENTARY METHODS
 ///////////////////////////////////////////////////////////////////////////*/

void resetWord(uint64_t frame, uint64_t idx)
{
  PMwrite (frame*PAGE_SIZE +idx,0);
}

void resetFrame(uint64_t frame)
{
  for (uint64_t i = 0 ; i < PAGE_SIZE ; i++)
    {
      resetWord(frame,i);
    }
}

uint64_t parseAddress(uint64_t virtualAddress , int depth)
{
  int right_shift = (TABLES_DEPTH - depth ) * OFFSET_WIDTH ; //todo: make sure cxalculation works
  return virtualAddress >> right_shift;
}

uint64_t offset(uint64_t virtualAddress)
{ return virtualAddress & ((1LL << OFFSET_WIDTH)- 1);}

uint64_t currOffset(uint64_t virtualAddress , int depth)
{
  virtualAddress = parseAddress(virtualAddress , depth);
  if (depth == 0){return virtualAddress & ((1LL << PTBR_WIDTH) - 1);}
  return virtualAddress & ((1LL << OFFSET_WIDTH)- 1);
}

uint64_t cyclicDist(uint64_t page_swapped_in , uint64_t p )
{
  uint64_t abs = (page_swapped_in - p >= 0) ? page_swapped_in - p : -1 * (page_swapped_in - p );
  return (NUM_PAGES - abs > abs) ? abs : NUM_PAGES - abs;
}

uint64_t findFrameToEvict( uint64_t * maxDist , uint64_t *availableFrame, uint64_t currFrame,  uint64_t * Frame, uint64_t * Idx , uint64_t page_swapped_in , uint64_t * page_swapped_out, uint64_t buildup ,int depth )
{
  if(depth == TABLES_DEPTH)
    {
      uint64_t dist = cyclicDist(page_swapped_in , buildup);
      if (*maxDist < dist)
      {
        *maxDist = dist;
        *availableFrame = currFrame;
        *page_swapped_out = buildup;
        return NEW_MAX_DIST;
      }
      return CONTINUE;
    }
  word_t nextFrame;
  for (uint64_t i=0; i< PAGE_SIZE; i++) //exploring branches
    {
      PMread(currFrame * PAGE_SIZE + i, &nextFrame); // where to travel next
      if (nextFrame != 0 )
        {
          uint64_t _buildup_ = (buildup << OFFSET_WIDTH) + i;
          uint64_t exploreNextFrame  = findFrameToEvict(maxDist , availableFrame ,nextFrame , Frame , Idx , page_swapped_in , page_swapped_out ,  _buildup_ , depth + 1);
          if (exploreNextFrame == NEW_MAX_DIST)
          {
              * Frame = currFrame;
              * Idx = i ;
          }
        }

    }
  return CONTINUE;
}



uint64_t findEmptyFrame(uint64_t currFrame, uint64_t * Frame,  uint64_t *availableFrame, uint64_t sourceFrame  , uint64_t * Idx, int depth)
{
  if(currFrame > *availableFrame){*availableFrame = currFrame;}
  if(depth >= TABLES_DEPTH){return TOUCHED_BOTTOM;}
  else
  {
    bool isEmpty = true;
    word_t nextFrame;
    for (int i=0; i< PAGE_SIZE; i++) //exploring branches
      {
      PMread(currFrame * PAGE_SIZE + i, &nextFrame); // where to travel next
      if (nextFrame != 0 )// path can continue
      { 
        isEmpty = false;
        uint64_t exploreNextFrame  = findEmptyFrame(nextFrame , Frame , availableFrame , sourceFrame , Idx , depth + 1 );
        if (exploreNextFrame != TOUCHED_BOTTOM)
          {
            if (exploreNextFrame == FOUND_EMPTY_FRAME)
              {
                * Frame = currFrame;
                * Idx = i ;
              }
              return CLIMB_BACK_UP;
          }
        }
      }
    if (isEmpty && currFrame != sourceFrame)
      {
        *availableFrame = currFrame; 
        return FOUND_EMPTY_FRAME;
      }
    return FAIL;
  }
}

uint64_t findAvailableFrame (uint64_t sourceFrame , uint64_t VM_page_address)
{
  uint64_t frame = 0 , availableFrame = 0 , idx = 0 ; int depth = 0;
  uint64_t foundEmptyFrame = findEmptyFrame(0, &frame, &availableFrame , sourceFrame ,& idx , depth );
  if(foundEmptyFrame == FAIL)
    {
      availableFrame++;
      if(availableFrame>= NUM_FRAMES)
        {
          uint64_t  maxDist = 0 , buildup = 0 , page_swapped_out = 0;
          frame = 0 , availableFrame = 0 , idx = 0 , depth = 0;
          findFrameToEvict(& maxDist , &availableFrame , 0 , &frame, &idx  ,VM_page_address ,& page_swapped_out,buildup, depth);
          PMevict( availableFrame,  page_swapped_out);
          resetWord(frame , idx); // going to use this frame somewhere else, so delete its old pointer

        }
    }
  else // found an empty frame
    {
      resetWord(frame , idx); // going to use this frame somewhere else, so delete it's old pointer
    }

  return availableFrame;
  
}


uint64_t physicalAddress (uint64_t virtualAddress) // full adress is sent including offset
  {
    word_t buffer = 0;
    for (int depth = 0; depth < TABLES_DEPTH  ; depth++)
      {
        uint64_t offset = currOffset(virtualAddress, depth);
        uint64_t baseAddress = buffer * PAGE_SIZE;

        PMread (baseAddress + offset, &buffer);
        if (buffer == 0)  //null ptr-> PAGE FAULT: either a mid-level page table or content page not in memory
          {
            uint64_t newFrame = findAvailableFrame (baseAddress / PAGE_SIZE , virtualAddress >> OFFSET_WIDTH);
            PMwrite (baseAddress + offset , (word_t) newFrame); // instead of null ptr - entry points to the newly designated frame
            buffer = (word_t) newFrame;
            if (depth == LAST_LEVEL) //reached final level - need to obtain content page from the disk
              {PMrestore (newFrame,virtualAddress>>OFFSET_WIDTH); } //obtain entire page from memory thus disregard offset}
            else //mid-level page table is missing. OS LECTURE: "When page fault to such a table occur,the OS can create all zero frame for that page table;later to be filled with the content we swap into memory"
              {resetFrame(newFrame );}
          }
      }
    return buffer * PAGE_SIZE;

  }




/*////////////////////////////////////////////////////////////////////////////
EXTERNAL API
/////////////////////////////////////////////////////////////////////////////*/

/*
 * Initialize the virtual memory.
 */
  void VMinitialize (){resetFrame (0);}

/* Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
  int VMread (uint64_t virtualAddress, word_t *value)
  {

    if (virtualAddress >= VIRTUAL_MEMORY_SIZE){ return 0; }
    uint64_t offSet = offset(virtualAddress);
    uint64_t address = physicalAddress (virtualAddress);
    PMread (address + offSet, value);
    return SUCSSES;
  }

/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
  int VMwrite (uint64_t virtualAddress, word_t value)
  {
    if (virtualAddress >= VIRTUAL_MEMORY_SIZE){ return 0; }
    uint64_t offSet = offset(virtualAddress);
    uint64_t address = physicalAddress (virtualAddress) ;
    PMwrite (address + offSet, value);
    return SUCSSES;
  }


