#include "Algebra.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

int isNumber(char *str){
    int len;
    float ignore;

    int ret = sscanf(str,"%f %n", &ignore, &len);
    return ret==1 && len==strlen(str);
}

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]){
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if(srcRelId == E_RELNOTOPEN){
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);
    if(ret != SUCCESS){
        return ret;
    }

    /*** Convert strVal (string) to an attribute of data type NUMBER or STRING ***/
    int type = attrCatEntry.attrType;
    Attribute attrVal;
    if(type == NUMBER){
        if(isNumber(strVal)) {
            attrVal.nVal = atof(strVal);
        } else{
            return E_ATTRTYPEMISMATCH;
        }
    } else if(type == STRING){
        strcpy(attrVal.sVal, strVal);
    }

    /*** Selecting records from the source relation ***/

    RelCacheTable::resetSearchIndex(srcRelId);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    /************************
     The following code prints the contents of a relation directly to the output
    console. Direct console output is not permitted by the actual the NITCbase
    specification and the output can only be inserted into a new relation. We will
    be modifying it in the later stages to match the specification.
    ************************/

    printf("|");
    for (int i = 0; i < relCatEntry.numAttrs; ++i) {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

        printf(" %s |", attrCatEntry.attrName);
    }
    printf("\n");   

    while(true){
        RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op);
        if(searchRes.block != -1 && searchRes.slot != -1){
            RecBuffer blockObj(searchRes.block);
            Attribute rec[relCatEntry.numAttrs];
            blockObj.getRecord(rec, searchRes.slot);
            printf("|");
            for (int i = 0; i < relCatEntry.numAttrs; ++i) {
                AttrCatEntry attrCatEntry;
                AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

                if(attrCatEntry.attrType == STRING){
                    printf(" %s |", rec[i].sVal);
                }else{
                    printf(" %d |", (int)rec[i].nVal);
                }
            }
            printf("\n"); 
        }else{
            // all records over
            break;
        }
    }
    return SUCCESS;
}