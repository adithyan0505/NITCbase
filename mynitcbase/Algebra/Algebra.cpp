#include "Algebra.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

int isNumber(char *str) {
    int len;
    float ignore;

    int ret = sscanf(str, "%f %n", &ignore, &len);
    return ret == 1 && len == strlen(str);
}

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);
    if (ret != SUCCESS) {
        return ret;
    }

    /*** Convert strVal (string) to an attribute of data type NUMBER or STRING ***/
    int type = attrCatEntry.attrType;
    Attribute attrVal;
    if (type == NUMBER) {
        if (isNumber(strVal)) {
            attrVal.nVal = atof(strVal);
        } else {
            return E_ATTRTYPEMISMATCH;
        }
    } else if (type == STRING) {
        strcpy(attrVal.sVal, strVal);
    }

    /*** Creating and opening the target relation ***/
    // Prepare arguments for createRel() :
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    int src_nAttrs = relCatEntry.numAttrs;
    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_type[src_nAttrs];

    for (int i = 0; i < src_nAttrs; i++) {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
        strcpy(attr_names[i], attrCatEntry.attrName);
        attr_type[i] = attrCatEntry.attrType;
    }

    // Create the target relation using createRel()
    int retVal = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_type);
    if (retVal != SUCCESS) {
        return retVal;
    }

    // Open the target relation using openRel()
    retVal = Schema::openRel(targetRel);
    if (retVal != SUCCESS) {
        Schema::deleteRel(targetRel);
        return retVal;
    }
    int targetRelId = OpenRelTable::getRelId(targetRel);

    /*** Selecting and inserting records into the target relation ***/
    RelCacheTable::resetSearchIndex(srcRelId);
    AttrCacheTable::resetSearchIndex(srcRelId, attrCatEntry.offset);
    Attribute record[src_nAttrs];

    // Initialise variable to store number of comparisons
    StaticBuffer::compCount = 0;

    while (BlockAccess::search(srcRelId, record, attr, attrVal, op) == SUCCESS) {
        ret = BlockAccess::insert(targetRelId, record);
        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }
    Schema::closeRel(targetRel);

    printf("The number of comparisons: %d\n", StaticBuffer::compCount);

    return SUCCESS;
}

// Stage-7 :

int Algebra::insert(char *relName, int numberOfAttributes, char (*record)[16]) {
    if (!strcmp(relName, RELCAT_RELNAME) || !strcmp(relName, ATTRCAT_RELNAME)) {
        return E_NOTPERMITTED;
    }

    int relId = OpenRelTable::getRelId(relName);
    if (relId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    if (relCatEntry.numAttrs != numberOfAttributes) {
        return E_NATTRMISMATCH;
    }

    union Attribute recordValues[numberOfAttributes];
    // convert 2D char array of record values to Attribute array recordValues
    for (int i = 0; i < numberOfAttributes; i++) {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);

        int type = attrCatEntry.attrType;
        if (type == NUMBER) {
            if (isNumber(record[i])) {
                recordValues[i].nVal = atof(record[i]);
            } else {
                return E_ATTRTYPEMISMATCH;
            }
        } else if (type == STRING) {
            strcpy(recordValues[i].sVal, record[i]);
        }
    }

    int retVal = BlockAccess::insert(relId, recordValues);
    return retVal;
}

// Stage-9 :

