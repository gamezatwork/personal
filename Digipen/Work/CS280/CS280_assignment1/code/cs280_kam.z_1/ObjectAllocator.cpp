/**************************************************************************/
  /*!
  \file   ObjectAllocator.cpp
  \author Wesley Kam
  \par    email:kam.z\@digipen.edu
  \par    DigiPen login: kam.z
  \par    Course: CS280
  \par    Assignment #1
  \date   09/18/2014
  \brief 
  Brief
    \n Contains definitions for an ObjectAllocator class, which allows users
    \n to make use of a MemBlock system to allocate and deallocate data.
    \n Also contains helper functions for the ObjectAllocator class
    \n
    \n FUNCTION DEFINITIONS
    \n  ([p] stands for private)
    \n [Empty namespace] (For utility)
    \n  OffsetPtr                     => Offsets the given pointer by the bytes
    \n  CheckPattern                  => Returns whether bytes fit the pattern
    \n    [Header namespace] (For header stuff)
    \n      CreateMemBlockInfo        => Creates dynamically allocated header
    \n      DestroyMemBlockInfo       => Deletes above-mentioned header
    \n      GetAlloc                  => Returns ref. to allocation num
    \n      GetFlags                  => Returns ref to flag byte
    \n      GetMemBlockInfo           => Returns ref to memblock pointer
    \n      GetUseCount               => Returns ref to use count
    \n      GetUserDefined            => Returns ref to user defined area start
    \n      
    \n ObjectAllocator class
    \n  PUBLIC FUNCTIONS
    \n    Allocate(const char*)       => Passes the client a block of memory
    \n    Free(void*)                 => Frees allocated memory
    \n    DumpMemoryInUse(DUMPCALLBACK)=> Calls the fn for currently used blocks
    \n    ValidatePages(VALIDATECALLBACK)=> Calls the fn for potential corrupted
    \n    FreeEmptyPages()            => Frees all empty pages
    \n    ImplementedExtraCredit()    => Whether extra credit is implemented
    \n    SetDebugState(bool)         => Sets the debugging mode
    \n    GetFreeList()               => returns the free list
    \n    GetPageList()               => returns the page list
    \n    GetConfig()                 => returns the configuration information
    \n    GetStats()                  => returns the stats
    \n  PRIVATE FUNCTIONS
    \n  (Exception checking functions)
    \n [p]CheckInFreeList(GenericObject*)=> Error checks for multiple freeing
    \n [p]CheckPadding(GenericObject*)=> Error checks the padding for corruption
    \n [p]CheckOnBoundary(GenericObject*)=> Error checks for bad boundary access
    \n  (Header functions)
    \n [p]GetDataPtr(GenericObject*)  => Returns pointer to header's data
    \n [p]GetFirstHeader(GenericObject*)=> Returns pointer to first header
    \n [p]GetNextHeader(GenericObject*)=> Returns pointer to next header
    \n  (Page functions)
    \n [p]AddPage()                   => Adds a new page to the pageList
    \n [p]FreePage(GenericObject*)    => Frees the page
    \n [p]GetPageSize(size_t,OAConfig&)=> Returns the size of a page
    \n [p]IsEmptyPage_Header(GenericObject*)=> Returns whether page is empty
    \n [p]IsEmptyPage_NoHeader(GenericObject*)=> Returns whether page is empty
    \n  (Data block functions)
    \n [p]PopFront()                  => Pops from the free list
    \n [p]PushFront(GenericObject*)   => Pushes onto the free list
    \n
  Specific portions that gave you the most trouble:
    Typing out all the functions. It was alot of it.
  */
/**************************************************************************/
#include "ObjectAllocator.h"  /* OAException, OAConfig, OAStats, GenericObject
                                  MemBlockInfo, ObjectAllocator */
#include <cstring>            /* memset, strcpy, strlen */

/**************************************************************************/
  /*!
  \brief 
    Contains helper functions and variables for the ObjectAllocator.
  */
/**************************************************************************/
namespace{
  /**************************************************************************/
  /*!
    \brief 
    Offsets the pointer by the specified amount of bytes and returns the
    offsetted pointer.
    Template contains the Type which the pointer is pointing to.
    
    \param [in] thePtr
    The pointer to offset.
    \param [in] bytes
    The amount of bytes to offset by.
    
    \return 
    The offsetted pointer.
   */
  /**************************************************************************/
  template <typename PtrType>
  PtrType* OffsetPtr(PtrType* thePtr, int bytes){
    // Cast it to size_t to add the bytes to it, and recast it back
    return reinterpret_cast<PtrType*>(reinterpret_cast<size_t>(thePtr) + bytes);
  }
  
  /**************************************************************************/
  /*!
    \brief 
    Checks for the pattern of a bunch of bytes, given the pointer to the
    starting pointer, the amount of bytes to check, and the pattern to 
    check against. Returns whether it was exactly the same or not.
    
    \param [in] thePtr
    The pointer to the starting point.
    \param [in] amt
    The amount of bytes to check
    \param [in] pattern
    The pattern to check each byte for.
    
   */
  /**************************************************************************/
  bool CheckPattern(unsigned char* thePtr, size_t amt, unsigned char pattern){
    // Whether it is corrupted or not
    bool isCorrupted = false;
    // Check through all the bytes
    while (amt > 0){
      // Check whether that byte is the same code as the pattern
      if ((*thePtr) != pattern){
        // PROBLEM, not the padding pattern
        isCorrupted = true;
        break;
      }
      else{
        // Go to next padding byte
        thePtr = OffsetPtr(thePtr, sizeof(unsigned char));
      }
      // Decrement amt
      --amt;
    }
    // Return the status
    return isCorrupted;
  }
  
