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

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf head;
  struct buf bucket[NBUCKET];
  struct spinlock bucketlock[NBUCKET];
} bcache;

int hash(uint dev, uint blockno)
{
  return (dev+blockno) % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for (int i = 0; i < NBUCKET; ++i)
  {
    initlock(&bcache.bucketlock[i], "bcache.bucket");
    bcache.bucket[i].next = 0;
  }

  // Create linked list of buffers
  //bcache.head.prev = &bcache.head;
  //bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->refcnt = 0;
    b->next = bcache.bucket[0].next;
    bcache.bucket[0].next = b;
    b->ticks = 0;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b, *lru_b, *prev;
  int h, i;

  h = hash(dev, blockno);
  acquire(&bcache.bucketlock[h]);
  for (b = bcache.bucket[h].next; b; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.bucketlock[h]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucketlock[h]);

  lru_b = 0;
  prev = 0;
  int idx = -1, found = 0;
  for (i = 0; i < NBUCKET; i++) {
    acquire(&bcache.bucketlock[i]);
    found = 0;
    for (b = &bcache.bucket[i]; b->next; b = b->next) {
      if (b->next->refcnt == 0 && (!prev || b->next->ticks < prev->next->ticks)) {
        prev = b;
        found = 1;
      }
    }
    if (!found) {
      release(&bcache.bucketlock[i]);
    } else {
      if (idx != -1)
        release(&bcache.bucketlock[idx]);
      idx = i;
    }
  }

  lru_b = prev->next;
  prev->next = lru_b->next;
  release(&bcache.bucketlock[idx]);

  acquire(&bcache.bucketlock[h]);
  for (b = bcache.bucket[h].next; b; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.bucketlock[h]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  lru_b->next = bcache.bucket[h].next;
  bcache.bucket[h].next = lru_b;
  lru_b->dev = dev;
  lru_b->blockno = blockno;
  lru_b->valid = 0;
  lru_b->refcnt = 1;
  release(&bcache.bucketlock[h]);
  acquiresleep(&lru_b->lock);
  return lru_b;

  panic("bget: no buffers");
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

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  int h;

  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  h = hash(b->dev, b->blockno);
  acquire(&bcache.bucketlock[h]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->ticks = ticks;
  }
  release(&bcache.bucketlock[h]);
}

void
bpin(struct buf *b) {
  int h;
  h = hash(b->dev, b->blockno);
  acquire(&bcache.bucketlock[h]);
  b->refcnt++;
  release(&bcache.bucketlock[h]);
}

void
bunpin(struct buf *b) {
  int h;
  h = hash(b->dev, b->blockno);
  acquire(&bcache.bucketlock[h]);
  b->refcnt--;
  release(&bcache.bucketlock[h]);
}