// Project All Attributes (Copy Relation)
int Algebra::project(char *srcRel, char *targetRel) {
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    // Creating and Opening target relation
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    int src_nAttrs = relCatEntry.numAttrs;
    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_type[src_nAttrs];

    for (int i = 0; i < src_nAttrs; i++) {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
        strcpy(attr_names[i], attrCatEntry.attrName);
        attr_type[i] = attrCatEntry.attrType;
    }
    // Create relation
    int retVal = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_type);
    if (retVal != SUCCESS) {
        return retVal;
    }
    // Open Relation
    retVal = Schema::openRel(targetRel);
    if (retVal != SUCCESS) {
        // if cache is full
        Schema::deleteRel(targetRel);
        return retVal;
    }
    int targetRelId = OpenRelTable::getRelId(targetRel);

    /*** Inserting projected records into the target relation ***/
    RelCacheTable::resetSearchIndex(srcRelId);
    Attribute record[src_nAttrs];

    while (BlockAccess::project(srcRelId, record) == SUCCESS) {
        retVal = BlockAccess::insert(targetRelId, record);
        if (retVal != SUCCESS) {
            // if disk is full
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return retVal;
        }
    }
    Schema::closeRel(targetRel);

    return SUCCESS;
}

// Project Specified Attributes
int Algebra::project(char *srcRel, char *targetRel, int tar_nAttrs, char (*tar_Attrs)[16]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
    int src_nAttrs = relCatEntry.numAttrs;

    int attr_offset[tar_nAttrs];
    int attr_types[tar_nAttrs];

    /*** Checking if attributes of target are present in the source relation
         and storing its offsets and types ***/

    for (int i = 0; i < tar_nAttrs; i++) {
        AttrCatEntry attrCatEntry;
        int ret = AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &attrCatEntry);
        if (ret == E_ATTRNOTEXIST) {
            return E_ATTRNOTEXIST;
        }
        attr_offset[i] = attrCatEntry.offset;
        attr_types[i] = attrCatEntry.attrType;
    }

    /*** Creating and opening the target relation ***/
    int ret = Schema::createRel(targetRel, tar_nAttrs, tar_Attrs, attr_types);
    if (ret != SUCCESS) {
        return ret;
    }

    ret = Schema::openRel(targetRel);
    if (ret != SUCCESS) {
        // Cache is full
        Schema::deleteRel(targetRel);
        return ret;
    }
    int targetRelId = OpenRelTable::getRelId(targetRel);

    /*** Inserting projected records into the target relation ***/
    RelCacheTable::resetSearchIndex(srcRelId);
    Attribute record[src_nAttrs];

    while (BlockAccess::project(srcRelId, record) == SUCCESS) {
        Attribute proj_record[tar_nAttrs];

        for (int i = 0; i < tar_nAttrs; i++) {
            proj_record[i] = record[attr_offset[i]];
        }
        ret = BlockAccess::insert(targetRelId, proj_record);
        if (ret != SUCCESS) {
            // Disk is full
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }
    Schema::closeRel(targetRel);

    return SUCCESS;
}

// Stage-12 :