  /**************************************************************************/
  /*!
    \brief 
    Contains helper functions and variables for the accessing information
    from the ObjectAllocator's headers.
   */
  /**************************************************************************/
  namespace Header{
    
    /* Static variables */
    
    static const int HDR_FLAGS_OFFSET = -static_cast<int>(sizeof(char));
    //!< The offset to the Flags for the header (withiout padding)
    
    static const int HDR_ALLOC_OFFSET = HDR_FLAGS_OFFSET - 
    static_cast<int>(sizeof(unsigned));
    //!< The offset to the alloc num for the header (without padding)
    
    static const int HDR_USEC_OFFSET = HDR_ALLOC_OFFSET - 
    static_cast<int>(sizeof(unsigned short));
    //!< The offset to the use count num for the header (without padding).
    //!< Only for Extended
    
    static const int HDRSIZE_BASIC = static_cast<int>(sizeof(unsigned) 
                                                    + sizeof(unsigned char));
    //!< The size of a basic header, which has an unsigned int
    //!< and an unsigned char.
    
    static const int HDRSIZE_EXTENDED = HDRSIZE_BASIC+
                                       static_cast<int>(sizeof(unsigned short));
    //!< The size of an extended header (excluding user fields), which has
    //!< the same info as a basic header, with an addition unsigned short.
    
    static const int HDRSIZE_EXTERNAL = static_cast<int>(sizeof(MemBlockInfo*));
    //!< The size of an external header, which contains a pointer to a
    //!< dynamically allocated MemBlockInfo
    
    /* Functions */
    /**************************************************************************/
    /*!
      \brief 
      Dynamically allocates a MemBlockInfo with the arguments provided, 
      and sets the provided pointer to point to that dynamically allocated 
      memory.
      
      \param [in] thePtr
      The reference to the MemBlockInfo pointer of the Data block.
      \param [in] inUse
      Whether to set the MemBlockInfo to inUse or not.
      \param [in] aNum
      The allocation number of the MemBlockInfo
      \param [in] label
      The label to pass into the MemBlockInfo.
      
     */
    /**************************************************************************/
    void CreateMemBlockInfo(MemBlockInfo*& thePtr,bool inUse,unsigned aNum,
    const char*& label) throw(OAException){
      // Try to allocate for a new memblockinfo
      try{ thePtr = new MemBlockInfo(); }
      // Problem allocation, throw another exception
      catch (std::bad_alloc& ba)
      { throw(OAException(OAException::E_NO_MEMORY, ba.what())); }
      // Assign in-use
      thePtr->in_use = inUse;
      // Assign the allocation num
      thePtr->alloc_num = aNum;
      // Dynamically allocate the string
      if (label){
        // Try to new
        try{ thePtr->label = new char[strlen(label)+1]; }
        // Problem allocation, throw another exception
        catch (std::bad_alloc& ba)
        { throw(OAException(OAException::E_NO_MEMORY, ba.what())); }
        // Copy it over
        strcpy(thePtr->label, label);
      }
      else thePtr->label = NULL;
    }
    
    /**************************************************************************/
    /*!
      \brief 
      Destroys the dynamically allocated MemBlockInfo theInfo is pointing
      to, and sets it to NULL.
      
      \param [in] theInfo
      The pointer to the dynamically allocated MemBlockInfo to delete.
      
     */
    /**************************************************************************/
    void DestroyMemBlockInfo(MemBlockInfo*& theInfo){
      // Dynamically deallocate the string if there is one
      if (theInfo->label) delete [] theInfo->label;
      // delete the info
      delete theInfo;
      // set it to NULL
      theInfo = NULL;
    }
    
    /**************************************************************************/
    /*!
      \brief 
      Returns the reference to the alloc number of the object passed in,
      with the padding provided.
      (Only works for Basic and Extended headers)
      
      \param [in] thePtr
      The pointer to the Data portion of the MemBlock.
      \param [in] padding
      The padding of the ObjectAllocator
      
      \return 
      The modifiable reference to the allocation number of the header.
     */
    /**************************************************************************/
    unsigned& GetAlloc(GenericObject* thePtr, int padding){
      // Return the pointer to the unsigned
      return *OffsetPtr(reinterpret_cast<unsigned*>(thePtr),
      HDR_ALLOC_OFFSET-padding);
    }
    
    /**************************************************************************/
    /*!
      \brief 
      Returns the reference to the header flags of the object passed in,
      with the padding provided.
      (Only works for Basic and Extended headers)
      
      \param [in] thePtr
      The pointer to the Data portion of the MemBlock.
      \param [in] padding
      The padding of the ObjectAllocator
      
      \return 
      The modifiable reference to the flags of the header.
     */
    /**************************************************************************/
    char& GetFlags(GenericObject* thePtr, int padding){
      // Return the pointer to the char
      return *OffsetPtr(reinterpret_cast<char*>(thePtr),
                        HDR_FLAGS_OFFSET - padding);
    }
    
    /**************************************************************************/
    /*!
      \brief 
      Returns the reference to the mem block info pointer of the object passed
      in, with the padding provided.
      (Only works for External headers)
      
      \param [in] thePtr
      The pointer to the Data portion of the MemBlock.
      \param [in] padding
      The padding of the ObjectAllocator
      
      \return 
      The modifiable reference to MemBlockInfo pointer.
     */
    /**************************************************************************/
    MemBlockInfo*& GetMemBlockInfo(GenericObject* thePtr, int padding){
      // Returns the external pointer area
      return *(OffsetPtr(reinterpret_cast<MemBlockInfo**>(thePtr), 
      -HDRSIZE_EXTERNAL-padding));
    }
    
