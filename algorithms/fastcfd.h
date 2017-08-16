#ifndef ALGORITHMS_FASTCFD_H_
#define ALGORITHMS_FASTCFD_H_

#include "baseminer.h"

class FastCFD : BaseMiner {
public:
    FastCFD(Database&);
    void mine(int);
    void mineFree();
    void mineFree(const Itemset&, const std::vector<MinerNode<SimpleTidList> >&);
    int nrCFDs() const;
    void findCovers(const Itemset&, const std::vector<std::pair<int,int> >&, const std::vector<Diffset>&, std::vector<Itemset>&);
    void mine(const Itemset&, const std::vector<MinerNode<SimpleTidList> >&, const Itemset&);
private:
	GeneratorStore<int> fGens;
	std::vector<std::pair<Itemset, SimpleTidList> > fFrees;
};

#endif // ALGORITHMS_FASTCFD_H_