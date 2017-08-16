#ifndef ALGORITHMS_BASEMINER_H_
#define ALGORITHMS_BASEMINER_H_

#include <map>
#include <vector>
#include "minernode.h"
#include "../data/database.h"
#include "../util/setutil.h"
#include "../util/prefixtree.h"

// Datastructure for efficient lookup whether an itemset is free/generator.
// See Charm algorithm for more info.
template<typename T>
class GeneratorStore {
public:
    bool addMinGen(const Itemset& newset, int supp, const T& hash) {
        const auto& genIt = fGenMap.find(hash);
        if (genIt != fGenMap.end()) {
            if (genIt->second.hasStrictSubset(newset, supp)) {
                return false;
            }
            genIt->second.insert(newset, supp);
        }
        else {
            fGenMap[hash].insert(newset, supp);
        }
        return true;
    }

    bool addMinGen(const Itemset& newset, int supp, const T& hash, std::vector<Itemset>& subs) {
        const auto& genIt = fGenMap.find(hash);
        if (genIt != fGenMap.end()) {
            if (genIt->second.hasSubset(newset, supp)) {
                const auto& ptSubs = genIt->second.getSets();
                subs.insert(subs.begin(), ptSubs.begin(), ptSubs.end());
                return false;
            }
            genIt->second.insert(newset, supp);
        }
        else {
            fGenMap[hash].insert(newset, supp);
        }
        return true;
    }

    bool isMinGen(const Itemset& newset, int supp, const T& hash) {
        const auto& genIt = fGenMap.find(hash);
        if (genIt != fGenMap.end()) {
            if (genIt->second.hasStrictSubset(newset, supp)) {
                return false;
            }
        }
        return true;
    }

    std::vector<Itemset> getMinGens(const Itemset& newset, int supp, const T& hash) {
        const auto& genIt = fGenMap.find(hash);
        if (genIt != fGenMap.end()) {
            return genIt->second.getSubsets(newset, supp);
        }
    }
private:
    std::map<T, PrefixTree<Itemset,int>> fGenMap;
};

class BaseMiner {
public:
    BaseMiner(Database&);
    std::vector<MinerNode<PartitionTidList> > getPartitionSingletons();
    std::vector<MinerNode<PartitionTidList> > getAllSingletons(int);
    std::vector<MinerNode<SimpleTidList> > getSingletons(int);
    std::vector<int> filterSameAttr(const Itemset&, const std::vector<int>&) const;
    std::vector<SimpleTidList> bucketTids(const std::vector<int>&, const SimpleTidList&) const;
    bool getDiffsets(std::vector<Diffset>&);
    bool getDiffsets(std::vector<Diffset>&, const Database& db);
    bool getDiffsets(const SimpleTidList&, std::vector<Diffset>&);
    int project(const SimpleTidList&, int);
    int projectConf(const SimpleTidList&, int, double&);
    std::vector<Diffset> projectDiffsets(const std::vector<Diffset>, int, std::vector<std::pair<int,int> >&);
    std::vector<Diffset> filterDiffsets(const std::vector<Diffset>, int, std::vector<std::pair<int,int> >&);
    std::vector<int> getConstantVio(const Itemset&, int, const SimpleTidList&, const Database&);
    std::vector<int> getVariableVio(const Itemset&, int, const SimpleTidList&, const Database&);
    Itemset subsetItems(const Itemset&, const Itemset&);
    int subsetItems(const Itemset&, int);
    const CFDList& getCFDs() const;
    int nrCFDs() const;
    bool precedes(const Itemset& a, const Itemset& b);


protected:
    int fMinSup;
    int fMaxSize;
	Database& fDb;
    CFDList fCFDs;
};

#endif // ALGORITHMS_BASEMINER_H_
