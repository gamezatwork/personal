/**************************************************************************/
  /*!
  \file   ObjectAllocator.h
  \author Wesley Kam
  \par    email:kam.z\@digipen.edu
  \par    DigiPen login: kam.z
  \par    Course: CS280
  \par    Assignment #1
  \date   09/18/2014
  \brief 
  Brief
    \n Contains declarations for an ObjectAllocator class, which allows users
    \n to make use of a MemBlock system to allocate and deallocate data.
    \n Also contains other structs which interact with the ObjectAllocator class
    \n
    \n INTERFACE
    \n
    \n Static variables
    \n    DEFAULT_OBJECTS_PER_PAGE    => The default objects per page
    \n    DEFAULT_MAX_PAGES           => The default maximum pages
    \n
    \n OAException
    \n  Enums
    \n    OA_Exception                => Exceptions for the ObjectAllocator
    \n  Functions
    \n    code                        => Returns the exception code
    \n    what                        => Returns information about the throw
    \n
    \n OAConfig
    \n  Static Variables
    \n    BASIC_HEADER_SIZE           => Basic header's size
    \n    EXTERNAL_HEADER_SIZE        => External header's size
    \n  Enums
    \n    HBLOCK_TYPE                 => Types of header blocks
    \n  Variables
    \n    UseCPPMemManager_           => by-pass the functionality of the OA
    \n    ObjectsPerPage_             => number of objects on each page
    \n    MaxPages_                   => maximum number of pages OA can allocate
    \n    DebugOn_                    => enable/disable debugging code
    \n    PadBytes_                   => size of the left/right padding perblock
    \n    HBlockInfo_                 => size of the header for each block
    \n    Alignment_                  => address alignment of each block
    \n    LeftAlignSize_              => number of bytes to align first blk
    \n    InterAlignSize_             => number of bytes between remaining
    \n  Subclasses
    \n    HeaderBlockInfo             => Contains info about the header
    \n
    \n OAStats
    \n  Variables
    \n    ObjectSize_                 => size of each object
    \n    PageSize_                   => size of a page
    \n    FreeObjects_                => number of objects on the free list
    \n    ObjectsInUse_               => number of objects in use by client
    \n    PagesInUse_                 => number of pages allocated
    \n    MostObjects_                => most objects in use by client
    \n    Allocations_                => total requests to allocate memory
    \n    Deallocations_              => total requests to free memory
    \n
    \n MemBlockInfo
    \n  Variables
    \n    in_use                      => Is the block free or in use?
    \n    alloc_num                   => The allocation number (count) of this
    \n    label                       => A dynamically allocated c-style string
    \n
    \n ObjectAllocator
    \n  Static Variables
    \n    UNALLOCATED_PATTERN         => Unallocated pattern
    \n    ALLOCATED_PATTERN           => Allocated pattern
    \n    FREED_PATTERN               => Freed pattern
    \n    PAD_PATTERN                 => Padding pattern
    \n    ALIGN_PATTERN               => Alignment pattern
    \n Functions
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
    \n
  Specific portions that gave you the most trouble:
    Typing out all the functions. It was alot of it.
  */
/**************************************************************************/
//---------------------------------------------------------------------------
#ifndef OBJECTALLOCATORH
#define OBJECTALLOCATORH
//---------------------------------------------------------------------------

// suppress warning: C++ Exception Specification ignored
#ifdef _MSC_VER
#pragma warning( disable : 4290 )
#endif

#include <string>       /* std::string */
#include <iostream>

// If the client doesn't specify these:
static const int DEFAULT_OBJECTS_PER_PAGE = 4;  //!< Default objects per page
static const int DEFAULT_MAX_PAGES = 3;         //!< Default pages

/**************************************************************************/
/*!
  \brief 
  The exception class for the ObjectAllocator
 */
/**************************************************************************/
class OAException
{
  public:
    /**************************************************************************/
    /*!
      \brief 
      Possible exception codes
     */
    /**************************************************************************/
    enum OA_EXCEPTION 
    {
      E_NO_MEMORY,      //!< out of physical memory (operator new fails)
      E_NO_PAGES,       //!< out of logical memory (max pages has been reached)
      E_BAD_BOUNDARY,   //!< block address is on a page, but not on any boundary
      E_MULTIPLE_FREE,  //!< block has already been freed
      E_CORRUPTED_BLOCK //!< block has been corrupted (pad bytes overwritten)
    };
    
    /**************************************************************************/
    /*!
      \brief 
      Default constructor for OAException
      
      \param [in] ErrCode
      The error code
      \param [in] Message
      The message
     */
    /**************************************************************************/
    OAException(OA_EXCEPTION ErrCode, const std::string& Message) : 
                error_code_(ErrCode), message_(Message) {};
                  
    /**************************************************************************/
    /*!
      \brief 
      Default destructor for OAException
     */
    /**************************************************************************/
    virtual ~OAException() {
    }
    
    /**************************************************************************/
    /*!
      \brief 
      Returns the exception code
      \return
      The exception code
     */
    /**************************************************************************/
    OA_EXCEPTION code(void) const { 
      return error_code_; 
    }
    
