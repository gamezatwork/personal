/**************************************************************************/
  /*!
  \file   BList.cpp
  \author Wesley Kam
  \par    email:kam.z\@digipen.edu
  \par    DigiPen login: kam.z
  \par    Course: CS280
  \par    Assignment #2
  \date   10/03/2014
  \brief 
  Brief
    \n Contains definitions for the BList class, which is a double-linked list
    \n containing multiple elements within the list.
    \n Also contains the RoundFloat helper function.
    \n
    \n FUNCTION DEFINITIONS
    \n  ([p] stands for private)
    \n [Empty namespace] (For utility)
    \n  RoundFloat(float)                 => Rounds a float and returns an int
    \n      
    \n BList class
    \n  STATIC FUNCTIONS
    \n    nodesize()                      => Returns the size of a node
    \n  PUBLIC FUNCTIONS
    \n    operator=(const BList&)         => Copy Assignment
    \n    push_back(const T&)             => Pushes the value into the back
    \n    push_front(const T&)            => Pushes the value into the front
    \n    insert(const T&)                => Inserts the element at sorted pos
    \n    remove(int)                     => Removes element at index
    \n    remove_by_value(const T&)       => Removes element with value
    \n    find(const T&)                  => Returns index of element with value
    \n    operator[](int)                 => Returns element at index
    \n    size()                          => Returns amount of elems
    \n    clear()                         => Clears entire list
    \n    GetHead()                       => Returns the head of the list
    \n    GetStats()                      => Returns the stats
    \n  PRIVATE FUNCTIONS
    \n  (Node functions)
    \n [p]_CreateNode(BNode*, BNode*)     => Creates a new node between
    \n [p]_DeleteNode(BNode*)             => Deletes the node
    \n [p]_SplitNode(BNode*)              => Splits node into 2
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

/**************************************************************************/
  /*!
  \brief 
    Contains helper functions for BList class
  */
/**************************************************************************/
namespace{
  /**************************************************************************/
  /*!
    \brief 
    Rounds up/down the float and returns the integral version
    
    \param [in] theFloat
    The float to round
    
    \return 
    The rounded result
   */
  /**************************************************************************/
  int RoundFloat(float theFloat)
  {
    // Get the int value
    int intVal = static_cast<int>(theFloat);
    // Get the remainder
    float remainder = theFloat - static_cast<float>(intVal);
    // if the remainder is more than or equal to 0.5, add 1
    if (remainder >= 0.5f) ++intVal;
    // return the val
    return intVal;
  }
}

// Static functions

/**************************************************************************/
/*!
  \brief 
  Returns the size of a node
  
  \return 
  The size of a node
 */
/**************************************************************************/
template <typename T, unsigned Size>
unsigned BList<T, Size>::nodesize(void)
{
  return sizeof(BNode);
}

// Public functions

/**************************************************************************/
/*!
  \brief 
  Default constructor for BList
 */
/**************************************************************************/
template <typename T, unsigned Size>
BList<T, Size>::BList() : head_(NULL), tail_(NULL), _isSorted(true), 
                          _stats(sizeof(BNode), 0, Size, 0){}

/**************************************************************************/
/*!
  \brief 
  Copy constructor for BList (Deep copy)
 */
/**************************************************************************/
template <typename T, unsigned Size>
BList<T, Size>::BList(const BList &rhs) : head_(NULL), tail_(NULL), 
                                          _isSorted(rhs._isSorted), 
                                          _stats(sizeof(BNode), 0, Size, 0)
{
  // Go through each of the nodes, copying over the info
  // The node from the rhs
  BNode* rhsNode = rhs.head_;
  // The node before that
  BNode* prevNode = NULL;
  while (rhsNode){
    // Create a new node
    BNode* theNode = _CreateNode(NULL, prevNode);
    // Push in all the values
    for (unsigned i = 0; i < rhsNode->count; ++i) 
      _InsertIntoNode(theNode, theNode->count, rhsNode->values[i]);
    // Store the node
    prevNode = theNode;
    // Go to the next node
    rhsNode = rhsNode->next;
  }
}

