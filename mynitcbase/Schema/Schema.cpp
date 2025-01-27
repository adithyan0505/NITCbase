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
