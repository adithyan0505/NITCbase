#include "BlockAccess.h"

#include <cstring>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op){
    RecId prevRecId;
    prevRecId.block = -1; prevRecId.slot = -1; ////////////////
    int ret = RelCacheTable::getSearchIndex(relId, &prevRecId);
    if(ret != SUCCESS){
        return prevRecId;
    }

    int block, slot;
    if(prevRecId.block == -1 && prevRecId.slot == -1){
        // (no hits from previous search - search should start from the first record) 
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        block = relCatEntry.firstBlk;
        slot = 0;
    }
    else{
        //(there is a hit from previous search - start search from next record
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
   while(block != -1){
        RecBuffer blockObj(block); 
        
        HeadInfo blockHeader;
        blockObj.getHeader(&blockHeader); 

        Attribute recBuffer[blockHeader.numAttrs];
        blockObj.getRecord(recBuffer, slot);
        
        unsigned char slotMap[blockHeader.numSlots];
        blockObj.getSlotMap(slotMap);

        if(slot >= blockHeader.numSlots){
            block = blockHeader.rblock;
            slot = 0;
            continue;
        }

        if(slotMap[slot] == SLOT_UNOCCUPIED){
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
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ){
            prevRecId.block = block; prevRecId.slot = slot;
            RelCacheTable::setSearchIndex(relId, &prevRecId);
            return RecId{block, slot};
        }
        slot++;
   }
   return RecId{-1,-1};
}