/**************************************************************************/
/*!
  \brief 
  Destructor for BList
 */
/**************************************************************************/
template <typename T, unsigned Size>
BList<T, Size>::~BList()
{
  // Just call clear
  clear();
}

/**************************************************************************/
/*!
  \brief 
  Copy assignment, does deep copy of rhs, deletes old elements.
  
  \param [in] rhs
  The BList to copy
  
  \return 
  The BList calling the operator
 */
/**************************************************************************/
template <typename T, unsigned Size>
BList<T, Size>& BList<T, Size>::operator=(const BList& rhs)throw(BListException)
{
  // Clear your old nodes first
  clear();
  // The node from the rhs
  BNode* rhsNode = rhs.head_;
  // The node before that
  BNode* prevNode = NULL;
  // The new node
  BNode* theNode;
  // Go through each of the nodes, copying over the info
  while (rhsNode){
    // Attempt to create a new node
    try {theNode = _CreateNode(NULL, prevNode);}
    catch (BListException& ba){
      // Call clear
      clear();
      // And then throw it on
      throw(ba);
    }
    // Push in all the values
    for (unsigned i = 0; i < rhsNode->count; ++i) 
      _InsertIntoNode(theNode, theNode->count, rhsNode->values[i]);
    // Store the node
    prevNode = theNode;
    // Go to the next node
    rhsNode = rhsNode->next;
  }
  // Return yourself
  return *this;
}

/**************************************************************************/
/*!
  \brief 
  Pushes an element into the back of the BList.
  Note: List will be considered unsorted.
  
  \param [in] value
    The value to push in
 */
/**************************************************************************/
template <typename T, unsigned Size>
void BList<T, Size>::push_back(const T& value) throw(BListException)
{
  // Welp, it ain't sorted anymore
  _isSorted = false;
  // If no tail, its an empty list, just add it in
  if (tail_ == NULL)
  {
    // Create the node
    BNode* newNode = _CreateNode();
    // Add in the value
    _InsertIntoNode(newNode, 0, value);
    // And just set it as the head and the tail
    head_ = newNode; tail_ = newNode;
  }
  // If there is a tail, see whether you can add it there
  else
  {
    BNode* theNode = tail_;
    // If it is a full node, add in a new node first
    if (tail_->count == Size) theNode = _CreateNode(NULL, tail_);
    // Then push it in
    _InsertIntoNode(theNode, theNode->count, value);
  }
}

/**************************************************************************/
/*!
  \brief 
  Pushes an element into the front of the BList.
  Note: List will be considered unsorted.
  
  \param [in] value
    The value to push in
 */
/**************************************************************************/
template <typename T, unsigned Size>
void BList<T, Size>::push_front(const T& value) throw(BListException)
{
  // Welp, it ain't sorted anymore
  _isSorted = false;
  // If no head, its an empty list, just add it in
  if (head_ == NULL)
  {
    // Create the node
    BNode* newNode = _CreateNode();
    // Add in the value
    _InsertIntoNode(newNode, 0, value);
    // And just set it as the head and the tail
    head_ = newNode; tail_ = newNode;
  }
  // If there is a head, see whether you can push it in there
  else
  {
    BNode* theNode = head_;
    // if it is a full node, add in a new node first
    if (head_->count == Size) theNode = _CreateNode(head_, NULL);
    // Then push it in
    _InsertIntoNode(theNode, 0, value);
  }
}

/**************************************************************************/
/*!
  \brief 
  Inserts the value into the BList at the appropriate location.
  (Assumes the BList is sorted)
  
  \param [in] value
    The value to insert
 */