    /**************************************************************************/
    /*!
      \brief 
      Returns the message
      \return
      The message
     */
    /**************************************************************************/
    virtual const char *what(void) const {
      return message_.c_str();
    }
  private:  
    OA_EXCEPTION error_code_;   //!< The error code
    std::string message_;       //!< The message
};

/**************************************************************************/
/*!
  \brief 
  ObjectAllocator configuration parameters
 */
/**************************************************************************/
struct OAConfig
{
  static const size_t BASIC_HEADER_SIZE = sizeof(unsigned) + 1;
  //!< allocation number + flags
  static const size_t EXTERNAL_HEADER_SIZE = sizeof(void*);
  //!< just a pointer
  
  /**************************************************************************/
  /*!
    \brief 
    The types of headers
   */
  /**************************************************************************/
  enum HBLOCK_TYPE{hbNone, hbBasic, hbExtended, hbExternal};
  
  /**************************************************************************/
  /*!
    \brief 
    The info about the headers
   */
  /**************************************************************************/
  struct HeaderBlockInfo
  {
    HBLOCK_TYPE type_;      //!< The type of the header
    size_t size_;           //!< The full size of the header
    size_t additional_;     //!< The additional size specified by the user
    /**************************************************************************/
    /*!
      \brief 
      Default constructor for the header block info
      \param [in] type
      The type of header
      \param [in] additional
      The additional user defined space (only for external)
     */
    /**************************************************************************/
    HeaderBlockInfo(HBLOCK_TYPE type = hbNone, unsigned additional = 0) : 
                    type_(type), size_(0), additional_(additional)
    {
      if (type_ == hbBasic)
        size_ = BASIC_HEADER_SIZE;
      else if (type_ == hbExtended) // alloc # + use counter + flag byte + user-defined
        size_ = sizeof(unsigned int) + sizeof(unsigned short) + 
                sizeof(char) + additional_;
      else if (type_ == hbExternal)
        size_ = EXTERNAL_HEADER_SIZE;
    };
  };
  
  /**************************************************************************/
  /*!
    \brief 
    Default constructor for OAConfig
    \param [in] UseCPPMemManager
    Use New/Delete instead of our manager?
    \param [in] ObjectsPerPage
    The amount of objects per page
    \param [in] MaxPages
    The maximum amount of pages allowed
    \param [in] DebugOn
    Whether it shows debugging information
    \param [in] PadBytes
    The amount of bytes for padding
    \param [in] HBInfo
    The information about the headers
    \param [in] Alignment
    The alignment for the data
   */
  /**************************************************************************/
  OAConfig(bool UseCPPMemManager = false,
           unsigned ObjectsPerPage = DEFAULT_OBJECTS_PER_PAGE, 
           unsigned MaxPages = DEFAULT_MAX_PAGES, 
           bool DebugOn = false, 
           unsigned PadBytes = 0,
           const HeaderBlockInfo &HBInfo = HeaderBlockInfo(),
           unsigned Alignment = 0) : UseCPPMemManager_(UseCPPMemManager),
                                     ObjectsPerPage_(ObjectsPerPage), 
                                     MaxPages_(MaxPages), 
                                     DebugOn_(DebugOn), 
                                     PadBytes_(PadBytes),
                                     HBlockInfo_(HBInfo),
                                     Alignment_(Alignment)
  {
    HBlockInfo_ = HBInfo;
    LeftAlignSize_ = 0;  
    InterAlignSize_ = 0;
  }

  bool UseCPPMemManager_;       //!< by-pass the functionality of the OA
  unsigned ObjectsPerPage_;     //!< number of objects on each page
  unsigned MaxPages_;           //!< maximum number of pages the OA can allocate
  bool DebugOn_;                //!< enable/disable debugging code
  unsigned PadBytes_;           //!< size of the left/right padding per block
  HeaderBlockInfo HBlockInfo_;  //!< size of the header for each block
  unsigned Alignment_;          //!< address alignment of each block

  unsigned LeftAlignSize_;      //!< number of bytes required to align first blk
  unsigned InterAlignSize_;     //!< number of bytes required between remaining
};

/**************************************************************************/
/*!
  \brief 
  ObjectAllocator statistical info
 */
/**************************************************************************/
struct OAStats
{
  /**************************************************************************/
  /*!
    \brief 
    Default constructor for OAStats
   */
  /**************************************************************************/
  OAStats(void) : ObjectSize_(0), PageSize_(0), FreeObjects_(0), 
                  ObjectsInUse_(0), PagesInUse_(0), MostObjects_(0), 
                  Allocations_(0), Deallocations_(0) {};

  size_t ObjectSize_;      //!< size of each object
  size_t PageSize_;        //!< size of a page including all headers, etc.
  unsigned FreeObjects_;   //!< number of objects on the free list
  unsigned ObjectsInUse_;  //!< number of objects in use by client
  unsigned PagesInUse_;    //!< number of pages allocated
  unsigned MostObjects_;   //!< most objects in use by client at one time
  unsigned Allocations_;   //!< total requests to allocate memory
  unsigned Deallocations_; //!< total requests to free memory
};