    /**************************************************************************/
    /*!
      \brief 
      Returns the reference to the use count of the object passed in,
      with the padding provided.
      (Only works for Extended headers)
      
      \param [in] thePtr
      The pointer to the Data portion of the MemBlock.
      \param [in] padding
      The padding of the ObjectAllocator
      
      \return 
      The modifiable reference to the use count of the header.
     */
    /**************************************************************************/
    unsigned short& GetUseCount(GenericObject* thePtr, int padding){
      // Return the pointer to the unsigned short
      return *OffsetPtr(reinterpret_cast<unsigned short*>(thePtr),
                        HDR_USEC_OFFSET-padding);
    }
    
    /**************************************************************************/
    /*!
      \brief 
      Returns the reference to the first bytes of the user-defined area of the
      object passed in, with the padding provided, and the size of the user
      defined area.
      (Only works for Extended headers)
      
      \param [in] thePtr
      The pointer to the Data portion of the MemBlock.
      \param [in] size
      The size of the user defined area.
      \param [in] padding
      The padding of the ObjectAllocator
      
      \return 
      The modifiable reference to the first byte of the user-defined area.
     */
    /**************************************************************************/
    char& GetUserDefined(GenericObject* thePtr, size_t size, int padding){
      // Return the pointer to the first char
      return *OffsetPtr(reinterpret_cast<char*>(thePtr), 
                        HDR_USEC_OFFSET - static_cast<int>(size) - padding);
    }
  }
}

/* Public functions */

/**************************************************************************/
/*!
  \brief 
  Creates the ObjectManager per the specified values.
  Throws an exception if the construction fails. (Memory allocation problem)
  
  \param [in] ObjectSize
  The size of each object.
  \param [in] config
  The configuration of the ObjectAllocator
 */
/**************************************************************************/
ObjectAllocator::ObjectAllocator(size_t ObjectSize, const OAConfig& config) 
                                 throw(OAException) : 
                                 _config(config), _stats(),
                                 _freeList(NULL), _pageList(NULL) 
{
  // Set up the OA Stats
  _stats = OAStats();
  _stats.ObjectSize_ = ObjectSize;
  // Calculates the amount of memory per page
  _stats.PageSize_ = GetPageSize(ObjectSize, _config);
  // Allocates a page of memory first
  _pageList = AddPage();
  // And set the next of it to be NULL
  _pageList->Next = NULL;
}

/**************************************************************************/
/*!
  \brief 
  Destroys the ObjectManager (never throws)
 */
/**************************************************************************/
ObjectAllocator::~ObjectAllocator() throw()
{
  // Clear all the pages
  while (_pageList){
    // Grab the next page
    GenericObject* nextPage = _pageList->Next;
    // Free the current page
    FreePage(_pageList);
    // Set page list to the next page
    _pageList = nextPage;
  }
}

/**************************************************************************/
/*!
  \brief 
  Take an object from the free list and give it to the client (simulates new)
  Throws an exception if the object can't be allocated. 
  (Memory allocation problem)
  
  \param [in] label
  The label to set for the External header (if it is External Header)
  
  \return
  The pointer to the allocated memory.
 */
/**************************************************************************/
void * ObjectAllocator::Allocate(const char *label) throw(OAException){
  // if the CPP memory manager is enabled, just use it.
  if (_config.UseCPPMemManager_){
    // The object which will be allocated
    char* theObject = NULL;
    // Attempt to allocate space for it
    try{ theObject = new char[_stats.ObjectSize_];}
    // Failed to get space!
    catch (std::bad_alloc& ba)
    { throw(OAException(OAException::E_NO_MEMORY, ba.what())); }
    // Succeede! Increment and return the object
    ++_stats.ObjectsInUse_ ; ++_stats.MostObjects_; ++_stats.Allocations_;
    return theObject;
  } // If there isn't any objects on the free list, add a page first
  if (!_freeList){
    // Add a page
    GenericObject* newPage = AddPage();
    // Add it to the page list
    newPage->Next = _pageList; _pageList = newPage;
  }
  // Grab the first one and increment allocations
  GenericObject* theObj = PopFront(); _stats.Allocations_++;
  // If it has no header, don't bother creating the header
  if (_config.HBlockInfo_.type_ != OAConfig::hbNone){
    // if it is NOT external
    if (_config.HBlockInfo_.type_ != OAConfig::hbExternal){
      // Grab the flags and set it to true
      Header::GetFlags(theObj, _config.PadBytes_) = true;
      // Grab the alloc num and set it to the new allocations
      Header::GetAlloc(theObj, _config.PadBytes_) = _stats.Allocations_;
      // if it is extended, set up the use count and zero out the user area
      if (_config.HBlockInfo_.type_ == OAConfig::hbExtended){
        // Increment use count
        ++(Header::GetUseCount(theObj, _config.PadBytes_));
        // Set up user defined as all 0s
        char& firstByte = Header::GetUserDefined(theObj,
                          _config.HBlockInfo_.additional_,_config.PadBytes_);
       std::memset(&firstByte, 0, _config.HBlockInfo_.additional_);
      }
    } // if it IS external, grab the external pointer area and set it up
    else{
      // Grab the external pointer area and set it up
      MemBlockInfo*& memBlock = Header::GetMemBlockInfo(theObj
                                                      ,_config.PadBytes_);
      Header::CreateMemBlockInfo(memBlock,true,_stats.Allocations_,label);
    }
  }
  // If it is debug, set up all the values as the pattern first
  if (_config.DebugOn_)std::memset(theObj,ALLOCATED_PATTERN,_stats.ObjectSize_);
  // Increment objects in use and return the object
  ++_stats.ObjectsInUse_ ; ++_stats.MostObjects_;
  return theObj;
}

/**************************************************************************/
/*!
  \brief 
  Returns an object to the free list for the client (simulates delete)
  Throws an exception if the the object can't be freed. (Invalid object)
  
  \param [in] Object
  The object to free
 */
