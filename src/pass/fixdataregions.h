#ifndef EGALITO_PASS_FIX_DATA_REGIONS_H
#define EGALITO_PASS_FIX_DATA_REGIONS_H

#include "chunkpass.h"

class FixDataRegionsPass : public ChunkPass {
private:
    Module *module;
public:
    virtual void visit(Module *module);
    virtual void visit(DataRegionList *dataRegionList);
    virtual void visit(DataRegion *dataRegion);
};

#endif