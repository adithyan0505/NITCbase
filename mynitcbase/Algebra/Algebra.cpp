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
    Attribute record[src_nAttrs];

    while (BlockAccess::search(srcRelId, record, attr, attrVal, op) == SUCCESS) {
        ret = BlockAccess::insert(targetRelId, record);
        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }
    Schema::closeRel(targetRel);

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
