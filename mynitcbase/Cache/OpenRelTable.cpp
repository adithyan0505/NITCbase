#include "OpenRelTable.h"
#include <cstdlib>
#include <cstring>

// Stage-3 :
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

   RelCacheEntry relCacheEntry;
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
         prevAttr = prevAttr->next;
      }else{
         AttrCacheTable::attrCache[RELCAT_RELID] = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
         prevAttr = AttrCacheTable::attrCache[RELCAT_RELID];
      }
      *prevAttr = attrCacheEntry;
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
         prevAttr = prevAttr->next;
      }else{
         AttrCacheTable::attrCache[ATTRCAT_RELID] = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
         prevAttr = AttrCacheTable::attrCache[ATTRCAT_RELID];
      }
      *prevAttr = attrCacheEntry;
   }
   prevAttr->next = nullptr;

   // /*** (Stage-3 : Exer. Q1) Modify to cache and print relation catalog & attribute catalog entries for the Student relation ***/

   // // populating relation cache
   // relCatBlock.getRecord(relCatRecord,2); // relation catalog slotNum for Student relation: 2
   // RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);
   // relCacheEntry.recId.block = RELCAT_BLOCK;
   // relCacheEntry.recId.slot = 2;

   // RelCacheTable::relCache[2] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
   // *(RelCacheTable::relCache[2]) = relCacheEntry;

   // // populating attrtibute cache
   // prevAttr = nullptr;
   // for(int i = 12 ; i < 12+relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal; i++){
   //    attrCatBlock.getRecord(attrCatRecord,i);
   //    AttrCacheEntry attrCacheEntry;
   //    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry.attrCatEntry);
   //    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
   //    attrCacheEntry.recId.slot = i;
   //    if(prevAttr != nullptr){
   //       prevAttr->next = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
   //       prevAttr = prevAttr->next;
   //    }else{
   //       AttrCacheTable::attrCache[2] = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
   //       prevAttr = AttrCacheTable::attrCache[2];
   //    }
   //    *prevAttr = attrCacheEntry;
   // }
   // prevAttr->next = nullptr;
}

OpenRelTable::~OpenRelTable(){
   // free all the memory that you allocated in the constructor
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

   // // free memory allocated in cache for Student relation (Stage-3 : Exer. Q1)
   // free(RelCacheTable::relCache[2]);

   // nextAttr = AttrCacheTable::attrCache[2];
   // while(nextAttr != nullptr){
   //    nextAttr = nextAttr->next;
   //    free(AttrCacheTable::attrCache[2]);
   //    AttrCacheTable::attrCache[2] = nextAttr;
   // }
}

// Stage-4 :
/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getRelId(char relName[ATTR_SIZE]){
   if(strcmp(relName, RELCAT_RELNAME)==0) return RELCAT_RELID;
   if(strcmp(relName, ATTRCAT_RELNAME)==0) return ATTRCAT_RELID;

   return E_RELNOTOPEN;
}