/**************************************************************************/
void ObjectAllocator::Free(void *Object) throw(OAException){
  // If it is a nullptr, do nothing.
  if (!Object) return;
  // if the CPP memory manager is enabled, just use it.
  if (_config.UseCPPMemManager_) delete [] reinterpret_cast<char*>(Object);
  else{
    // Cast it to a GenericObject*
    GenericObject* theObj = reinterpret_cast<GenericObject*>(Object);
    // If debug is on, do the checks first
    if (_config.DebugOn_){
      // If there is padding, check the padding
      if (_config.PadBytes_) CheckPadding(theObj);
      // Check whether it is on the boundaries
      CheckOnBoundary(theObj);
      // Check whether its in the free list
      if (_freeList) CheckInFreeList(theObj);
    }
    // Check the headers next if there are any, and set it up
    switch (_config.HBlockInfo_.type_){
      // if it is external, destroy the memblockinfo
      case OAConfig::hbExternal:{
        // Grab the external pointer area
        MemBlockInfo*& theHeader = Header::GetMemBlockInfo(theObj,
                                                          _config.PadBytes_);
        // Check whether it is in use
        if ((theHeader && theHeader->in_use) || !_config.DebugOn_){
          // Destroy it
          Header::DestroyMemBlockInfo(theHeader);
          // And set it to null
          theHeader = NULL;
        }
        // If it is unused, BAD BOY
        else throw(OAException(OAException::E_MULTIPLE_FREE,"Multiple frees"));
      } break;
      // If it isn't, use regular checks
      case OAConfig::hbBasic:
      case OAConfig::hbExtended:
      // Set the flag to unused if it is used
      if (Header::GetFlags(theObj, _config.PadBytes_) || !_config.DebugOn_){
        Header::GetFlags(theObj, _config.PadBytes_) = false;
        Header::GetAlloc(theObj, _config.PadBytes_) = 0;
      }
      // If it is unused, BAD BOY
      else throw(OAException(OAException::E_MULTIPLE_FREE, "Multiple frees"));
      break;
      // If it is noHeader, no header check/setting
      case OAConfig::hbNone:
      break;
    }
    // If it is debug mode, memset hte pattern
    if (_config.DebugOn_)std::memset(theObj, FREED_PATTERN, _stats.ObjectSize_);
    // Pass it back into the free list
    PushFront(theObj);
  }
  // Increment deallocs and decrement objects in use
  ++_stats.Deallocations_; --_stats.ObjectsInUse_;
}

/**************************************************************************/
/*!
  \brief 
  Calls the callback fn for each block still in use
  
  \param [in] fn
  The callback function to use on each block.
  void (*)(const void *, size_t)
  
  \return
  The number of memory blocks still in use
 */
/**************************************************************************/
unsigned ObjectAllocator::DumpMemoryInUse(DUMPCALLBACK fn) const {
  // If fn is a nullptr, just return 0
  if (fn == NULL) return 0;
  // Number of blocks found
  unsigned numOfBlocks = 0;
  // Traverse the pages
  GenericObject* pagePtr = _pageList;
  while (pagePtr){
    // Traverse through the headers
    GenericObject* headerPtr = GetFirstHeader(pagePtr);
    for (size_t i = 0; i < _config.ObjectsPerPage_; ++i){
      // Whether it is in use
      bool inUse = false;
      switch (_config.HBlockInfo_.type_){
        // If there is no header, traverse through it manually
        case OAConfig::hbNone:
        { // Grab the data
          GenericObject* theData = GetDataPtr(headerPtr);
          // Check through the free list
          GenericObject* freeListPtr = _freeList;
          while (freeListPtr){
            // Found it in the free list, DON'T call it
            if (freeListPtr == theData) break;
            freeListPtr = freeListPtr->Next;
          }
          // If it is Null, its in use
          if (!freeListPtr)inUse = true;
        } break;
        // If it is external, grab ptr to mem block
        case OAConfig::hbExternal:
        { // Grab the header
          MemBlockInfo* theHeaderPtr=reinterpret_cast<MemBlockInfo*>(headerPtr);
          // If it is used, use function
          if (theHeaderPtr->in_use) inUse = true;
        } break;
        // If it is basic or extended, just check the useflags
        case OAConfig::hbBasic:
        case OAConfig::hbExtended:
        if (Header::GetFlags(GetDataPtr(headerPtr),_config.PadBytes_))
          inUse = true;
        break;
      }
      // if it is in use, call the function
      if (inUse){
        // Grab the data
        GenericObject* theData = GetDataPtr(headerPtr);
        // Call the function
        fn(theData, _stats.ObjectSize_);
        // Increment the number of blocks found
        ++numOfBlocks;
      }
      // Move to the next header if it is not the last
      if (i != _config.ObjectsPerPage_ -1) headerPtr = GetNextHeader(headerPtr);
    } // Go to the next page
    pagePtr = pagePtr->Next;
  }
  return numOfBlocks;
}

/**************************************************************************/
/*!
  \brief 
  Calls the callback fn for each block that is potentially corrupted
  
  \param [in] fn
  The callback function to use on each potentially corrupted block.
  void (*)(const void *, size_t)
  
  \return
  The number of pages validated
 */
