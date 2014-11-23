/**************************************************************************/
  /*!
  \file   BList.h
  \author Wesley Kam
  \par    email:kam.z\@digipen.edu
  \par    DigiPen login: kam.z
  \par    Course: CS280
  \par    Assignment #2
  \date   10/03/2014
  \brief 
  Brief
    \n Contains declarations for the BList class, which is a double-linked list
    \n containing multiple elements within the list.
    \n Also BListException and BListStats.
    \n
    \n BListException
    \n  Enums
    \n    BList_Exception             => Exceptions for the BList
    \n  Functions
    \n    code                        => Returns the exception code
    \n    what                        => Returns information about the throw
    \n
    \n BListStats
    \n  Variables
    \n    NodeSize                    => The size of each node
    \n    NodeCount                   => The number of nodes in the list
    \n    ArraySize                   => The max number of items in each node
    \n    ItemCount                   => The total number of items
    \n
    \n BList
    \n  Subclasses
    \n    BNode                       => A linked list node for the BList
    \n  STATIC FUNCTIONS
    \n    nodesize()                  => Returns the size of a node
    \n  PUBLIC FUNCTIONS
    \n    operator=(const BList&)     => Copy Assignment
    \n    push_back(const T&)         => Pushes the value into the back
    \n    push_front(const T&)        => Pushes the value into the front
    \n    insert(const T&)            => Inserts the element at sorted pos
    \n    remove(int)                 => Removes element at index
    \n    remove_by_value(const T&)   => Removes element with value
    \n    find(const T&)              => Returns index of element with value
    \n    operator[](int)             => Returns element at index
    \n    size()                      => Returns amount of elems
    \n    clear()                     => Clears entire list
    \n    GetHead()                   => Returns the head of the list
    \n    GetStats()                  => Returns the stats
    \n  PRIVATE FUNCTIONS
    \n  (Node functions)
    \n [p]_CreateNode(BNode*, BNode*) => Creates a new node between
    \n [p]_DeleteNode(BNode*)         => Deletes the node
    \n [p]_SplitNode(BNode*)          => Splits node into 2
    \n  (Element functions)
    \n [p]_InsertIntoNode(BNode*,size_t,const T&)  => Inserts elem into node
    \n [p]_RemoveFromNode(BNode*, size_t) => Removes elem at index from node
    \n  (Searching functions)
    \n [p]_LinearSearch(BNode*, const T&) => Linear search for elem in node
    \n [p]_BinarySearch(BNode*, const T&) => Binary search for elem in node
    \n [p]_FindInNode(BNode*, const T&)   => Search for elem in node
    \n
  Specific portions that gave you the most trouble:
    Making insert work properly.
  */
/**************************************************************************/

////////////////////////////////////////////////////////////////////////////////
#ifndef BLIST_H
#define BLIST_H
////////////////////////////////////////////////////////////////////////////////
#include <string>     /* std::string */
#include <stdexcept>  /* std::exception */

#ifdef _MSC_VER
#pragma warning( disable : 4290 ) 
// suppress warning: C++ Exception Specification ignored
#endif

/**************************************************************************/
/*!
  \brief 
  The exception class for the BList
 */
/**************************************************************************/
class BListException : public std::exception
{
  private:  
    int m_ErrCode;              //!< The error code
    std::string m_Description;  //!< The message

  public:
    /**************************************************************************/
    /*!
      \brief 
      Default constructor
     */
    /**************************************************************************/
    BListException(int ErrCode, const std::string& Description) :
    m_ErrCode(ErrCode), m_Description(Description) {};
    
    /**************************************************************************/
    /*!
      \brief 
      Returns the exception code
      \return
      The exception code
     */
    /**************************************************************************/
    virtual int code(void) const { 
      return m_ErrCode; 
    }
    
    /**************************************************************************/
    /*!
      \brief 
      Returns the message
      \return
      The message
     */
    /**************************************************************************/
    virtual const char *what(void) const throw() {
      return m_Description.c_str();
    }
    
    /**************************************************************************/
    /*!
      \brief 
      Default destructor for BListException
     */
    /**************************************************************************/
    virtual ~BListException() throw() {
    }
    
    /**************************************************************************/
    /*!
      \brief 
      Possible exception codes
     */
    /**************************************************************************/
    enum BLIST_EXCEPTION {E_NO_MEMORY, E_BAD_INDEX, E_DATA_ERROR};
};

