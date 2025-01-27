#include "OpenRelTable.h"
#include "AttrCacheTable.h"
#include "RelCacheTable.h"
#include <cstdlib>
#include <cstring>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable() {
    // initialize relCache and attrCache with nullptr
    for (int i = 0; i < MAX_OPEN; i++) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
        OpenRelTable::tableMetaInfo[i].free = true;
    }

    /************ Setting up Relation Cache entries ************/
    // populating relation cache with entries from relation catalog and attribute
    // catalog

    /**** setting up Relation Catalog relation in the Relation Cache Table****/
    RecBuffer relCatBlock(RELCAT_BLOCK);

    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

    RelCacheEntry relCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

    // allocate this on the heap because we want it to persist outside this
    // function
    RelCacheTable::relCache[RELCAT_RELID] =
      (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

    /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

    // allocate this on the heap because we want it to persist outside this
    // function
    RelCacheTable::relCache[ATTRCAT_RELID] =
      (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;

    /************ Setting up Attribute cache entries ************/
    // populating attribute cache with entries from relation catalog and attribute
    // catalog

    /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    AttrCacheEntry *prevAttr = nullptr;
    for (int i = 0; i <= 5; i++) {
        attrCatBlock.getRecord(attrCatRecord, i);
        AttrCacheEntry attrCacheEntry;
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,
                                             &attrCacheEntry.attrCatEntry);
        attrCacheEntry.recId.block = ATTRCAT_BLOCK;
        attrCacheEntry.recId.slot = i;
        if (prevAttr != nullptr) {
            prevAttr->next = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
            prevAttr = prevAttr->next;
        } else {
            AttrCacheTable::attrCache[RELCAT_RELID] =
              (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
            prevAttr = AttrCacheTable::attrCache[RELCAT_RELID];
        }
        *prevAttr = attrCacheEntry;
    }
    prevAttr->next = nullptr;

    /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
    prevAttr = nullptr;
    for (int i = 6; i <= 11; i++) {
        attrCatBlock.getRecord(attrCatRecord, i);
        AttrCacheEntry attrCacheEntry;
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,
                                             &attrCacheEntry.attrCatEntry);
        attrCacheEntry.recId.block = ATTRCAT_BLOCK;
        attrCacheEntry.recId.slot = i;
        if (prevAttr != nullptr) {
            prevAttr->next = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
            prevAttr = prevAttr->next;
        } else {
            AttrCacheTable::attrCache[ATTRCAT_RELID] =
              (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
            prevAttr = AttrCacheTable::attrCache[ATTRCAT_RELID];
        }
        *prevAttr = attrCacheEntry;
    }
    prevAttr->next = nullptr;

    tableMetaInfo[RELCAT_RELID].free = false;
    tableMetaInfo[ATTRCAT_RELID].free = false;
    strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);
    strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);
}

OpenRelTable::~OpenRelTable() {

    // close all open relations (rel-id = 2 to MAX_OPEN)
    for (int i = 2; i < MAX_OPEN; i++) {
        if (tableMetaInfo[i].free == false) {
            OpenRelTable::closeRel(i);
        }
    }

    // free memory allocated to rel-id = 0 and rel-id = 1 in the caches
    free(RelCacheTable::relCache[RELCAT_RELID]);
    free(RelCacheTable::relCache[ATTRCAT_RELID]);
    RelCacheTable::relCache[RELCAT_RELID] = nullptr;
    RelCacheTable::relCache[ATTRCAT_RELID] = nullptr;

    AttrCacheEntry *nextAttr = AttrCacheTable::attrCache[RELCAT_RELID];
    while (nextAttr != nullptr) {
        nextAttr = nextAttr->next;
        free(AttrCacheTable::attrCache[RELCAT_RELID]);
        AttrCacheTable::attrCache[RELCAT_RELID] = nextAttr;
    }

    nextAttr = AttrCacheTable::attrCache[ATTRCAT_RELID];
    while (nextAttr != nullptr) {
        nextAttr = nextAttr->next;
        free(AttrCacheTable::attrCache[ATTRCAT_RELID]);
        AttrCacheTable::attrCache[ATTRCAT_RELID] = nextAttr;
    }
}