/**************************************************************************/
unsigned ObjectAllocator::ValidatePages(VALIDATECALLBACK fn) const{
  // If fn is null, just return 0
  if (fn == NULL) return 0;
  // The number of potentially corrupted pages
  unsigned numOfPages = 0;
  // Traverse the pages
  GenericObject* pagePtr = _pageList;
  while (pagePtr){
    // Traverse through the headers
    GenericObject* headerPtr = GetFirstHeader(pagePtr);
    for (size_t i = 0; i < _config.ObjectsPerPage_; ++i){
      // Whether it is corrupted or not
      bool isCorrupted = false;
      // Store the pointer to the padding start
      unsigned char* checkPtr = OffsetPtr(
                                reinterpret_cast<unsigned char*>(headerPtr), 
                                static_cast<int>(_config.HBlockInfo_.size_));
      // What will be used to check the padding
      unsigned char* padCheckPtr = checkPtr;
      // If padding exists, check the front and the back padding
      if (_config.PadBytes_ && _config.DebugOn_){
        // Check the front padding
        isCorrupted = CheckPattern(padCheckPtr, _config.PadBytes_, PAD_PATTERN);
        // If it is NOT corrupted, carry on checking
        if (!isCorrupted){
          // Move past the data
          checkPtr = OffsetPtr(checkPtr,
                        static_cast<int>(_stats.ObjectSize_+_config.PadBytes_));
          // Check corrupted of the other padding
          isCorrupted = CheckPattern(checkPtr, _config.PadBytes_, PAD_PATTERN);
        }
      }
      // Done checking the block, if it is corrupted, call validate
      if (isCorrupted){
        GenericObject* dataPtr = GetDataPtr(headerPtr);
        fn(dataPtr,_stats.ObjectSize_);
        ++numOfPages;
      }
      // Move to the next one if it isn't the last one
      if (i != _config.ObjectsPerPage_-1) headerPtr = GetNextHeader(headerPtr);
    }
    // Go to the next pagePtr
    pagePtr = pagePtr->Next;
  }
  // Return the num of pages
  return numOfPages;
}

/**************************************************************************/
/*!
  \brief 
  Frees all empty pages.
  
  \return
  The amount of pages freed.
 */
/**************************************************************************/
unsigned ObjectAllocator::FreeEmptyPages(void){
  // If the amount in the free list is less than the page amount allowed, NONE
  if (_stats.FreeObjects_ < _config.ObjectsPerPage_) return 0;
  // Stores the number of pages
  unsigned numOfPages = 0;
  // Traverse the pages
  GenericObject* pagePtr = _pageList;
  GenericObject* prevPagePtr = NULL;
  while (pagePtr){
    // Whether there are any blocks used
    bool pageIsUsed;
    // If it is not NONE, call the non-NONE version
    if (_config.HBlockInfo_.type_ != OAConfig::hbNone)
      // Check whether it is an empty page
      pageIsUsed = IsEmptyPage_Header(pagePtr);
    // if it is NONE, call the NONE version
    else pageIsUsed = IsEmptyPage_NoHeader(pagePtr);
    // If the page is NOT used, free the page
    if (!pageIsUsed){
      // Grab the next page
      GenericObject* nextPagePtr = pagePtr->Next;
      // Clear the old page
      FreePage(pagePtr);
      // Set pagePtr to the next
      pagePtr = nextPagePtr;
      // If it is not the first page, set the prev page to point to that
      if (prevPagePtr) prevPagePtr->Next = pagePtr;
      // If it is, set the page list to begin from that
      else _pageList = pagePtr;
      ++numOfPages;
    }
    // if it is used, just go to the next page
    else{
      prevPagePtr = pagePtr;
      pagePtr = pagePtr->Next;
    }
  }
  return numOfPages;
}

/**************************************************************************/
/*!
  \brief 
  Returns true if FreeEmptyPages and alignments are implemented
  
  \return
  Whether extra credit has been implemented
 */
/**************************************************************************/
bool ObjectAllocator::ImplementedExtraCredit(void)
{
  // Yep, it is.
  return true;
}

/**************************************************************************/
/*!
  \brief 
  Switches on/off the debugging methods.
  
  \param [in] State
  true=enable, false=disable
 */
/**************************************************************************/
void ObjectAllocator::SetDebugState(bool State)
{
  _config.DebugOn_ = State;
}

/**************************************************************************/
/*!
  \brief 
  Returns a pointer to the internal free list
  
  \return
  Pointer to the free list
 */
/**************************************************************************/
const void * ObjectAllocator::GetFreeList(void) const
{
  return _freeList;
}  

/**************************************************************************/
/*!
  \brief 
  Returns a pointer to the internal page list
  
  \return
  Pointer to the page list
 */
/**************************************************************************/
const void * ObjectAllocator::GetPageList(void) const
{
  return _pageList;
}

/**************************************************************************/
/*!
  \brief 
  Returns the configuration parameters
  
  \return
  The configuration parameters
 */
/**************************************************************************/
OAConfig ObjectAllocator::GetConfig(void) const       
{
  return _config;
}

/**************************************************************************/
/*!
  \brief 
  Returns the stats of the ObjectAllocator
  
  \return
  The stats
 */
/**************************************************************************/
OAStats ObjectAllocator::GetStats(void) const         
{
  return _stats;
}

/* Private functions */
// Exception checking functions
/**************************************************************************/
/*!
  \brief 
  Checks whether the object is in the free list, throws exceptions 
  if it is. (Not supposed to be in free list)
  Does so manually.
  
  \param [in] theObj
  The object to check whether it is within the free list
 */
/**************************************************************************/
void ObjectAllocator::CheckInFreeList(GenericObject* theObj) const 
throw(OAException){
  // Check free list, Loop through free list lookin for whether it is in free
  GenericObject* listPtr = _freeList;
  while(listPtr){
    // Check whether it is in the free list, throw if it is
    if (listPtr == reinterpret_cast<GenericObject*>(theObj))
    throw(OAException(OAException::E_MULTIPLE_FREE, "Free: Multiple free!!"));
    listPtr = listPtr->Next;
  }
}

/**************************************************************************/
/*!
  \brief 
  Checks the padding of the object, throws exceptions if it is corrupted
  
  \param [in] theObj
  The object to check the padding of
 */
