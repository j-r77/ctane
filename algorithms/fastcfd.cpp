#include "fastcfd.h"
#include "../util/output.h"

FastCFD::FastCFD(Database& db)
    :BaseMiner(db) {
}

void FastCFD::mineFree() {
    SimpleTidList allTids(fDb.size());
    std::iota(allTids.begin(), allTids.end(), 0);
    fFrees.emplace_back(Itemset(), allTids);
    mineFree(Itemset(), getSingletons(fMinSup));
}

void FastCFD::mine(int minsup) {
    fMinSup = minsup;
    std::vector<Diffset> calD;
    getDiffsets(calD);
    std::vector<int> attrs;
    for (int attr = 0; attr < fDb.nrAttrs(); attr++) {
        std::vector<std::pair<int,int> > counts;
        std::vector<Diffset> calDA = projectDiffsets(calD, attr, counts);
        if (std::find(calDA.begin(), calDA.end(), Diffset()) == calDA.end()) {
            std::vector<Itemset> covs;
            findCovers(Itemset(), counts, calDA, covs);
            for (const Itemset& lhs : covs) {
                if (lhs.size()) {
                    fCFDs.emplace_back(lhs, -1-attr);
                }
            } 
        }
        attrs.push_back(-1-attr);
    }
    mine(Itemset(), getSingletons(fMinSup), attrs);
}

void FastCFD::findCovers(const Itemset& prefix, const std::vector<std::pair<int,int> >& cands, const std::vector<Diffset>& calD, std::vector<Itemset>& covs) {
    for (int ix = cands.size() - 1; ix >= 0; ix--) {
        if (!cands[ix].second) continue;
        int attr = cands[ix].first;
        Itemset lhs = join(prefix, attr);
        std::vector<std::pair<int, int> > subCounts;
        auto calDA = filterDiffsets(calD, attr, subCounts);
        if (calDA.size() == 0) {
            if (!containsSubsetOf(covs, lhs)) {
                covs.push_back(lhs);
            }
            continue;
        }
        if (calDA.size() == calD.size()) continue;
        std::set<int> rAttrs;
        for (int jx = ix + 1; jx < cands.size(); jx++) {
            if (!cands[jx].second) continue;
            rAttrs.insert(cands[jx].first);
        }
        std::vector<std::pair<int, int> > subCands;
        for (const auto& p : subCounts) {
            if (rAttrs.find(p.first) != rAttrs.end() && p.second) {
                subCands.push_back(p);
            }
        }
        if (subCands.size()) {
            findCovers(lhs, subCands, calDA, covs);
        }
    }
}

void FastCFD::mine(const Itemset& prefix, const std::vector<MinerNode<SimpleTidList> >& items, const Itemset& attrs) {
    // Reverse pre-order traversal of items
    std::vector<int> rightItems;
    rightItems.reserve(items.size());
    for (int ix = items.size()-1; ix >= 0; ix--) {
        const MinerNode<SimpleTidList>& node = items[ix];
        const Itemset iset = join(prefix, node.fItem);
        if (fGens.addMinGen(iset, node.fSupp, node.fHash)) {
            std::vector<Diffset> calD ;
            getDiffsets(node.fTids, calD);
            Itemset subAttrs;
            for (int a : attrs) {
                // Free itemsets covers this attribute -> Trivial CFD
                if (contains(fDb.getAttrVectorItems(prefix), a)) continue;

                // All tuples have same rhs -> Add Constant CFD
                int rhs = project(node.fTids, -1-a);
                if (rhs >= 0) {
                    fCFDs.emplace_back(iset, rhs);
                    continue;
                }
                subAttrs.push_back(a);
                std::vector<std::pair<int,int> > counts;
                std::vector<Diffset> calDA = projectDiffsets(calD, -1-a, counts);
                if (std::find(calDA.begin(), calDA.end(), Diffset()) == calDA.end()) {
                    std::vector<Itemset> covs;
                    findCovers(Itemset(), counts, calDA, covs);
                    for (const Itemset& lhs : covs) {
                        if (lhs.size()) {
                            fCFDs.emplace_back(join(iset, lhs), a);
                        }
                    } 
                }
            }

            std::vector<MinerNode<SimpleTidList> > suffix;
            if (items.size() - ix - 1 > 2 * fDb.nrAttrs()) {
                std::vector<SimpleTidList> ijtidMap = bucketTids(rightItems, node.fTids);
                for (uint jx = 0; jx < rightItems.size(); jx++) {
                    int jtem = rightItems[jx];
                    SimpleTidList& ijtids = ijtidMap[jx];
                    int ijsupp = ijtids.size();
                    if (ijsupp != node.fSupp && ijsupp >= fMinSup) {
                        suffix.push_back(MinerNode<SimpleTidList>(jtem, std::move(ijtids)));
                    }
                }
            }
            else {
                for (uint jx = ix + 1; jx < items.size(); jx++) {
                    const MinerNode<SimpleTidList>& j = items[jx];
                    SimpleTidList ijtids = intersection(node.fTids, j.fTids);
                    int ijsupp = ijtids.size();
                    if (ijsupp != node.fSupp && ijsupp >= fMinSup) {
                        suffix.push_back(MinerNode<SimpleTidList>(j.fItem, std::move(ijtids)));
                    }
                }
            }

            // Sort suffix and recurse
            if (suffix.size()) {
                std::sort(suffix.begin(), suffix.end());
                mine(iset, suffix, subAttrs);
            }
            rightItems.push_back(node.fItem);
        }
    }
}

int FastCFD::nrCFDs() const {
    return fCFDs.size();
}

void FastCFD::mineFree(const Itemset& prefix, const std::vector<MinerNode<SimpleTidList> >& items) {
    // Reverse pre-order traversal of items
    std::vector<int> rightItems;
    rightItems.reserve(items.size());
    for (int ix = items.size()-1; ix >= 0; ix--) {
        const MinerNode<SimpleTidList>& node = items[ix];
        const Itemset iset = join(prefix, node.fItem);
        if (fGens.addMinGen(iset, node.fSupp, node.fHash)) {
            fFrees.emplace_back(iset, node.fTids);
            std::vector<MinerNode<SimpleTidList> > suffix;
            if (items.size() - ix - 1 > 2 * fDb.nrAttrs()) {
                std::vector<SimpleTidList> ijtidMap = bucketTids(rightItems, node.fTids);
                for (uint jx = 0; jx < rightItems.size(); jx++) {
                    int jtem = rightItems[jx];
                    SimpleTidList& ijtids = ijtidMap[jx];
                    int ijsupp = ijtids.size();
                    if (ijsupp != node.fSupp && ijsupp >= fMinSup) {
                        suffix.push_back(MinerNode<SimpleTidList>(jtem, std::move(ijtids)));
                    }
                }
            }
            else {
                for (uint jx = ix + 1; jx < items.size(); jx++) {
                    const MinerNode<SimpleTidList>& j = items[jx];
                    SimpleTidList ijtids = intersection(node.fTids, j.fTids);
                    int ijsupp = ijtids.size();
                    if (ijsupp != node.fSupp && ijsupp >= fMinSup) {
                        suffix.push_back(MinerNode<SimpleTidList>(j.fItem, std::move(ijtids)));
                    }
                }
            }
            
            // Sort suffix and recurse
            if (suffix.size()) {
                std::sort(suffix.begin(), suffix.end());
                mineFree(iset, suffix);
            }
            rightItems.push_back(node.fItem);
        }
    }
}