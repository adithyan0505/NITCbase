#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

// Stage-2 + Exercise Q1
int main(int argc, char *argv[]) {
  Disk disk_run;

  // create object for the relation catalog
  RecBuffer relCatBuffer(RELCAT_BLOCK);

  HeadInfo relCatHeader;

  // load the headers of both the blocks into relCatHeader and attrCatHeader.
  // (we will implement these functions later)
  relCatBuffer.getHeader(&relCatHeader);

  for (int i=0;i<relCatHeader.numEntries;i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    // create object for the attribute catalog & initialise data structure to store header of attribute catalog block
    RecBuffer attrCatBuffer(ATTRCAT_BLOCK);
    HeadInfo attrCatHeader;
    attrCatBuffer.getHeader(&attrCatHeader);

    bool lastFlag = false;
    // loop through all blocks of attribute catalog
    while(!lastFlag){
      for (int j=0;j<attrCatHeader.numEntries;j++) {

        // declare attrCatRecord and load the attribute catalog entry into it
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord,j);

        if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal)==0) {
          const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
          printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
        }
      }
      if(attrCatHeader.rblock != -1){
        //the rblock field in the header of the attribute catalog block gives the next block
        attrCatBuffer = attrCatHeader.rblock; // equivalent to setting blockNum=header.rblock & calling method RecBuffer attrCatBuffer(blockNum); ?
        attrCatBuffer.getHeader(&attrCatHeader);
      }else lastFlag = true; // break loop if last block of attribute catalog
    }
    printf("\n");
  }

  return 0;
}

/* Stage-1 Exercise Q1 :
int main(int argc, char *argv[]) {
  // Initialize the Run Copy of Disk
  Disk disk_run;
  
  for(int i=0;i<3;i++){
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