/**************************************************************************/
void ObjectAllocator::CheckPadding(GenericObject* theObj) const 
throw(OAException) {
  // If padding exists, check the front and the back padding
  if (_config.PadBytes_){
    // Grab the padding
    unsigned char* padChckPtr=OffsetPtr(reinterpret_cast<unsigned char*>(theObj)
    , -static_cast<int>(_config.PadBytes_));
    // Check the front padding
    bool isCorrupted = CheckPattern(padChckPtr, _config.PadBytes_, PAD_PATTERN);
    // If it is NOT corrupted, carry on checking
    if (!isCorrupted){
      // Move past the data
      padChckPtr = OffsetPtr(padChckPtr, 
                   static_cast<int>(_stats.ObjectSize_+_config.PadBytes_));
      // Check the padding again
      isCorrupted = CheckPattern(padChckPtr, _config.PadBytes_, PAD_PATTERN);
    }
    // if it is corrupted now, THROW
    if (isCorrupted) 
    throw(OAException(OAException::E_CORRUPTED_BLOCK,"Data corrupted!"));
  }
}

/**************************************************************************/
/*!
  \brief 
  Checks whether the object is on the boundary, throws exceptions 
  if it is not (the pointer is not pointing to the data block's start)
  
  \param [in] theObj
  The object to check the padding of
 */
/**************************************************************************/
void ObjectAllocator::CheckOnBoundary(GenericObject* theObj) const
throw(OAException){
  // The address of the object
  size_t objAddress = reinterpret_cast<size_t>(theObj);
  // Find the list you are on
  GenericObject* listPtr = _pageList;
  // Address of the list
  size_t listAddr = reinterpret_cast<size_t>(listPtr);
  while(listPtr){
    // If the address is within the list, its "in" the list area
    if ((objAddress>listAddr) && (objAddress<(listAddr+_stats.PageSize_)))
      break;
    // If not, carry on
    listPtr = listPtr->Next;
    if (listPtr) listAddr = reinterpret_cast<size_t>(listPtr);
  }
  // if it couldn't find any of them, NOT ON BOUNDARY, THROW
  if (!listPtr) throw(OAException(OAException::E_BAD_BOUNDARY,
                      "Bad boundary, not in list!"));
  // Address offset of the object relative to the list
  int ptrOffset = static_cast<int>(objAddress) - static_cast<int>(listAddr);
  // If it has alignment, check against the alignment
  if (_config.Alignment_){
    // NOT ALIGNED
    if (ptrOffset%_config.Alignment_) 
    throw(OAException(OAException::E_BAD_BOUNDARY,"Bad boundary, unaligned!"));
  }
  // Else, check against the object sizes manually
  else{
    // Remove the list ptr,the first padding and header and check
    ptrOffset-=static_cast<int>(sizeof(GenericObject*)+
    _config.PadBytes_+_config.HBlockInfo_.size_);
    // First member?
    if (ptrOffset == 0) return;
    // if it is NOT negative and it is a multiple of the ObjectSize, FOUND!
    if ((ptrOffset%(_stats.ObjectSize_+_config.PadBytes_*2+
    _config.HBlockInfo_.size_))==0) return;
    // If not, THROW
    else 
    throw(OAException(OAException::E_BAD_BOUNDARY,"Bad boundary, unaligned!"));
  }
}

// Header functions

/**************************************************************************/
/*!
  \brief 
  Just returns the data portion which belongs to the header
  DOES NOT CHECK FOR NULLPTRS.
  
  \param [in] theHeader
  The header to grab the data portion of
  
  \return
  The pointer to the data portion
 */
/**************************************************************************/
GenericObject* ObjectAllocator::GetDataPtr(GenericObject* theHeader) const{
  return OffsetPtr(theHeader,static_cast<int>(_config.HBlockInfo_.size_+
                                              _config.PadBytes_));
}

/**************************************************************************/
/*!
  \brief 
  Just returns the pointer to the first header in the list.
  DOES NOT CHECK FOR NULLPTRS
  
  \param [in] theList
  The list to get the first header of
  
  \return
  The pointer to the first header
 */
/**************************************************************************/
GenericObject* ObjectAllocator::GetFirstHeader(GenericObject* theList) const{
  return OffsetPtr(theList, 
                   static_cast<int>(sizeof(theList)+_config.LeftAlignSize_));
}

/**************************************************************************/
/*!
  \brief 
  Just returns the pointer to the next header in the list.
  DOES NOT CHECK FOR NULLPTRS.
  
  \param [in] prevHdr
  The header to grab the next of
  
  \return
  The pointer to the next header
 */
/**************************************************************************/
GenericObject* ObjectAllocator::GetNextHeader(GenericObject* prevHdr) const{
  return OffsetPtr(prevHdr,static_cast<int>(_config.HBlockInfo_.size_+
  (_config.PadBytes_*2+_stats.ObjectSize_+_config.InterAlignSize_)));
}

// Page functions

/**************************************************************************/
/*!
  \brief 
  Allocates a new page and sets it up, along with returning a pointer
  to the new page (modifies data)
  
  \return
  The new page
 */