/**************************************************************************/
template <typename T, unsigned Size>
void BList<T, Size>::insert(const T& value) throw(BListException)
{
  // Start from the head
  BNode* theNode = head_;
  // If it is an empty list, just add it in and stuff it there
  if (theNode == NULL){ push_back(value); return; }
  // Store the previous node
  BNode* prevNode = NULL;
  // Find the first node which has a head value larger than value
  while (theNode && theNode->values[0] < value){
    prevNode = theNode;
    theNode = theNode->next;
  }
  // If there is a previous, grab the previous one (because you will add there)
  if (prevNode) theNode = prevNode;
  // Find the first position in the array which has a larger value
  unsigned thePos = 0;
  while (thePos != theNode->count && theNode->values[thePos] < value) ++thePos;
  // If there isn't space in the node, split the node into two first
  if (theNode->count >= Size){
    unsigned theSize = Size;
    // Ensure it is not the special case Size ==1
    if (theSize != 1){
      // First, check to see whether there is any space in the next node
      if (thePos >= theNode->count&&theNode->next && theNode->next->count<Size){
        // if there is, just use that instead
        // Reduce the pos and go to the newly splitted node
        thePos -= theNode->count; theNode = theNode->next;
      }
      // No space, just split
      else{
        // Split the node
        theNode = _SplitNode(theNode);
        // If the pos is more than the first one, use the new node
        if (thePos > theNode->count){
          // Reduce the pos and go to the newly splitted node
          thePos -= theNode->count; theNode = theNode->next;
        }
      }
    }
    // If it is, add a node in front or behind based on the value
    else{
      // The new node
      BNode* newNode;
      // If the value is less, add a node to the back and put it there
      if (thePos == 1) newNode = _CreateNode(theNode->next, theNode);
      // if not, add a node to the front and put it there
      else newNode = _CreateNode(theNode, theNode->prev);
      // Then put it there
      theNode = newNode; thePos = 0;
    }
  }// Then stuff it into the appropriate node
  _InsertIntoNode(theNode, thePos, value);
}

/**************************************************************************/
/*!
  \brief 
  Removes the element at the index specified.
  
  \param [in] index
    The index at which the element to be removed is
 */
/**************************************************************************/
template <typename T, unsigned Size>
void BList<T, Size>::remove(int index) throw(BListException)
{
  // If the index is more than the ItemCount, throw exception
  if (index >= static_cast<int>(_stats.ItemCount) || index < 0) 
  throw(BListException(BListException::E_BAD_INDEX, "Index is out of bounds!"));
  // Go to the appropriate node
  BNode* theNode = head_;
  while (theNode && index >= static_cast<int>(theNode->count)){
    // Decrement the index by the node's size
    index -= theNode->count;
    // Go to the next node
    theNode = theNode->next;
  }
  // Then use that node for removing
  _RemoveFromNode(theNode, static_cast<size_t>(index));
}

/**************************************************************************/
/*!
  \brief 
  Removes the element which has the value stated
  
  \param [in] value
    The value to remove
 */
/**************************************************************************/
template <typename T, unsigned Size>
void BList<T, Size>::remove_by_value(const T& value)
{
  // Find the value
  int thePos = find(value);
  // If it is not negative, remove it
  if (thePos >= 0) remove(thePos);
}

/**************************************************************************/
/*!
  \brief 
  Finds the index at which the element with the value is located.
  
  \param [in] value
    The value to search for
    
  \return
    The index if it could find the element, -1 if it couldn't
 */
