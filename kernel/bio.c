// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct spinlock bcachelock;
struct buf buf[NBUF];
struct {
  struct spinlock lock;
  struct buf head;
} bcache[NBUCKET];

//struct buf *LRU_buffer;
//int min_bucketno;

void
binit(void)
{
  initlock(&bcachelock, "bcache");
  for(int i = 0; i < NBUCKET; i++) {
    initlock(&bcache[i].lock, "bcache");
    bcache[i].head.prev = &bcache[i].head;
    bcache[i].head.next = &bcache[i].head;
  }

  struct buf *b;
  for(b = buf; b < buf + NBUF; b++) {
    b->next = bcache[0].head.next;
    b->prev = &bcache[0].head;
    bcache[0].head.next->prev = b;
    bcache[0].head.next = b;
    
    initsleeplock(&b->lock, "buffer");
  }
}

static struct buf*
bget(uint dev, uint blockno)
{
  int bucketno = blockno % NBUCKET;  
  // Is the block already cached?

  acquire(&bcache[bucketno].lock);
  for(struct buf *b = bcache[bucketno].head.next; b != &bcache[bucketno].head; b = b->next) {
    if(b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      b->ticks = ticks;
      release(&bcache[bucketno].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache[bucketno].lock);

  struct buf *LRU_buffer = 0;
  int min_bucketno = 0;

  // Update LRU_buffer.
  acquire(&bcachelock);
  uint64 m_ticks = ticks + 1;
  for(int i = 0; i < NBUCKET; i++) {
    acquire(&bcache[i].lock);
    for(struct buf *b = bcache[i].head.next; b != &bcache[i].head; b = b->next) {
      if(b->ticks < m_ticks && b->refcnt == 0) {
	m_ticks = b->ticks;
	min_bucketno = i;
	LRU_buffer = b;
      }
    }
    release(&bcache[i].lock);
  }

  // Not cached.
  // move the LRU buffer to our bucket.
  // warning: they may both from the same bucket, be careful with DEADLOCK!
  acquire(&bcache[min_bucketno].lock);
  LRU_buffer->prev->next = LRU_buffer->next;
  LRU_buffer->next->prev = LRU_buffer->prev;
  release(&bcache[min_bucketno].lock);

  acquire(&bcache[bucketno].lock);
  LRU_buffer->next = bcache[bucketno].head.next;
  LRU_buffer->prev = &bcache[bucketno].head;
  bcache[bucketno].head.next->prev = LRU_buffer;
  bcache[bucketno].head.next = LRU_buffer;

  if(LRU_buffer->refcnt == 1)
    panic("WRONG REFCNT.");

  LRU_buffer->dev = dev;
  LRU_buffer->blockno = blockno;
  LRU_buffer->valid = 0;
  //LRU_buffer->refcnt = 1;
  LRU_buffer->refcnt++;
  LRU_buffer->ticks = ticks;
  release(&bcache[bucketno].lock);
  release(&bcachelock);
  acquiresleep(&LRU_buffer->lock);
  return LRU_buffer;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;
  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock)) {
    panic("brelse");
  }
  releasesleep(&b->lock);

  int bucketno = b->blockno % NBUCKET;
  acquire(&bcache[bucketno].lock);
  b->refcnt--;
  release(&bcache[bucketno].lock);
}

void
bpin(struct buf *b) {
  int bucketno = b->blockno % NBUCKET;
  acquire(&bcache[bucketno].lock);
  b->refcnt++;
  release(&bcache[bucketno].lock);
}

void
bunpin(struct buf *b) {
  int bucketno = b->blockno % NBUCKET;
  acquire(&bcache[bucketno].lock);
  b->refcnt--;
  release(&bcache[bucketno].lock);
}