/**************************************************************************/
GenericObject* ObjectAllocator::AddPage() throw(OAException){
  // Check how many pages are allocated, throw if there are too many
  if (_stats.PagesInUse_ == _config.MaxPages_) 
    throw(OAException(OAException::E_NO_PAGES, "Allocate: Not enough pages"));
  // Store the size of a generic pointer (used very often)
  unsigned ptrSize = sizeof(GenericObject*);
  // Allocates a page of memory first
  unsigned char* theCharPtr = NULL;
  try{ theCharPtr = new unsigned char[_stats.PageSize_]; }
  // Problem allocation, throw another exception
  catch (std::bad_alloc& ba){
    throw(OAException(OAException::E_NO_MEMORY, ba.what()));
  }
  GenericObject* pagePtr = reinterpret_cast<GenericObject*>(theCharPtr);
  //(NextPtr)Skip the next ptr for the page(at the header now)
  theCharPtr = OffsetPtr(theCharPtr, ptrSize);
  //(LeftAlign)If debug is on, set up pattern in the left align
  if (_config.DebugOn_ && _config.LeftAlignSize_)
    std::memset(theCharPtr, ALIGN_PATTERN, _config.LeftAlignSize_);
  //(LeftAlign)Finished with LeftAlign, move on
  theCharPtr = OffsetPtr(theCharPtr,_config.LeftAlignSize_);
  //(DB:Start)Iterate for all the elements
  for (size_t i = 0; i < _config.ObjectsPerPage_; ++i){
    //(DB:Header) Only act if there is any header, memset all to 0
    if (_config.HBlockInfo_.size_)
      std::memset(theCharPtr, 0, _config.HBlockInfo_.size_);
    //(DB:Header)Finished with header block, move on
    theCharPtr = OffsetPtr(theCharPtr,
                 static_cast<int>(_config.HBlockInfo_.size_));
    //(DB:PaddingStart)If debug is on, set up the pad pattern in the padding
    if (_config.DebugOn_ && _config.PadBytes_)
      std::memset(theCharPtr, PAD_PATTERN, _config.PadBytes_);
    //(DB:PaddingStart)Finished with padding block, move on
    theCharPtr = OffsetPtr(theCharPtr,_config.PadBytes_);
    //(DB:Data)Create the next ptr for the data block
    GenericObject* blockPtr = reinterpret_cast<GenericObject*>(theCharPtr);
    //(DB:Data)Make this the head of the free list
    PushFront(blockPtr);
    //(DB:Data)Finished with data ptr, move on
    theCharPtr = OffsetPtr(theCharPtr,ptrSize);
    //(DB:DataIn)If debug is on, set up the unallocated pattern in the data
    if (_config.DebugOn_)
      std::memset(theCharPtr, UNALLOCATED_PATTERN, _stats.ObjectSize_-ptrSize);
    //(DB:DataIn)Finished with the data block, move on
    theCharPtr= OffsetPtr(theCharPtr, static_cast<int>(_stats.ObjectSize_)
                                    - static_cast<int>(ptrSize));
    //(DB:PaddingEnd)If debug is on, set up the pad pattern in the padding
    if (_config.DebugOn_ && _config.PadBytes_)
      std::memset(theCharPtr, PAD_PATTERN, _config.PadBytes_);
    //(DB:PostPadding)If it is not the last one
    if (i != _config.ObjectsPerPage_-1){
      //(DB:PostPadding)Finished with padding block, move on
      theCharPtr = OffsetPtr(theCharPtr,_config.PadBytes_);
      //(DB:InterAlign)Interalignment, memset all to align pattern
      if (_config.InterAlignSize_) 
        std::memset(theCharPtr, ALIGN_PATTERN, _config.InterAlignSize_);
      //(DB:InterAlign)Finished with the interalignsize, move on
      theCharPtr = OffsetPtr(theCharPtr,_config.InterAlignSize_);
    }
  }
  // Increment the amoutn of pages allocated
  ++_stats.PagesInUse_;
  // Return the pagePtr
  return pagePtr;
}

/**************************************************************************/
/*!
  \brief 
  Frees the entire page and removes all items in the free list which belong
  to that page.
  
  \param [in] thePagePtr
  The page to free
 */
/**************************************************************************/
void ObjectAllocator::FreePage(GenericObject* thePagePtr){
  // If it is external header, delete all the headers first
  if (_config.HBlockInfo_.type_ == OAConfig::hbExternal){
    // Jump through all the headers
    MemBlockInfo** headerPtr = OffsetPtr(reinterpret_cast<MemBlockInfo**>
                               (thePagePtr),static_cast<int>
                               (sizeof(GenericObject*)+_config.LeftAlignSize_));
    for (size_t i = 0; i < _config.ObjectsPerPage_; ++i){
      // Grab the next header if it isn't the last
      if (i != (_config.ObjectsPerPage_ - 1)){
        MemBlockInfo** nextHeader = OffsetPtr(headerPtr,
        static_cast<int>(_config.HBlockInfo_.size_+_config.PadBytes_*2
        +_stats.ObjectSize_+_config.InterAlignSize_));
        // Delete the old header
        delete *headerPtr;
        // Set it to the nextHeader
        headerPtr = nextHeader;
      }
      // Just delete if not
      else delete *headerPtr;
    }
  }
  // Remove all the items in the free list which are part of it
  int amtOfItemsLeft = _config.ObjectsPerPage_;
  // Run through the free list
  GenericObject* freeListPtr = _freeList;
  GenericObject* prevListPtr = NULL;
  while(amtOfItemsLeft && freeListPtr){
    int offset = static_cast<int>(reinterpret_cast<size_t>(freeListPtr)) 
              - static_cast<int>(reinterpret_cast<size_t>(thePagePtr));
    // check whether it is within the boundaries of the page ptr
    if (offset >= 0 && offset < static_cast<int>(_stats.PageSize_)){
      // FOUND ONE! Remove it from the list
      if (prevListPtr != NULL) prevListPtr->Next = freeListPtr->Next;
      else _freeList = freeListPtr->Next;
      freeListPtr = freeListPtr->Next;
      --_stats.FreeObjects_;
    }
    else{
      // Go to the next freeListPtr
      prevListPtr = freeListPtr;
      freeListPtr = freeListPtr->Next;
    }
  }
  // Then delete the entire page
  delete [] reinterpret_cast<char*>(thePagePtr);
  // Decrement the amoutn of pages allocated
  --_stats.PagesInUse_;
}