/**************************************************************************/
template <typename T, unsigned Size>
int BList<T, Size>::find(const T& value) const
{
  // If the list is empty, return -1
  if (!head_) return -1;
  // Start from the head
  BNode* theNode = head_;
  int currentPos = 0;
  // If it is sorted, look for the appropriate node
  if (_isSorted){
    // store previous node
    BNode* prevNode = NULL;
    // Find the first node which has a head value larger than value
    while (theNode && (theNode->values[0] < value||theNode->values[0] == value))
    {
      prevNode = theNode;
      // Increment the current pos by the node's size
      currentPos += theNode->count;
      // Go to the next node
      theNode = theNode->next;
    }
    // If there is no previous, you are accessing the first, itsa cool
    // If there is, grab the previous one
    if (prevNode) theNode = prevNode;
    // Use the node to look
    return currentPos - theNode->count + _FindInNode(theNode, value);
  }
  // if it is not, just go through all elements
  else{
    // Go through each node
    int foundPos = -1;
    while (theNode){
      // Increment the current pos by the node's size
      currentPos += theNode->count;
      // See whether you could find it
      foundPos = _FindInNode(theNode, value);
    // If you could find it, return
    if (foundPos != -1) break;
      // Go to the next node
      theNode = theNode->next;
    }
    // if it couldn't find it, just return -1
    if (foundPos == -1) return -1;
    // if it could find something, return it
    return currentPos - theNode->count + foundPos;
  }
}

/**************************************************************************/
/*!
  \brief 
  Returns the element at the index.
  
  \param [in] index
    The index to get the element from
    
  \return
    A reference to the element
 */
/**************************************************************************/
template <typename T, unsigned Size>
T& BList<T, Size>::operator[](int index) throw(BListException)
{
  // use the const version for this
  return const_cast<T&>(const_cast<const BList<T,Size>&>(*this)[index]);
}

/**************************************************************************/
/*!
  \brief 
  Returns the element at the index. (Const version)
  
  \param [in] index
    The index to get the element from
    
  \return
    A const reference to the element
 */
/**************************************************************************/
template <typename T, unsigned Size>
const T& BList<T, Size>::operator[](int index) const throw(BListException)
{
  // If the index is out of boundaries, throw exception
  if (index < 0 || static_cast<unsigned>(index) >= _stats.ItemCount) 
  throw(BListException(BListException::E_BAD_INDEX, "Index is out of bounds!"));
  // Keep jumping through the nodes until your index is less than the count
  BNode* theNode = head_;
  while (static_cast<unsigned>(index) >= theNode->count)
  {
    // Reduce the index
    index -= theNode->count;
    // Go to next node
    theNode = theNode->next;
  }
  // Found the node that you belong to, grab that one
  return theNode->values[index];
}

/**************************************************************************/
/*!
  \brief 
  Returns the total number of elements
  
  \return
    The total number of elements in the list
 */
/**************************************************************************/
template <typename T, unsigned Size>
unsigned BList<T, Size>::size(void) const
{
  // Just return teh stats
  return _stats.ItemCount;
}

/**************************************************************************/
/*!
  \brief 
  Deletes all the nodes in the BList
 */
/**************************************************************************/
template <typename T, unsigned Size>
void BList<T, Size>::clear(void)
{
  // Go through all the nodes, deleting all of them
  while (head_) _DeleteNode(head_);
  // And set your stats to 0
  _stats.ItemCount = 0;
  _stats.NodeCount = 0;
}

/**************************************************************************/
/*!
  \brief 
  Returns the head of the list.
  
  \return
    The first element in the list
 */
/**************************************************************************/
template <typename T, unsigned Size>
const typename BList<T, Size>::BNode* BList<T, Size>::GetHead() const
{
  // Just grab the head
  return head_; 
}

/**************************************************************************/
/*!
  \brief 
  Returns the stats of the list.
  
  \return
    The stats of the list
 */
/**************************************************************************/
template <typename T, unsigned Size>
BListStats BList<T, Size>::GetStats() const
{
  // Just grab the stats
  return _stats;
}

// Private functions

/**************************************************************************/
/*!
  \brief 
  Adds an empty node in between afterNode and beforeNode, and returns the
  new node.
  
  \param [in] afterNode
    The node which will be after the new node
  \param [in] beforeNode
    The node which will be before the new node
  
  \return
    The new node
 */
