#include "StaticBuffer.h"
#include <algorithm>
#include <climits>

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer() {
    // copying BMAP from disk to memory
    for (int i = 0; i <= 3; i++) {
        Disk::readBlock(&StaticBuffer::blockAllocMap[BLOCK_SIZE * i], i);
    }

    // initialise all metainfo of all buffer blocks
    for (int i = 0; i < BUFFER_CAPACITY; i++) {
        metainfo[i].free = true;
        metainfo[i].blockNum = -1;
        metainfo[i].dirty = false;
        metainfo[i].timeStamp = -1;
    }
}

StaticBuffer::~StaticBuffer() {
    // write back BMAP from memory to disk
    for (int i = 0; i <= 3; i++) {
        Disk::writeBlock(&StaticBuffer::blockAllocMap[BLOCK_SIZE * i], i);
    }

    // write back modified blocks from buffer to disk
    for (int i = 0; i < BUFFER_CAPACITY; i++) {
        if (!metainfo[i].free && metainfo[i].dirty) {
            Disk::writeBlock(blocks[i], metainfo[i].blockNum);
        }
    }
}

int StaticBuffer::getFreeBuffer(int blockNum) {
    if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }

    int maxTimeStamp = -1; // to store the highest Time Stamp
    int maxTimeIndex = -1; // to store Least-Recently-Used bufferNum (index)
    for (int i = 0; i < BUFFER_CAPACITY; i++) {
        if (!metainfo[i].free) {
            metainfo[i].timeStamp += 1;
            if (metainfo[i].timeStamp > maxTimeStamp) {
                maxTimeIndex = i;
                maxTimeStamp = metainfo[i].timeStamp;
            }
        }
    }

    int bufferNum = -1;

    for (int i = 0; i < BUFFER_CAPACITY; i++) {
        if (metainfo[i].free) {
            bufferNum = i;
            break;
        }
    }

    // if buffer is full - replacement according to LRU policy
    if (bufferNum == -1) {
        if (metainfo[maxTimeIndex].dirty) {
            Disk::writeBlock(blocks[maxTimeIndex], metainfo[maxTimeIndex].blockNum);
        }
        bufferNum = maxTimeIndex;
    }

    metainfo[bufferNum].free = false;
    metainfo[bufferNum].blockNum = blockNum;
    metainfo[bufferNum].dirty = false;
    metainfo[bufferNum].timeStamp = 0;

    return bufferNum;
}

int StaticBuffer::getBufferNum(int blockNum) {
    if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }

    for (int i = 0; i < BUFFER_CAPACITY; i++) {
        if (!metainfo[i].free && metainfo[i].blockNum == blockNum) {
            return i;
        }
    }

    return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum) {
    int bufferNum = StaticBuffer::getBufferNum(blockNum);
    if (bufferNum < 0) {
        return bufferNum; // Error codes are returned (E_BLOCKNOTINBUFFER / E_OUTOFBOUND)
    }
    metainfo[bufferNum].dirty = true; // bufferNum is valid hence, setting dirty bit
    return SUCCESS;
}
