 #ifndef WRITE_POOL_HEADER
 #define WRITE_POOL_HEADER
 /***************************************************************************
 *            write_pool.h
 *
 *  Wed Jul  2 11:55:44 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include "../common/utils.h"
#include "../mng/mng.h"
#include <list> 
#include <ext/hash_map>
 
__STXXL_BEGIN_NAMESPACE
 
//! \brief Implements dynamically resizable buffered writing pool
template <class BlockType>
class write_pool
{
public:
  typedef BlockType block_type;
  typedef typename block_type::bid_type bid_type;
  
  // a hack to make wait_any work with busy_entry type
  struct busy_entry
  {
    block_type * block;
    request_ptr req;
    bid_type bid;
    
    busy_entry(): block(NULL) {}
    busy_entry(const busy_entry & a): block(a.block),req(a.req),bid(a.bid) {}
    busy_entry(block_type * & bl, request_ptr & r, bid_type & bi) :
      block(bl),req(r),bid(bi) { }
        
    operator request_ptr () { return req; }
  };
  //typedef __gnu_cxx::hash_map < bid_type, request_ptr , bid_hash > hash_map_type;
  //typedef typename hash_map_type::iterator block_track_iterator;
  typedef typename std::list<block_type *>::iterator free_blocks_iterator;
  typedef typename std::list<busy_entry>::iterator busy_blocks_iterator;
protected:
   
  // contains free write blocks
  std::list<block_type *> free_blocks;
  // blocks that are in writing
	std::list<busy_entry> busy_blocks;
  
  //hash_map_type block_track;
  
  unsigned free_blocks_size,busy_blocks_size;
public:
  //! \brief Constructs pool
  //! \param init_size initial number of blocks in the pool
  write_pool(unsigned init_size=1):free_blocks_size(init_size),busy_blocks_size(0)
  {
    unsigned i = 0;
    for(;i<init_size;i++)
      free_blocks.push_back(new block_type);
  }
  //! \brief Waits for completion of all ongoing write requests and frees memory
  virtual ~write_pool()
  {
    STXXL_VERBOSE2("write_pool::~write_pool free_blocks_size: "<<
      free_blocks_size<<" busy_blocks_size: "<<busy_blocks_size)
    while(!free_blocks.empty())
    {
      delete free_blocks.back();
      free_blocks.pop_back();
    }
    
    busy_blocks_iterator i2 = busy_blocks.begin();
    for(;i2 != busy_blocks.end();i2++)
    {
      i2->req->wait();
      delete i2->block;
    }
  }
  
  //! \brief Returns number of owned blocks
  unsigned size() const { return free_blocks_size + busy_blocks_size; }
  
  //! \brief Passes a block to the pool for writing
  //! \param block block to write. Ownership of the block goes to the pool. 
  //! \c block must be allocated dynamically with using \c new .
  //! \param bid location, where to write
  //! \warning \c block must be allocated dynamically with using \c new .
  //! \return request object of the write operation
  request_ptr write(block_type * block,bid_type bid)
  {
    request_ptr result = block->write(bid);
    ++busy_blocks_size;
    busy_blocks.push_back(busy_entry(block,result,bid));
    return result;
  }
  
  //! \brief Take out a block from the pool
  //! \return pointer to the block. Ownership of the block goes to the caller.
  block_type * steal()
  {
    assert(size() > 0);
    if(free_blocks_size)
    {
      --free_blocks_size;
      block_type * p = free_blocks.back();
      free_blocks.pop_back();
      return p;
    }
    busy_blocks_iterator completed = wait_any(busy_blocks.begin(),busy_blocks.end());
    
    assert(completed != busy_blocks.end()); // we got something reasonable from wait_any
    assert(completed->req->poll()); // and it is *really* completed
    block_type * p = completed->block;
    busy_blocks.erase(completed);
    --busy_blocks_size;
    return p;
  }
  
  // depricated name for the steal()
  block_type * get()
  {
    return steal();
  }
  
  //! \brief Resizes size of the pool
  //! \param new_size new size of the pool after the call
  void resize(unsigned new_size)
  {
    int diff = int(new_size) - int(size());
    if(diff > 0 )
    {
      free_blocks_size += diff;
      while(--diff >= 0)
        free_blocks.push_back(new block_type);
      
      return;
    }
    
    while(++diff <= 0)
      delete get();
  }
  
  request_ptr get_request(bid_type bid)
  {
    busy_blocks_iterator i2 = busy_blocks.begin();
    for(;i2 != busy_blocks.end();i2++)
    {
      if(i2->bid == bid)
        return i2->req;
    }
    return request_ptr();
  }
  
  
  block_type * steal(bid_type bid)
  {
    busy_blocks_iterator i2 = busy_blocks.begin();
    for(;i2 != busy_blocks.end();i2++)
    {
      if(i2->bid == bid)
      {
        block_type * p = i2->block;
        i2->req->wait();
        busy_blocks.erase(i2);
        --busy_blocks_size;
        return p;
      }
    }
    return NULL;
  }
  
  void  add(block_type * block)
  {
    free_blocks.push_back(block);
    ++free_blocks_size;
  }
};
 
__STXXL_END_NAMESPACE
 
 #endif
