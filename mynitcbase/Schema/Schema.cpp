#include "Schema.h"

#include <cmath>
#include <cstring>

int Schema::openRel(char *relName) {
    // the OpenRelTable::openRel() function returns the rel-id if successful
    // a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
    // error codes will be negative
    int ret = OpenRelTable::openRel(relName);
    if (ret >= 0) {
        return SUCCESS;
    }
    return ret;
}

int Schema::closeRel(char *relName) {
    if (!strcmp(relName, RELCAT_RELNAME) || !strcmp(relName, ATTRCAT_RELNAME)) {
        return E_NOTPERMITTED;
    }
    // this function returns the rel-id of a relation if it is open or
    // E_RELNOTOPEN if it is not. we will implement this later.
    int relId = OpenRelTable::getRelId(relName);

    if (relId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }
    return OpenRelTable::closeRel(relId);
}

// Stage-6 :

int Schema::renameRel(char *oldRelName, char *newRelName) {
    if (!strcmp(oldRelName, RELCAT_RELNAME) || !strcmp(oldRelName, ATTRCAT_RELNAME)) {
        return E_NOTPERMITTED;
    }
    if (!strcmp(newRelName, RELCAT_RELNAME) || !strcmp(newRelName, ATTRCAT_RELNAME)) {
        return E_NOTPERMITTED;
    }
    if (OpenRelTable::getRelId(oldRelName) != E_RELNOTOPEN) {
        return E_RELOPEN;
    }
    int retVal = BlockAccess::renameRelation(oldRelName, newRelName);
    return retVal;
}

int Schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName) {
    if (!strcmp(relName, RELCAT_RELNAME) || !strcmp(relName, ATTRCAT_RELNAME)) {
        return E_NOTPERMITTED;
    }
    if (OpenRelTable::getRelId(relName) != E_RELNOTOPEN) {
        return E_RELOPEN;
    }
    int retVal = BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);
    return retVal;
}

int Schema::createRel(char *relName, int numOfAttributes, char (*attrNames)[ATTR_SIZE], int *attrType) {
    Attribute relNameAsAttribute;
    strcpy(relNameAsAttribute.sVal, relName);
    RecId targetRelId;
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    targetRelId = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAsAttribute, EQ); /////
    if (targetRelId.block != -1 && targetRelId.slot != -1) {
        return E_RELEXIST;
    }

    for (int i = 0; i < numOfAttributes; i++) {
        for (int j = i + 1; j < numOfAttributes; j++) {
            if (strcmp(attrNames[i], attrNames[j]) == 0) {
                return E_DUPLICATEATTR;
            }
        }
    }

    Attribute relCatRecord[RELCAT_NO_ATTRS];
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, relName);
    relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal = numOfAttributes;
    relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal = 0;
    relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal = -1;
    relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal = -1;
    relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = floor((2016 / (16 * numOfAttributes + 1)));

    int retVal = BlockAccess::insert(RELCAT_RELID, relCatRecord);
    if (retVal != SUCCESS) {
        // Fails if no more space in Relation Catalog
        return retVal;
    }

    for (int i = 0; i < numOfAttributes; i++) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName);
        strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrNames[i]);
        attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrType[i];
        attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal = i;

        retVal = BlockAccess::insert(ATTRCAT_RELID, attrCatRecord);
        if (retVal != SUCCESS) {
            // Fails if disk is full

            // Necessary to delete relation entry from Relation Catalog
            Schema::deleteRel(relName);

            return E_DISKFULL;
        }
    }

    return SUCCESS;
}

int Schema::deleteRel(char *relName) {
    if (!strcmp(RELCAT_RELNAME, relName) || !strcmp(ATTRCAT_RELNAME, relName)) {
        return E_NOTPERMITTED;
    }

    if (OpenRelTable::getRelId(relName) != E_RELNOTOPEN) {
        return E_RELOPEN;
    }

    return BlockAccess::deleteRelation(relName);
}
