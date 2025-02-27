#include "BlockAccess.h"

#include <cstring>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    RecId prevRecId;
    prevRecId.block = -1;
    prevRecId.slot = -1; ////////////////
    int ret = RelCacheTable::getSearchIndex(relId, &prevRecId);
    if (ret != SUCCESS) {
        return prevRecId;
    }

    int block, slot;
    if (prevRecId.block == -1 && prevRecId.slot == -1) {
        // (no hits from previous search - search should start from the first record)
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        block = relCatEntry.firstBlk;
        slot = 0;
    } else {
        //(there is a hit from previous search - start search from next record
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    while (block != -1) {
        RecBuffer blockObj(block);

        HeadInfo blockHeader;
        blockObj.getHeader(&blockHeader);

        Attribute recBuffer[blockHeader.numAttrs];
        blockObj.getRecord(recBuffer, slot);

        unsigned char slotMap[blockHeader.numSlots];
        blockObj.getSlotMap(slotMap);

        if (slot >= blockHeader.numSlots) {
            block = blockHeader.rblock;
            slot = 0;
            continue;
        }

        if (slotMap[slot] == SLOT_UNOCCUPIED) {
            slot++;
            continue;
        }

        // compare record's attribute value to the the given attrVal as below:

        // i. get attribute offset for attrName attribute from attribute cache entry of the relation
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

        // ii. use the attribute offset to get the value of the attribute from current record
        int cmpVal = compareAttrs(recBuffer[attrCatEntry.offset], attrVal, attrCatEntry.attrType);

        if (
          (op == NE && cmpVal != 0) || // if op is "not equal to"
          (op == LT && cmpVal < 0) ||  // if op is "less than"
          (op == LE && cmpVal <= 0) || // if op is "less than or equal to"
          (op == EQ && cmpVal == 0) || // if op is "equal to"
          (op == GT && cmpVal > 0) ||  // if op is "greater than"
          (op == GE && cmpVal >= 0)    // if op is "greater than or equal to"
        ) {
            prevRecId.block = block;
            prevRecId.slot = slot;
            RelCacheTable::setSearchIndex(relId, &prevRecId);
            return RecId{block, slot};
        }
        slot++;
    }
    return RecId{-1, -1};
}

// Stage-6 :

int BlockAccess::renameRelation(char *oldName, char *newName) {
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute newRelName;
    strcpy(newRelName.sVal, newName);
    char relName_attr[ATTR_SIZE];
    strcpy(relName_attr, RELCAT_ATTR_RELNAME);
    RecId searchRes = BlockAccess::linearSearch(RELCAT_RELID, relName_attr, newRelName, EQ);
    if (searchRes.block != -1 && searchRes.slot != -1) {
        return E_RELEXIST; // relation with relname newName exists already
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute oldRelName;
    strcpy(oldRelName.sVal, oldName);
    searchRes = BlockAccess::linearSearch(RELCAT_RELID, relName_attr, oldRelName, EQ);
    if (searchRes.block == -1 && searchRes.slot == -1) {
        return E_RELNOTEXIST; // relation with relname oldName does not exist
    }

    // update relName in relation catalog
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute record[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(record, searchRes.slot);
    strcpy(record[RELCAT_REL_NAME_INDEX].sVal, newName);
    relCatBlock.setRecord(record, searchRes.slot);

    // update relName in respective entries of attribute catalog from oldName to newName
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    strcpy(relName_attr, ATTRCAT_ATTR_RELNAME);
    int numAttrs = record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    for (int i = 0; i < numAttrs; i++) {
        searchRes = BlockAccess::linearSearch(ATTRCAT_RELID, relName_attr, oldRelName, EQ);
        RecBuffer attrCatBlock(searchRes.block);
        attrCatBlock.getRecord(record, searchRes.slot);
        strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal, newName);
        attrCatBlock.setRecord(record, searchRes.slot);
    }

    return SUCCESS;
}

int BlockAccess::renameAttribute(char *relName, char *oldName, char *newName) {
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relationName;
    strcpy(relationName.sVal, relName);
    char relName_attr[ATTR_SIZE];
    strcpy(relName_attr, RELCAT_ATTR_RELNAME);
    RecId searchRes = BlockAccess::linearSearch(RELCAT_RELID, relName_attr, relationName, EQ);
    if (searchRes.block == -1 && searchRes.slot == -1) {
        return E_RELNOTEXIST;
    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrToRenameRecId{-1, -1}; // stores the attr-cat recId of the attribute to rename
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    while (true) {
        searchRes = linearSearch(ATTRCAT_RELID, relName_attr, relationName, EQ);
        if (searchRes.block == -1 && searchRes.slot == -1) {
            break; // No more records to search
        }
        RecBuffer attrCatBlock(searchRes.block);
        attrCatBlock.getRecord(attrCatEntryRecord, searchRes.slot);
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0) {
            attrToRenameRecId.block = searchRes.block;
            attrToRenameRecId.slot = searchRes.slot;
        }
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0) {
            return E_ATTREXIST;
        }
    }
    if (attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1) {
        return E_ATTRNOTEXIST;
    }

    RecBuffer attrCatBlock(attrToRenameRecId.block);
    attrCatBlock.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    attrCatBlock.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    return SUCCESS;
}

