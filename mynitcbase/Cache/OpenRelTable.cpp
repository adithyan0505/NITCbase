#include "OpenRelTable.h"
#include <cstdlib>
#include <cstring>

OpenRelTable::OpenRelTable(){
   // initialize relCache and attrCache with nullptr
   for(int i=0;i<MAX_OPEN;i++){
      RelCacheTable::relCache[i] = nullptr;
      AttrCacheTable::attrCache[i] = nullptr;
   }

   /************ Setting up Relation Cache entries ************/
   // populating relation cache with entries from relation catalog and attribute catalog

   /**** setting up Relation Catalog relation in the Relation Cache Table****/
   RecBuffer relCatBlock(RELCAT_BLOCK);

   Attribute relCatRecord[RELCAT_NO_ATTRS];
   relCatBlock.getRecord(relCatRecord,RELCAT_SLOTNUM_FOR_RELCAT);

   RelCacheEntry relCacheEntry;                                //try directly passing into RelCacheTable::relCache[RELCAT_RELID]
   RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
   relCacheEntry.recId.block = RELCAT_BLOCK;
   relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

   // allocate this on the heap because we want it to persist outside this function
   RelCacheTable::relCache[RELCAT_RELID] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
   *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

   /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
   relCatBlock.getRecord(relCatRecord,RELCAT_SLOTNUM_FOR_ATTRCAT);
   RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);
   relCacheEntry.recId.block = RELCAT_BLOCK;
   relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

   // allocate this on the heap because we want it to persist outside this function
   RelCacheTable::relCache[ATTRCAT_RELID] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
   *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;

   /************ Setting up Attribute cache entries ************/
   // populating attribute cache with entries from relation catalog and attribute catalog

   /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
   RecBuffer attrCatBlock(ATTRCAT_BLOCK);

   Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

   AttrCacheEntry* prevAttr = nullptr;
   for(int i=0;i<=5;i++){
      attrCatBlock.getRecord(attrCatRecord,i);
      AttrCacheEntry attrCacheEntry;
      AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry.attrCatEntry);
      attrCacheEntry.recId.block = ATTRCAT_BLOCK;
      attrCacheEntry.recId.slot = i;
      if(prevAttr != nullptr){
         prevAttr->next = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
         *(prevAttr->next) = attrCacheEntry;
         prevAttr = prevAttr->next;
      }else{
         AttrCacheTable::attrCache[RELCAT_RELID] = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
         *(AttrCacheTable::attrCache[RELCAT_RELID]) = attrCacheEntry;
         prevAttr = AttrCacheTable::attrCache[RELCAT_RELID];
      }
   }
   prevAttr->next = nullptr;

   /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
   prevAttr = nullptr;
   for(int i=6;i<=11;i++){
      attrCatBlock.getRecord(attrCatRecord,i);
      AttrCacheEntry attrCacheEntry;
      AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry.attrCatEntry);
      attrCacheEntry.recId.block = ATTRCAT_BLOCK;
      attrCacheEntry.recId.slot = i;
      if(prevAttr != nullptr){
         prevAttr->next = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
         *(prevAttr->next) = attrCacheEntry;
         prevAttr = prevAttr->next;
      }else{
         AttrCacheTable::attrCache[ATTRCAT_RELID] = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
         *(AttrCacheTable::attrCache[ATTRCAT_RELID]) = attrCacheEntry;
         prevAttr = AttrCacheTable::attrCache[ATTRCAT_RELID];
      }
   }
   prevAttr->next = nullptr;
}

OpenRelTable::~OpenRelTable(){
   free(RelCacheTable::relCache[RELCAT_RELID]);
   free(RelCacheTable::relCache[ATTRCAT_RELID]);

   AttrCacheEntry* nextAttr = AttrCacheTable::attrCache[RELCAT_RELID];
   while(nextAttr != nullptr){
      nextAttr = nextAttr->next;
      free(AttrCacheTable::attrCache[RELCAT_RELID]);
      AttrCacheTable::attrCache[RELCAT_RELID] = nextAttr;
   }

   nextAttr = AttrCacheTable::attrCache[ATTRCAT_RELID];
   while(nextAttr != nullptr){
      nextAttr = nextAttr->next;
      free(AttrCacheTable::attrCache[ATTRCAT_RELID]);
      AttrCacheTable::attrCache[ATTRCAT_RELID] = nextAttr;
   }
}