/**************************************************************************/
template <typename T, unsigned Size>
typename BList<T, Size>::BNode* BList<T, Size>::_CreateNode(BNode* afterNode, 
                                                            BNode* beforeNode) 
                                                           throw(BListException)
{
  // The new node
  BNode* newNode;
  // Try to allocate for a new node
  try{ newNode = new BNode(); }
  // Problem allocation, throw another exception
  catch (std::bad_alloc& ba)
  { throw(BListException(BListException::E_NO_MEMORY, ba.what())); }
  // Set the values to invalid
  for (unsigned i = 0; i < Size; ++i) newNode->values[i] = T();
  // If there is no node before that, its at the start of the list
  if (beforeNode == NULL)
  {
    // Make the head be the next of the newNode
    newNode->next = head_;
    if (head_) head_->prev = newNode;
    // Update the head
    head_ = newNode;
  }
  // If not, then set it as the next
  else
  {
    // Set before to point to this next
    beforeNode->next = newNode;
    // Set newNode to opint to this prev
    newNode->prev = beforeNode;
  }
  // If there is no node after that, its at the end of the list
  if (afterNode == NULL)
  {
    // Make the tail be the prev of the newNode
    newNode->prev = tail_; 
    if (tail_) tail_->next = newNode;
    // Update the tail
    tail_ = newNode;
  }
  // If not, set it as the previous
  else
  {
    // Set after to point to this as prev
    afterNode->prev = newNode;
    // set newNode to point to this as next
    newNode->next = afterNode;
  }
  // INcrement the count
  ++_stats.NodeCount;
  // Then return it
  return newNode;
}

/**************************************************************************/
/*!
  \brief 
  Deletes the node and sets up the prev and nexts.
  
  \param [in] theNode
    The node to delete
 */
/**************************************************************************/
// Removes a node (delete)
template <typename T, unsigned Size>
void BList<T, Size>::_DeleteNode(typename BList<T, Size>::BNode* theNode)
{
  // If there is a prev, set it to point to the next
  if (theNode->prev) theNode->prev->next = theNode->next;
  // if there isn't, its at the start, set the next as the first
  else head_ = theNode->next;
  // if it has a next, update next's prev
  if (theNode->next) theNode->next->prev = theNode->prev;
  // If there isn't, its at the end, update the tail
  else tail_ = theNode->prev;
  // Decrement the count
  --_stats.NodeCount;
  // Then just delete it
  delete theNode;
}

/**************************************************************************/
/*!
  \brief 
  Inserts an element into the node at the position specified
  
  \param [in] theNode
    The node to insert the element into
  \param [in] pos
    The position to insert the element into
  \param [in] theElem
    The element to insert
 */
/**************************************************************************/
template <typename T, unsigned Size>
void BList<T, Size>::_InsertIntoNode(typename BList<T, Size>::BNode* theNode, 
                                      size_t pos, 
                                      const T& theElem)
{
  // Go through the elements from the back, moving each of them front by 1
  for (unsigned i = theNode->count; i > pos; --i)
  {
    //std::cout << "Moving " << i << " backwards: " << pos << std::endl;
    theNode->values[i] = theNode->values[i-1];
  }
  // Then put the new elem in the first position
  theNode->values[pos] = theElem;
  // INcremnt the count
  ++theNode->count;
  ++_stats.ItemCount;
}

/**************************************************************************/
/*!
  \brief 
  Removes the element at the position specified from the node
  
  \param [in] theNode
    The node to remove the element from
  \param [in] pos
    The position which contains the element to remove
 */
/**************************************************************************/
template <typename T, unsigned Size>
void BList<T, Size>::_RemoveFromNode(typename BList<T, Size>::BNode* theNode, 
                                     size_t pos)
{
  // Go through the elements from the position, moving them back by 1
  for (size_t i = pos; i < (theNode->count-1); ++i)
  {
    theNode->values[i] = theNode->values[i+1];
  }
  // Then null out the last element
  theNode->values[theNode->count-1] = T();
  // Decremnt the count
  --theNode->count;
  --_stats.ItemCount;
  // If the node is now empty, delete the node as well
  if (theNode->count == 0) _DeleteNode(theNode);
}