/**************************************************************************/
/*!
  \brief 
  Stores information about the BList
 */
/**************************************************************************/
struct BListStats
{
  /**************************************************************************/
  /*!
    \brief 
    Default constructor
   */
  /**************************************************************************/
  BListStats() : NodeSize(0), NodeCount(0), ArraySize(0), ItemCount(0)  {};
  
  /**************************************************************************/
  /*!
    \brief 
    Non-default constructor
    
    \param [in] nsize
      The size of each node
    \param [in] ncount
      The number of nods in the list
    \param [in] asize
      The maximum number of items in each node
    \param [in] count
      The number of items in the entire list
   */
  /**************************************************************************/
  BListStats(size_t nsize, size_t ncount, size_t asize, unsigned count) : 
     NodeSize(nsize), NodeCount(ncount), ArraySize(asize), ItemCount(count)  {};

  size_t NodeSize;    //!< Size of a node
  size_t NodeCount;   //!< Number of nodes in the list
  size_t ArraySize;   //!< Max number of items in each node
  unsigned ItemCount; //!< Number of items in the entire list
};  

/**************************************************************************/
/*!
  \brief 
  The BList class, which is a doubly linked list with multiple elements per
  node.
 */
/**************************************************************************/
template <typename T, unsigned Size = 1>
class BList
{
  public:
    /**************************************************************************/
    /*!
      \brief 
      Each node within the BList
     */
    /**************************************************************************/
    struct BNode
    {
      BNode *next;    //!< The next node
      BNode *prev;    //!< The previous node
      unsigned count; //!< number of items currently in the node
      T values[Size]; //!< The elements within the node
      /************************************************************************/
      /*!
        \brief 
        Default constructor
       */
      /************************************************************************/
      BNode() : next(0), prev(0), count(0) {}
    };

    BList();                 //!< default constructor                        
    BList(const BList &rhs); //!< copy constructor
    ~BList();                //!< destructor                         
    
    //!< Copy assignment (deep copy)
    BList& operator=(const BList &rhs) throw(BListException);
    
    //!< Pushes the value into the back (list is now unsorted)
    void push_back(const T& value) throw(BListException);
    //!< Pushes the value into the front (list is now unsorted)
    void push_front(const T& value) throw(BListException);
    //!< Inserts the value into a sorted list
    void insert(const T& value) throw(BListException);
    //!< Removes element at index
    void remove(int index) throw(BListException);
    //!< Removes element with value
    void remove_by_value(const T& value); 
    //!< Returns index of element with value
    int find(const T& value) const;       
    //!< Returns element at index
    T& operator[](int index) throw(BListException);             
    //!< Returns element at index (const version)
    const T& operator[](int index) const throw(BListException); 
    
    //!< Returns amount of elements
    unsigned size(void) const;
    //!< Clears entire list
    void clear(void);
    
    //!< Returns the size of a node
    static unsigned nodesize(void);

    // For debugging
    //!< Returns the head of the list
    const BNode *GetHead() const;
    //!< Returns the stats
    BListStats GetStats() const;

  private:
    BNode *head_;               //!< points to the first node
    BNode *tail_;               //!< points to the last node

    // Other private methods you may need
    
    // Private Variables
    bool _isSorted;             //!< Whether the list is sorted or not
    BListStats _stats;          //!< The stats of the BList
    
    // Private functions
    // Node functions
    //!< Creates an empty node (new) in between the nodes
    BNode* _CreateNode(BNode* afterNode = NULL, BNode* beforeNode = NULL) 
                      throw(BListException);
    //!< Removes a node (delete)
    void _DeleteNode(BNode* theNode);
    //!< Split a node into two, returns pointer to the first node
    BNode* _SplitNode(BNode* theNode);
    
    // Element functions
    //!< Inserts the element into the node
    void _InsertIntoNode(BNode* theNode, size_t pos, const T& theElem);
    //!< Removes the element from the node
    void _RemoveFromNode(BNode* theNode, size_t pos);
    
    // Searching functions
    //!< Linear search for a value in theNode
    int _LinearSearch(BNode* theNode, const T& valToSearch) const;
    //!< Binary search for a value in theNode
    int _BinarySearch(BNode* theNode, const T& valToSearch) const;
    //!< Search in the node for the value
    int _FindInNode(BNode* theNode, const T& theVal) const;
};

#include "BList.cpp"

#endif // BLIST_H