// Stage-7 :

int BlockAccess::insert(int relId, union Attribute *record) {
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(relId, &relCatBuf);

    int blockNum = relCatBuf.firstBlk;

    RecId rec_id = RecId{-1, -1};

    int numOfSlots = relCatBuf.numSlotsPerBlk;
    int numofAttributes = relCatBuf.numAttrs;

    int prevBlockNum = -1;
    while (blockNum != -1) {
        RecBuffer block(blockNum);
        HeadInfo head;
        block.getHeader(&head);
        unsigned char slotMap[head.numSlots];
        block.getSlotMap(slotMap);

        for (int i = 0; i < head.numSlots; i++) {
            if (slotMap[i] == SLOT_UNOCCUPIED) {
                rec_id.block = blockNum;
                rec_id.slot = i;
                break;
            }
        }
        prevBlockNum = blockNum;
        blockNum = head.rblock;
    }
    // if no free slots found in existing record blocks
    if (rec_id.block == -1 && rec_id.slot == -1) {
        // if relation is RELCAT - no block allocation
        if (strcmp(relCatBuf.relName, RELCAT_RELNAME) == 0) {
            return E_MAXRELATIONS;
        }

        // allocating new record block using RecBuffer constructor - 1
        RecBuffer newBlock;
        int ret = newBlock.getBlockNum();
        if (ret == E_DISKFULL) {
            return E_DISKFULL;
        }

        rec_id.block = ret;
        rec_id.slot = 0;

        // set header of new block (partially done in constructor-1)
        HeadInfo newHead;
        newBlock.getHeader(&newHead);
        newHead.lblock = prevBlockNum;
        newHead.numSlots = numOfSlots;
        newHead.numAttrs = numofAttributes;
        newBlock.setHeader(&newHead);

        // set block's slotmap will all slots marked unoccupied
        unsigned char newSlotMap[numOfSlots];
        for (int i = 0; i < numOfSlots; i++) {
            newSlotMap[i] = SLOT_UNOCCUPIED;
        }
        newBlock.setSlotMap(newSlotMap);

        if (prevBlockNum != -1) {
            RecBuffer prevBlock(prevBlockNum);
            HeadInfo prevHead;
            prevBlock.getHeader(&prevHead);
            prevHead.rblock = rec_id.block;
            prevBlock.setHeader(&prevHead);
        } else {
            relCatBuf.firstBlk = rec_id.block;
        }
        relCatBuf.lastBlk = rec_id.block;
        RelCacheTable::setRelCatEntry(relId, &relCatBuf);
    }

    RecBuffer block(rec_id.block);
    block.setRecord(record, rec_id.slot);

    unsigned char slotMap[numOfSlots];
    block.getSlotMap(slotMap);
    slotMap[rec_id.slot] = SLOT_OCCUPIED;
    block.setSlotMap(slotMap);

    HeadInfo head;
    block.getHeader(&head);
    head.numEntries += 1;
    block.setHeader(&head);

    relCatBuf.numRecs += 1;
    RelCacheTable::setRelCatEntry(relId, &relCatBuf);
    // relation cache write-back to relation catalog happens in closeRel()

    return SUCCESS;
}