/**************************************************************************/
/*!
  \brief 
  Split a node into two (creating an extra node), returns pointer to the 
  first node. The elements will also be split across the two nodes.
  
  \param [in] theNode
    The node to split
  
  \return
    The original node
 */
/**************************************************************************/
template <typename T, unsigned Size>
typename BList<T, Size>::BNode* BList<T, Size>::_SplitNode(
                                        typename BList<T, Size>::BNode* theNode)
{
  // Create a new node
  BNode* newNode = _CreateNode(theNode->next, theNode);
  // Copy over the second half of the values and null yourself
  for (int i = Size / 2; i < Size; ++i)
  {
    // Insert it to the back of the new node
    _InsertIntoNode(newNode, newNode->count, theNode->values[i]);
    // null out yourself
    theNode->values[i] = T();
    // And decrement the count
    --theNode->count;
    --_stats.ItemCount;
  }
  
  // Return the original node
  return theNode;
}

/**************************************************************************/
/*!
  \brief 
  Search in the node for the value.
  (Will call LinearSearch if it is not sorted, BinarySearch if it is)
  
  \param [in] theNode
    The node to search in
  \param [in] theVal
    The value to search for
  
  \return
    The index of the value if it was found, -1 if it wasn't.
 */
/**************************************************************************/
template <typename T, unsigned Size>
int BList<T, Size>::_FindInNode(typename BList<T, Size>::BNode* theNode, 
                                const T& theVal) const
{
  // If it is sorted, call binarysearch
  if (_isSorted) return _BinarySearch(theNode, theVal);
  // If not, call linearsearch
  else return _LinearSearch(theNode, theVal);
}

/**************************************************************************/
/*!
  \brief 
  Linear search for the value in the list.
  (Assumed to be unsorted)
  
  \param [in] theNode
    The node to search in
  \param [in] theVal
    The value to search for
  
  \return
    The index of the value if it was found, -1 if it wasn't.
 */
/**************************************************************************/
template <typename T, unsigned Size>
int BList<T, Size>::_LinearSearch(typename BList<T, Size>::BNode* theNode, 
                                  const T& valToSearch) const
{
  // Just go through the entire thing until you find it
  int thePos = 0;
  while (thePos < static_cast<int>(theNode->count) && 
          !(theNode->values[thePos] == valToSearch)) ++thePos;
  // If it failed, return -1
  if (thePos == static_cast<int>(theNode->count)) return -1;
  // If not, return the position
  return thePos;
}

/**************************************************************************/
/*!
  \brief 
  Binary search for the value in the list.
  (Assumed to be sorted)
  
  \param [in] theNode
    The node to search in
  \param [in] theVal
    The value to search for
  
  \return
    The index of the value if it was found, -1 if it wasn't.
 */
/**************************************************************************/
template <typename T, unsigned Size>
int BList<T, Size>::_BinarySearch(typename BList<T, Size>::BNode* theNode, 
                                  const T& valToSearch) const
{
  // Start from the middle
  int thePos = theNode->count/2;
  // The starting size of the subarray
  int subSize = theNode->count/2;
  // Keep iterating until you find it
  while (!(theNode->values[thePos] == valToSearch) && subSize != 0)
  {
    // If it is more than, move to the left
    if (valToSearch < theNode->values[thePos]) thePos -= subSize/2;
    // If it is less than, move to the right
    else if (theNode->values[thePos] < valToSearch) thePos += subSize/2;
    // Cut subSize by half and round it if it is not 1
    if (subSize != 1) subSize = RoundFloat(static_cast<float>(subSize)/2.0f);
    // If it is one, you're done
    else subSize = 0;
  }
  // If the value at the pos is the val to search, you're done
  if (theNode->values[thePos] == valToSearch) return thePos;
  // If not, you failed, return -1
  return -1;
}