int OpenRelTable::getFreeOpenRelTableEntry() {
    for (int i = 2; i < MAX_OPEN; i++) {
        if (tableMetaInfo[i].free) {
            return i;
        }
    }
    return E_CACHEFULL;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
    for (int i = 0; i < MAX_OPEN; i++) {
        if (strcmp(tableMetaInfo[i].relName, relName) == 0) {
            return i;
        }
    }
    return E_RELNOTOPEN;
}

int OpenRelTable::openRel(char *relName) {
    int relId;
    if ((relId = getRelId(relName)) != E_RELNOTOPEN) {
        return relId;
    }
    if ((relId = getFreeOpenRelTableEntry()) == E_CACHEFULL) {
        return E_CACHEFULL;
    }
    // now relId stores index of a free slot

    /****** Setting up Relation Cache entry for the relation ******/ // Linear search in relation catalog block
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relationName;
    strcpy(relationName.sVal, relName);
    char relname_attr[ATTR_SIZE];
    strcpy(relname_attr, RELCAT_ATTR_RELNAME);
    RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, relname_attr, relationName, EQ);
    if (relcatRecId.block == -1 && relcatRecId.slot == -1) {
        return E_RELNOTEXIST;
    }

    RecBuffer relCatBlock(relcatRecId.block);
    Attribute record[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(record, relcatRecId.slot);
    RelCacheEntry relCacheEntry;
    RelCacheTable::recordToRelCatEntry(record, &relCacheEntry.relCatEntry);
    relCacheEntry.recId = relcatRecId;
    // relCacheEntry.searchIndex = {-1,-1} ??
    RelCacheTable::relCache[relId] = (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[relId]) = relCacheEntry;

    /****** Setting up Attribute Cache entry for the relation ******/ // Linear search in attribute catalog block(s)
    AttrCacheEntry *prevAttr = nullptr;
    int numberofAttrs = RelCacheTable::relCache[relId]->relCatEntry.numAttrs;
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    for (int i = 0; i < numberofAttrs; i++) {
        RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, relname_attr, relationName, EQ);

        RecBuffer attrCatBlock(attrcatRecId.block);
        Attribute record[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(record, attrcatRecId.slot);
        AttrCacheEntry attrCacheEntry;
        AttrCacheTable::recordToAttrCatEntry(record, &attrCacheEntry.attrCatEntry);
        attrCacheEntry.recId.block = attrcatRecId.block;
        attrCacheEntry.recId.slot = attrcatRecId.slot;

        if (prevAttr == nullptr) {
            AttrCacheTable::attrCache[relId] = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
            prevAttr = AttrCacheTable::attrCache[relId];
        } else {
            prevAttr->next = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
            prevAttr = prevAttr->next;
        }
        *prevAttr = attrCacheEntry;
    }
    prevAttr->next = nullptr;

    // Updating tableMetaInfo values
    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName, relName);

    return relId;
}

int OpenRelTable::closeRel(int relId) {
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID) {
        return E_NOTPERMITTED;
    }
    if (relId < 0 && relId >= MAX_OPEN) {
        return E_OUTOFBOUND;
    }
    if (tableMetaInfo[relId].free) {
        return E_RELNOTOPEN;
    }

    free(RelCacheTable::relCache[relId]);
    RelCacheTable::relCache[relId] = nullptr;

    AttrCacheEntry *attrNode = AttrCacheTable::attrCache[relId];
    while (attrNode != nullptr) {
        attrNode = attrNode->next;
        free(AttrCacheTable::attrCache[relId]);
        AttrCacheTable::attrCache[relId] = attrNode;
    }

    tableMetaInfo[relId].free = true;

    return SUCCESS;
}
