#include <string.h>
#include <nvm3.h>

Ecode_t nvm3_writeData(nvm3_Handle_t *h, nvm3_ObjectKey_t key, const void *value, size_t len);
Ecode_t nvm3_readPartialData(nvm3_Handle_t *h, nvm3_ObjectKey_t key, void *value, size_t ofs, size_t len);
Ecode_t nvm3_readData(nvm3_Handle_t *h, nvm3_ObjectKey_t key, void *value, size_t maxLen);
Ecode_t nvm3_eraseAll(nvm3_Handle_t *h);
Ecode_t nvm3_deleteObject(nvm3_Handle_t *h, nvm3_ObjectKey_t key);
Ecode_t nvm3_open(nvm3_Handle_t *h, const nvm3_Init_t *i);
Ecode_t nvm3_getObjectInfo(nvm3_Handle_t *h, nvm3_ObjectKey_t key, uint32_t *type, size_t *len);
bool nvm3_repackNeeded(nvm3_Handle_t *h);
Ecode_t nvm3_repack(nvm3_Handle_t *h);
Ecode_t nvm3_close(nvm3_Handle_t *h);
size_t nvm3_enumObjects(nvm3_Handle_t *h, nvm3_ObjectKey_t *keyListPtr, size_t keyListSize, nvm3_ObjectKey_t keyMin, nvm3_ObjectKey_t keyMax);