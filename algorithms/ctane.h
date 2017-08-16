#ifndef ALGORITHMS_CTANE_H_
#define ALGORITHMS_CTANE_H_

#include "baseminer.h"

class CTane : public BaseMiner {
public:
    CTane(Database&);
    void mine(int, double=1);
    void pruneCands(std::vector<MinerNode<PartitionTidList> >& items, const Itemset& sub, int out);
    std::vector<std::pair<Itemset,SimpleTidList> > getItemsetLayer();

private:
    GeneratorStore<int> fGens;
    std::unordered_map<Itemset,PartitionTidList> fStore;
    Itemset fAllAttrs;
    PrefixTree<Itemset, Itemset> fCandStore;
    std::vector<MinerNode<SimpleTidList> > fItemsLayer;
    double fMinConf;
};

#endif // ALGORITHMS_CTANE_H_