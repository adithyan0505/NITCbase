#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

// Stage-3 : Setting up Disk Buffer and Caches
int main(int argc, char *argv[]){
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  for(int i=0;i<=2;i++) //(i.e RELCAT_RELID and ATTRCAT_RELID) + (Student RELID - Exer. Q1)
  {
    RelCatEntry relCatBuf;
    int ret = RelCacheTable::getRelCatEntry(i,&relCatBuf);
    if(ret != SUCCESS){
      return ret;
    }
    printf("Relation: %s\n", relCatBuf.relName);

    for(int j = 0;j<relCatBuf.numAttrs;j++){
      AttrCatEntry attrCatBuf;
      int ret = AttrCacheTable::getAttrCatEntry(i,j,&attrCatBuf);
      if(ret != SUCCESS){
        return ret;
      }
      const char *attrType = attrCatBuf.attrType == NUMBER ? "NUM" : "STR";
      printf("  %s: %s\n", attrCatBuf.attrName, attrType);
    }
    printf("\n");
  }
  return 0;
}

// // Stage-2 : Exercise Q2
// void updateAttr(const char* relName,const char* attrName,const char* newAttrName){
//   int blockNum = ATTRCAT_BLOCK;
//   while(true){
//     RecBuffer attrCatBuffer(blockNum);
//     HeadInfo attrCatHeader;
//     attrCatBuffer.getHeader(&attrCatHeader);
//     for(int i=0;i<attrCatHeader.numEntries;i++){
//       Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
//       attrCatBuffer.getRecord(attrCatRecord,i);
//       if(!strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relName) && !strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,attrName)){
//         unsigned char buffer[BLOCK_SIZE];
//         Disk::readBlock(buffer,blockNum);
//         int recordSize = ATTRCAT_NO_ATTRS * ATTR_SIZE;
//         int offset = HEADER_SIZE + SLOTMAP_SIZE_RELCAT_ATTRCAT + recordSize*i + 16;
//         memcpy(buffer+offset,newAttrName,16);
//         Disk::writeBlock(buffer,blockNum);
//         break;
//       }
//     }
//     if(attrCatHeader.rblock != -1)
//       blockNum = attrCatHeader.rblock;
//     else  break;
//   }
// }

// int main(int argc, char *argv[]) {
//   Disk disk_run;
//   StaticBuffer buffer;

//   //updateAttr("Students","Class","Batch"); // Exer. Q2 caller

//   // Stage-2 + Exer. Q1 :-
//   // create object for the relation catalog
//   RecBuffer relCatBuffer(RELCAT_BLOCK);

//   HeadInfo relCatHeader;

//   // load the headers of both the blocks into relCatHeader and attrCatHeader.
//   // (we will implement these functions later)
//   relCatBuffer.getHeader(&relCatHeader);

//   for (int i=0;i<relCatHeader.numEntries;i++) {

//     Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

//     relCatBuffer.getRecord(relCatRecord, i);

//     printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

//     // create object for the attribute catalog & initialise data structure to store header of attribute catalog block
//     RecBuffer attrCatBuffer(ATTRCAT_BLOCK);
//     HeadInfo attrCatHeader;
//     attrCatBuffer.getHeader(&attrCatHeader);

//     bool lastFlag = false;
//     // loop through all blocks of attribute catalog
//     while(!lastFlag){
//       for (int j=0;j<attrCatHeader.numEntries;j++) {

//         // declare attrCatRecord and load the attribute catalog entry into it
//         Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
//         attrCatBuffer.getRecord(attrCatRecord,j);

//         if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal)==0) {
//           const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
//           printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
//         }
//       }
//       if(attrCatHeader.rblock != -1){
//         //the rblock field in the header of the attribute catalog block gives the next block
//         attrCatBuffer = attrCatHeader.rblock; // equivalent to setting blockNum=header.rblock & calling method RecBuffer attrCatBuffer(blockNum); ?
//         attrCatBuffer.getHeader(&attrCatHeader);
//       }else lastFlag = true; // break loop if last block of attribute catalog
//     }
//     printf("\n");
//   }

//   return 0;
// }

/* Stage-1 Exercise Q1 :
int main(int argc, char *argv[]) {
  // Initialize the Run Copy of Disk
  Disk disk_run;
  
  for(int i=0;i<=3;i++){
    unsigned char buffer[BLOCK_SIZE];
    Disk::readBlock(buffer,i);
    for(int x: buffer)
      printf("%d ",x);
    std::cout << std::endl;
  }
  return 0;
}*/

/* Stage-1 :
int main(int argc, char *argv[]) {
  // Initialize the Run Copy of Disk
  Disk disk_run;

  unsigned char buffer[BLOCK_SIZE];
  Disk::readBlock(buffer,7000);
  char message[] = "hello";
  memcpy(buffer+20,message,6); //
  Disk::writeBlock(buffer,7000);

  unsigned char buffer2[BLOCK_SIZE];
  char message2[6];
  Disk::readBlock(buffer2,7000);
  memcpy(message2,buffer2+20,6);
  std::cout << message2 << std::endl;

  return 0;

} */