int Algebra::join(char *srcRelOne, char *srcRelTwo, char *targetRel, char *attrOne, char *attrTwo) {
    int relId_1 = OpenRelTable::getRelId(srcRelOne);
    int relId_2 = OpenRelTable::getRelId(srcRelTwo);
    if (relId_1 == E_RELNOTOPEN || relId_2 == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry1, attrCatEntry2;
    int ret1 = AttrCacheTable::getAttrCatEntry(relId_1, attrOne, &attrCatEntry1);
    int ret2 = AttrCacheTable::getAttrCatEntry(relId_2, attrTwo, &attrCatEntry2);
    if (ret1 == E_ATTRNOTEXIST || ret2 == E_ATTRNOTEXIST) {
        return E_ATTRNOTEXIST;
    }

    if (attrCatEntry1.attrType != attrCatEntry2.attrType) {
        return E_ATTRTYPEMISMATCH;
    }

    RelCatEntry relCatEntry_1, relCatEntry_2;
    RelCacheTable::getRelCatEntry(relId_1, &relCatEntry_1);
    RelCacheTable::getRelCatEntry(relId_2, &relCatEntry_2);

    int numAttrs1 = relCatEntry_1.numAttrs;
    int numAttrs2 = relCatEntry_2.numAttrs;

    for (int i = 0; i < numAttrs1; i++) {
        AttrCatEntry attrCatBuf1;
        AttrCacheTable::getAttrCatEntry(relId_1, i, &attrCatBuf1);

        for (int j = 0; j < numAttrs2; j++) {
            AttrCatEntry attrCatBuf2;
            AttrCacheTable::getAttrCatEntry(relId_2, j, &attrCatBuf2);

            if (i == attrCatEntry1.offset && j == attrCatEntry2.offset) {
                continue;
            }

            if (!strcmp(attrCatBuf1.attrName, attrCatBuf2.attrName)) {
                return E_DUPLICATEATTR;
            }
        }
    }

    if (attrCatEntry2.rootBlock == -1) {
        int ret = BPlusTree::bPlusCreate(relId_2, attrTwo);
        if (ret == E_DISKFULL) {
            return E_DISKFULL;
        }
    }

    int numAttrsInTarget = numAttrs1 + numAttrs2 - 1;
    char targetRelAttrNames[numAttrsInTarget][ATTR_SIZE];
    int targetRelAttrTypes[numAttrsInTarget];

    for (int i = 0; i < numAttrs1; i++) {
        AttrCatEntry attrCatBuf;
        AttrCacheTable::getAttrCatEntry(relId_1, i, &attrCatBuf);
        strcpy(targetRelAttrNames[i], attrCatBuf.attrName);
        targetRelAttrTypes[i] = attrCatBuf.attrType;
    }
    bool flag = 0;
    for (int i = 0; i < numAttrs2; i++) {
        AttrCatEntry attrCatBuf;
        AttrCacheTable::getAttrCatEntry(relId_2, i, &attrCatBuf);
        if(attrCatBuf.offset == attrCatEntry2.offset) {
            flag = 1;
            continue; 
        }
        strcpy(targetRelAttrNames[numAttrs1 + i - flag], attrCatBuf.attrName);
        targetRelAttrTypes[numAttrs1 + i - flag] = attrCatBuf.attrType;
    }

    int ret = Schema::createRel(targetRel, numAttrsInTarget, targetRelAttrNames, targetRelAttrTypes);
    if (ret != SUCCESS) {
        // MAX_RELATIONS, DISK_FULL, RELATION EXISTS, (DUPLICATE_ATTRS !?)
        return ret;
    }

    int targetRelId = OpenRelTable::openRel(targetRel);
    if (targetRelId == E_CACHEFULL) {
        Schema::deleteRel(targetRel);
        return E_CACHEFULL;
    }

    Attribute record1[numAttrs1];
    Attribute record2[numAttrs2];
    Attribute targetRecord[numAttrsInTarget];

    RelCacheTable::resetSearchIndex(relId_1);
    // loop to get each record of srcRelOne one-by-one
    while (BlockAccess::project(relId_1, record1) == SUCCESS) {
        RelCacheTable::resetSearchIndex(relId_2);           // (required??)
        AttrCacheTable::resetSearchIndex(relId_2, attrTwo); // BPlusSearch using index

        // loop to get every record that satisfies the equi-join condition
        while (BlockAccess::search(relId_2, record2, attrTwo, record1[attrCatEntry1.offset], EQ) == SUCCESS) {
            for (int i = 0; i < numAttrs1; i++) {
                targetRecord[i] = record1[i];
            }
            flag = 0;
            for (int i = 0; i < numAttrs2; i++) {
                if (i == attrCatEntry2.offset) {
                    flag = 1;
                    continue;
                }
                targetRecord[numAttrs1 + i - flag] = record2[i];
            }
            // insert current record into target relation
            if (BlockAccess::insert(targetRelId, targetRecord) == E_DISKFULL) {
                OpenRelTable::closeRel(targetRelId);
                Schema::deleteRel(targetRel);
                return E_DISKFULL;
            }
        }
    }
    OpenRelTable::closeRel(targetRelId);
    return SUCCESS;
}