// Stage-8 :

// this function will copy the result of the search to the `record` argument
int BlockAccess::search(int relId, Attribute *record, char *attrName, Attribute attrVal, int op) {
    RecId recId;
    RelCacheTable::resetSearchIndex(relId);
    recId = BlockAccess::linearSearch(relId, attrName, attrVal, op);

    if (recId.block == -1 && recId.slot == -1) {
        return E_NOTFOUND;
    }

    RecBuffer block(recId.block);
    block.getRecord(record, recId.slot);

    return SUCCESS;
}

int BlockAccess::deleteRelation(char *relName) {
    if (!strcmp(RELCAT_RELNAME, relName) || !strcmp(ATTRCAT_RELNAME, relName)) {
        return E_NOTPERMITTED;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr; // stores relName as type union Attribute
    strcpy(relNameAttr.sVal, relName);
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, EQ);
    if (recId.block == -1 && recId.slot == -1) {
        return E_RELNOTEXIST;
    }
    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    RecBuffer relCatBlock(recId.block);
    relCatBlock.getRecord(relCatEntryRecord, recId.slot);

    int blockNo = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numOfAttrs = relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    // Delete all the record blocks of the relation
    while (blockNo != -1) {
        BlockBuffer block(blockNo);
        HeadInfo head;
        block.getHeader(&head);
        blockNo = head.rblock;
        block.releaseBlock();
    }

    // Delete attribute catalog entries corresponding the relation
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    int numberOfAttributesDeleted = 0;
    while (true) {
        RecId attrCatRecId;
        attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);
        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1) {
            break;
        }
        numberOfAttributesDeleted++;

        RecBuffer attrCatBlock(attrCatRecId.block);
        HeadInfo head;
        attrCatBlock.getHeader(&head);
        Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatEntryRecord, attrCatRecId.slot);

        int rootBlock = attrCatEntryRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
        // This will be used later to delete any existing indexes

        unsigned char slotMap[head.numSlots];
        attrCatBlock.getSlotMap(slotMap);
        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
        attrCatBlock.setSlotMap(slotMap);

        head.numEntries--;
        attrCatBlock.setHeader(&head);

        if (head.numEntries == 0) {
            RecBuffer lblock(head.lblock);
            HeadInfo lhead;
            lblock.getHeader(&lhead);
            lhead.rblock = head.rblock;
            lblock.setHeader(&lhead);

            if (head.rblock != -1) {
                RecBuffer rblock(head.rblock);
                HeadInfo rhead;
                rblock.getHeader(&rhead);
                rhead.lblock = head.lblock;
                rblock.setHeader(&rhead);
            } else {
                // block being released is the last block
                // update relation catalog entry of attribute catalog relation
                RelCatEntry relCatBuf;
                RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatBuf);
                relCatBuf.lastBlk = head.lblock;
                RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatBuf);
            }
            // attribute catalog will never be empty
            // no need to handle case of linked list becoming empty
            attrCatBlock.releaseBlock();
        }
    }

    // Delete the entry corresponding to the relation from relation catalog
    HeadInfo head;
    relCatBlock.getHeader(&head);
    head.numEntries--;
    relCatBlock.setHeader(&head);

    unsigned char slotMap[head.numSlots];
    relCatBlock.getSlotMap(slotMap);
    slotMap[recId.slot] = SLOT_UNOCCUPIED;
    relCatBlock.setSlotMap(slotMap);

    // Updating the Relation Cache Table - decrement no. of records
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatBuf);
    relCatBuf.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatBuf);

    // Update attribute catalog entry - reduce no. of records
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatBuf);
    relCatBuf.numRecs -= numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatBuf);

    return SUCCESS;
}