/**************************************************************************/
/*!
  \brief 
  Returns the size of a page based on the config and modifies the config
  file accordingly.
  
  \param [in] ObjectSize
  The size of each object
  
  \param [in] config
  The configurations to modify
  
  \return
  The size of the page
 */
/**************************************************************************/
size_t ObjectAllocator::GetPageSize(size_t ObjectSize, OAConfig& config) const {
  // Get the size of the front block first
  size_t frontSize = static_cast<size_t>(sizeof(GenericObject*))+
                       config.PadBytes_+config.HBlockInfo_.size_;
  // If the front size is not a multiple of the alignment, setup left align
  if (config.Alignment_ && frontSize%config.Alignment_){
    config.LeftAlignSize_ = static_cast<size_t>(
                            static_cast<int>(config.Alignment_)-
                            static_cast<int>(frontSize%config.Alignment_));
    // And increment frontSize
    frontSize += config.LeftAlignSize_;
  }
  // Next, get the size of each block (from the start of the data to right
  // before the start of the next data)
  size_t blockSize = ObjectSize + config.PadBytes_*2 + config.HBlockInfo_.size_;
  
  // If each block's size is not a multiple of the alignment, setup interalign
  if (config.Alignment_ && (blockSize+frontSize)%config.Alignment_){
    config.InterAlignSize_ = static_cast<int>(config.Alignment_) - 
                        static_cast<int>(blockSize+frontSize)%config.Alignment_;
    // And increment frontSize
    blockSize += config.InterAlignSize_;
  }
  // Done, return the total size
  return frontSize+blockSize*(config.ObjectsPerPage_-1)
                  +ObjectSize+config.PadBytes_;
}

/**************************************************************************/
/*!
  \brief 
  Returns whether its an empty page (If they have headers)
  
  \param [in] pagePtr
  The callback function to use on each potentially corrupted block.
  void (*)(const void *, size_t)
  
  \return
  Whether it is empty or not.
 */
/**************************************************************************/
bool ObjectAllocator::IsEmptyPage_Header(GenericObject* pagePtr) const{
  bool pageIsUsed = false;
  // Traverse through the headers
  GenericObject* headerPtr = GetFirstHeader(pagePtr);
  for (size_t i = 0; i < _config.ObjectsPerPage_; ++i){
    // If it is basic or extended, just check the useflags
    if (_config.HBlockInfo_.type_ == OAConfig::hbBasic ||
    _config.HBlockInfo_.type_ == OAConfig::hbExtended)
    {
      if (Header::GetFlags(GetDataPtr(headerPtr),_config.PadBytes_))
        pageIsUsed = true;
    }
    // if it is external, grab the ptr to the mem block
    else if (_config.HBlockInfo_.type_ == OAConfig::hbExternal){
      // Grab the header
      MemBlockInfo* theHeaderPtr = reinterpret_cast<MemBlockInfo*>(headerPtr);
      // If it is used, use function
      if (theHeaderPtr->in_use) pageIsUsed = true;
    }
    // If the page is used, break the loop
    if (pageIsUsed) break;
    // Move to the next header if it is not the last
    if (i != _config.ObjectsPerPage_ -1) headerPtr = GetNextHeader(headerPtr);
  }
  return pageIsUsed;
}

/**************************************************************************/
/*!
  \brief 
  Returns whether its an empty page (If they have NO headers)
  
  \param [in] pagePtr
  The callback function to use on each potentially corrupted block.
  void (*)(const void *, size_t)
  
  \return
  Whether it is empty or not.
 */
/**************************************************************************/
bool ObjectAllocator::IsEmptyPage_NoHeader(GenericObject* pagePtr) const{
  // USE POINTER ARITHMETIC
  int amtOfItemsLeft = static_cast<int>(_config.ObjectsPerPage_);
  // Run through the free list
  GenericObject* freeListPtr = _freeList;
  while(amtOfItemsLeft != 0 && freeListPtr != NULL){
    // The offset of the free list obj from the list
    int offset = static_cast<int>(reinterpret_cast<size_t>(freeListPtr))
                -static_cast<int>(reinterpret_cast<size_t>(pagePtr));
    // check whether it is within the boundaries of the page ptr
    if (offset >= 0 && static_cast<size_t>(offset) < _stats.PageSize_)
    // FOUND ONE! Decrease items left by one
    --amtOfItemsLeft;
    // Go to the next freeListPtr
    freeListPtr = freeListPtr->Next;
  }
  // If have items left, STILL HAVE ITEMS NOT IN FREE LIST
  if (amtOfItemsLeft != 0) return true;
  return false;
}

// Data block functions
/**************************************************************************/
/*!
  \brief 
  Grabs the first mem block from the front of the free list and returns
  
  \return
  The first mem block
 */
/**************************************************************************/
GenericObject* ObjectAllocator::PopFront() throw(OAException){
  // If it is empty, throw exception
  if (!_freeList) 
    throw(OAException(OAException::E_NO_MEMORY, "Allocate: Not enough memory"));
  // Grabs the first
  GenericObject* poppedObj = _freeList;
  // Changes the free list ot begin with the next
  _freeList = poppedObj->Next;
  // Decrement objects on free list
  --_stats.FreeObjects_;
  // Returns the popped obj
  return poppedObj;
}

/**************************************************************************/
/*!
  \brief 
  Adds the memory block to the front of the free list
  
  \param [in] memBlock
  The memory block to add
 */
/**************************************************************************/
void ObjectAllocator::PushFront(GenericObject* memBlock){
  //Make the old free list the next after the memBlock
  memBlock->Next = _freeList;
  // Make it the start of the free list
  _freeList = memBlock;
  // Increment objects on free list
  ++_stats.FreeObjects_;
}
