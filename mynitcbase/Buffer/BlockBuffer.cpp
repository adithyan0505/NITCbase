#include "BlockBuffer.h"
#include "StaticBuffer.h"

#include <cstdlib>
#include <cstring>

// Stage-2 :
BlockBuffer::BlockBuffer(int blockNum) {
    // initialise this.blockNum with the argument
    this->blockNum = blockNum;
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

// load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {

    // read the block from the buffer instead of the disk directly (stage-3)
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
        return ret;
    }

    // populate the numEntries, numAttrs and numSlots fields in *head
    memcpy(&head->numSlots, bufferPtr + 24, 4);
    memcpy(&head->numEntries, bufferPtr + 16, 4);
    memcpy(&head->numAttrs, bufferPtr + 20, 4);
    memcpy(&head->rblock, bufferPtr + 12, 4);
    memcpy(&head->lblock, bufferPtr + 8, 4);
    memcpy(&head->blockType,bufferPtr, 4);
    memcpy(&head->pblock, bufferPtr+4, 4);

    return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
    struct HeadInfo head;

    // get the header using this.getHeader() function
    this->getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    // read the block at this.blockNum from the buffer instead of the disk directly (stage-3)
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
        return ret;
    }

    /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
       - each record will have size attrCount * ATTR_SIZE
       - slotMap will be of size slotCount
    */
    int recordSize = attrCount * ATTR_SIZE;
    unsigned char *slotPointer = bufferPtr + HEADER_SIZE + slotCount + recordSize * slotNum;

    // load the record into the rec data structure
    memcpy(rec, slotPointer, recordSize);

    return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
    // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if (bufferNum == E_OUTOFBOUND) {
        return E_OUTOFBOUND;
    }

    if (bufferNum == E_BLOCKNOTINBUFFER) {
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
        // reading block at this.blockNum from disk and storing into buffer location pointer by bufferNum
        Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
    } else {
        for (int i = 0; i < BUFFER_CAPACITY; i++) {
            if (!StaticBuffer::metainfo[i].free) {
                StaticBuffer::metainfo[i].timeStamp += 1;
            }
        }
        StaticBuffer::metainfo[bufferNum].timeStamp = 0;
    }
    *buffPtr = StaticBuffer::blocks[bufferNum]; // points to req. block stored in buffer

    return SUCCESS;
}

// Stage-4 :
int RecBuffer::getSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
        return ret;
    }
    struct HeadInfo head;
    this->getHeader(&head);

    int slotCount = head.numSlots;
    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

    for (int i = 0; i < slotCount; i++) {
        slotMap[i] = slotMapInBuffer[i];
    }

    return SUCCESS;
}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {
    double diff;
    if (attrType == STRING) {
        diff = strcmp(attr1.sVal, attr2.sVal);
    } else {
        diff = attr1.nVal - attr2.nVal;
    }

    if (diff > 0)
        return 1;
    if (diff < 0)
        return -1;
    return 0; // diff==0
}

// Stage-6 :

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if (ret != SUCCESS) {
        return ret;
    }

    HeadInfo head;
    this->getHeader(&head);

    int numAttrs = head.numAttrs;
    int numSlots = head.numSlots;

    if (slotNum >= numSlots) {
        return E_OUTOFBOUND;
    }

    int recordSize = ATTR_SIZE * numAttrs;
    bufferPtr += HEADER_SIZE + numSlots + slotNum * recordSize;
    memcpy(bufferPtr, rec, recordSize);

    StaticBuffer::setDirtyBit(this->blockNum);

    return SUCCESS;
}

// Stage-7 :

int BlockBuffer::setHeader(struct HeadInfo *head) {
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
        return ret;
    }

    // cast bufferPtr to type HeadInfo*
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

    // copy the fields of the HeadInfo pointed to by head (except reserved) to
    // the header of the block (pointed to by bufferHeader)
    bufferHeader[0].blockType = head->blockType;
    bufferHeader[0].pblock = head->pblock;
    bufferHeader[0].lblock = head->lblock;
    bufferHeader[0].rblock = head->rblock;
    bufferHeader[0].numAttrs = head->numAttrs;
    bufferHeader[0].numEntries = head->numEntries;
    bufferHeader[0].numSlots = head->numSlots;

    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS) {
        return ret;
    }

    return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType) {
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
        return ret;
    }
    ((int32_t *)bufferPtr)[0] = blockType;

    StaticBuffer::blockAllocMap[this->blockNum] = blockType;

    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS) {
        return ret;
    }

    return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType) {
    // iterate through BMAP to find a free block
    int blockNum = -1;
    for (int i = 4; i < DISK_BLOCKS; i++) {
        if (StaticBuffer::blockAllocMap[i] == UNUSED_BLK) {
            blockNum = i;
            break;
        }
    }
    if (blockNum == -1) {
        return E_DISKFULL;
    }
    // set the object's blockNum to the block number of the free block
    this->blockNum = blockNum;

    // initialise the header and setting the header
    struct HeadInfo head;
    head.pblock = -1, head.lblock = -1, head.rblock = -1, head.numSlots = 0, head.numEntries = 0, head.numAttrs = 0;
    BlockBuffer::setHeader(&head);

    // update block type of the block to input blockType
    BlockBuffer::setBlockType(blockType);

    return blockNum;
}

// BlockBuffer class - constructor 1
// (called if a new block of the input type is to be allocated in the disk)
// (If the block has already been initialised as a record or index block, use constructor 2)

BlockBuffer::BlockBuffer(char blockType) {
    int block_type;
    if (blockType == 'R')
        block_type = REC;
    else if (blockType == 'I')
        block_type = IND_INTERNAL;
    else
        block_type = IND_LEAF;
    int blockNum = BlockBuffer::getFreeBlock(block_type);
    // blockNum will contain either a valid block number or error code
    // returned by getFreeBlock()

    this->blockNum = blockNum;
    // (The caller must check if the constructor allocatted block successfully
    // by checking the value of block number field.)
}

// RecBuffer class - constructor 1
// (called if a new record block is to be allocated in the disk)
// (if the block has already been initialised as a record block on the disk, use constructor 2)

RecBuffer::RecBuffer() : BlockBuffer('R') {}
// call parent non-default constructor with 'R' denoting record block.

int RecBuffer::setSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
        return ret;
    }

    HeadInfo head;
    this->getHeader(&head);
    int numSlots = head.numSlots;

    // copy contents of argument 'slotMap' into buffer replacing existing slotmap
    memcpy(bufferPtr + HEADER_SIZE, slotMap, numSlots);

    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS) {
        return ret;
    }

    return SUCCESS;
}

int BlockBuffer::getBlockNum() {
    // Defined to access the protected member field blockNum of the class
    return this->blockNum;
}

// Stage-8 :

void BlockBuffer::releaseBlock() {
    if (this->blockNum == INVALID_BLOCKNUM) {
        return;
    }
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
    // free buffer corresponding to the blockNum
    if (bufferNum != E_BLOCKNOTINBUFFER) {
        StaticBuffer::metainfo[bufferNum].free = true;
    }
    // free blockNum in disk
    StaticBuffer::blockAllocMap[this->blockNum] = UNUSED_BLK;
    // invalidating object's blockNum field
    this->blockNum = INVALID_BLOCKNUM;
}
