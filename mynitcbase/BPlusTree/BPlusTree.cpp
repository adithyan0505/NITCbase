#include "BPlusTree.h"

#include <cstring>

RecId BPlusTree::bPlusSearch(int relId, char *attrName, union Attribute attrVal, int op) {
    IndexId searchIndex;
    AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);

    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    int block, index;

    if (searchIndex.block == -1 && searchIndex.index == -1) {
        // (search is done for the first time)
        block = attrCatEntry.rootBlock;
        index = 0;

        if (block == -1) {
            // attrName doesn't have a B+ tree
            return RecId{-1, -1};
        }
    } else {
        /*a valid searchIndex points to an entry in the leaf index of the attribute's
        B+ Tree which had previously satisfied the op for the given attrVal.*/
        block = searchIndex.block;
        index = searchIndex.index + 1;

        IndLeaf leaf(block);

        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        if (index >= leafHead.numEntries) {
            // all the entries in the block has been searched
            // search from the beginning of the next leaf index block
            block = leafHead.rblock;
            index = 0;

            if (block == -1) {
                // end of linked list - search over
                return RecId{-1, -1};
            }
        }   
    }
    // Traverse through all the internal nodes
    // according to value of attrVal and the operator op

    /* (This section is only needed when
        - search restarts from the root block (when searchIndex is reset by caller)
        - root is not a leaf
    If there was a valid search index, then we are already at a leaf block
    and the test condition in the following loop will fail)*/

    while (StaticBuffer::getStaticBlockType(block) == IND_INTERNAL) {
        // load the block into internalBlk using IndInternal::IndInternal()
        IndInternal internalBlk(block);

        HeadInfo intHead;
        internalBlk.getHeader(&intHead);

        InternalEntry intEntry;

        if (op == NE || op == LT || op == LE) {
            // move to left-most leaf node
            internalBlk.getEntry(&intEntry, 0);
            block = intEntry.lChild;
        } else {
            // EQ, GT, GE
            while (index < intHead.numEntries) {
                internalBlk.getEntry(&intEntry, index);
                if (op == EQ || op == GE) {
                    if (compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType) >= 0) {
                        break;
                    }
                } else {
                    // GT
                    if (compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType) > 0) {
                        break;
                    }
                }
                index++;
            }
            if (index < intHead.numEntries) {
                // move to the left child of that entry
                block = intEntry.lChild;
                index = 0;
            } else {
                // move to the right child of the last entry of the block
                block = intEntry.rChild;
                index = 0;
            }
        }
    }

    // NOTE: `block` now has the block number of a leaf index block

    while (block != -1) {
        // load the block into leafBlk
        IndLeaf leafBlk(block);
        HeadInfo leafHead;

        leafBlk.getHeader(&leafHead);

        Index leafEntry;

        while (index < leafHead.numEntries) {
            leafBlk.getEntry(&leafEntry, index);

            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, attrCatEntry.attrType);

            if (
                (op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0)) {
                // entry satisfying the condition found
                searchIndex.block = block;
                searchIndex.index = index;
                AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);

                return RecId{leafEntry.block, leafEntry.slot};
            } else if ((op == EQ || op == LE) && cmpVal > 0) {
                // future entries will NOT satisfy EQ, LE
                return RecId{-1, -1};
            } else if (op == LT && cmpVal >= 0) {
                // future entries will NOT satisfy LT
                return RecId{-1, -1};
            }
            index++;
        }
        /*only for NE operation do we have to check the entire linked list;
        for all the other op it is guaranteed that the block being searched
        will have an entry, if it exists, satisying that op. */
        if (op != NE) {
            break;
        }

        block = leafHead.rblock;
        index = 0;
    }
    return RecId{-1, -1};
}