/**************************************************************************/
/*!
  \brief 
  A node class which we can use to treat raw objects as nodes in a linked
  list.
 */
/**************************************************************************/
struct GenericObject
{
  GenericObject *Next;  //!< The next object
};

/**************************************************************************/
/*!
  \brief 
  Information about the memory block (for external header)
 */
/**************************************************************************/
struct MemBlockInfo
{
  bool in_use;        //!< Is the block free or in use?
  unsigned alloc_num; //!< The allocation number (count) of this block
  char *label;        //!< A dynamically allocated NUL-terminated string
};

/**************************************************************************/
/*!
  \brief 
  The Object Allocator (Memory manager)
 */
/**************************************************************************/
class ObjectAllocator
{
  public:
    typedef void (*DUMPCALLBACK)(const void *, size_t);
    //!< Defined by the client (pointer to a block, size of block)
    typedef void (*VALIDATECALLBACK)(const void *, size_t);
    //!< Defined by the client (pointer to a block, size of block)

    // Predefined values for memory signatures
    static const unsigned char UNALLOCATED_PATTERN = 0xAA;  //!< Unallocated
    static const unsigned char ALLOCATED_PATTERN = 0xBB;    //!< Allocated
    static const unsigned char FREED_PATTERN = 0xCC;        //!< Freed
    static const unsigned char PAD_PATTERN = 0xDD;          //!< Padding
    static const unsigned char ALIGN_PATTERN = 0xEE;        //!< Alignment
    
    //!< Creates the ObjectManager
    ObjectAllocator(size_t ObjectSize,const OAConfig& config)throw(OAException);
    
    //!< Destroys the ObjectManager (never throws)
    ~ObjectAllocator() throw();
    
    //!< Passes the client a block of memory
    void *Allocate(const char *label = 0) throw(OAException);

    //!< Frees allocated memory
    void Free(void *Object) throw(OAException);

    //!< Calls the callback fn for each block still in use
    unsigned DumpMemoryInUse(DUMPCALLBACK fn) const;

    //!< Calls the callback fn for each block that is potentially corrupted
    unsigned ValidatePages(VALIDATECALLBACK fn) const;

    //!< Frees all empty pages (extra credit)
    unsigned FreeEmptyPages(void);

    //!< Returns true if FreeEmptyPages and alignments are implemented
    static bool ImplementedExtraCredit(void);

    // Testing/Debugging/Statistic methods
    //!< Sets the debugging state
    void SetDebugState(bool State);
    //!< Returns pointer to free list
    const void *GetFreeList(void) const;
    //!< Returns pointer to page list
    const void *GetPageList(void) const;
    //!< Returns config info
    OAConfig GetConfig(void) const;
    //!< Returns stats info
    OAStats GetStats(void) const;

  private:

    //!< No copy construction
    ObjectAllocator(const ObjectAllocator &oa);
    //!< No assignment
    ObjectAllocator &operator=(const ObjectAllocator &oa);
    
    // Other private fields and methods...
    
    // Private variables
    OAConfig _config;         //!< The configuration options
    OAStats _stats;           //!< The current statistics
    
    GenericObject* _freeList; //!< The free list
    GenericObject* _pageList; //!< The page list
    
    // Exception checking functions
    //!< Check whether the object is in the free list, throw exceptions if it is
    void CheckInFreeList(GenericObject* theObj) const throw(OAException);
    //!< Checks the padding of the object, throws exceptions if it is corrupted
    void CheckPadding(GenericObject* theObj) const throw(OAException);
    //!< Check whether the object is on the boundary, throw exceptions if not
    void CheckOnBoundary(GenericObject* theObj) const throw(OAException);
    // Header functions
    //!< Allocates a new page and sets it up, returns pointer to new page
    GenericObject* AddPage() throw(OAException);
    //!< Frees the page
    void FreePage(GenericObject* thePagePtr);
    //!< Just returns the data portion which belongs to the header
    GenericObject* GetDataPtr(GenericObject* theHeader) const;
    //!< Just returns the pointer to the first header in the list
    GenericObject* GetFirstHeader(GenericObject* theList) const;
    //!< Just returns the pointer to the next header in the list
    GenericObject* GetNextHeader(GenericObject* prevHdr) const;
    //!< Returns whether its an empty page (Non-HeaderNone version)
    bool IsEmptyPage_Header(GenericObject* pagePtr)const;
    //!< Returns whether it is an empty page (HeaderNone version)
    bool IsEmptyPage_NoHeader(GenericObject* pagePtr) const;
    //!< Returns the size of a page based on the config (Modifies the config)
    size_t GetPageSize(size_t ObjectSize, OAConfig& config) const;
    // Data block functions
    //!< Grabs the first mem block from the front of the free list and returns
    GenericObject* PopFront() throw(OAException);
    //!< Adds the memory block to the front of the free list
    void PushFront(GenericObject* memBlock);
};

#endif
