#include "ctane.h"
#include <iterator>

CTane::CTane(Database& db)
    :BaseMiner(db) {
}

void CTane::mine(int minsup, double minconf) {
    fMaxSize = fDb.nrAttrs();
    fMinSup = minsup;
    fMinConf = minconf;

    // Initial setup, first layer
    fAllAttrs = range(-fDb.nrAttrs(), 0);
    auto allItems = range(1, fDb.nrItems()+1);
    fAllAttrs.insert(fAllAttrs.end(), allItems.begin(), allItems.end());
    PartitionTable::fDbSize = fDb.size();
    auto items = getPartitionSingletons();
    fItemsLayer = getSingletons(fMinSup);
    auto constants = getItemsetLayer();
    for (const auto& cp : constants) {
        items.emplace_back(cp.first[0], convert(cp.second), cp.second.size());
    }

    // Initialize candidate RHS sets for first layer
    for (auto& a : items) {
        Itemset at;
        for (int cat : fAllAttrs) {
            if (a.fItem < 0 && cat >= 0) continue;
            if (a.fItem != cat && fDb.getAttrIndex(a.fItem) == fDb.getAttrIndex(cat)) continue;
            at.push_back(cat);
        }
        a.fCands = at;
    }

    // Store info for empty itemset
    fCandStore = PrefixTree<Itemset, Itemset>();
    fGens.addMinGen(Itemset(), support(convert(iota(fDb.size()))), 0);
    fStore[Itemset()] = convert(iota(fDb.size()));
    fCandStore.insert(Itemset(), fAllAttrs);

    // Process all BFS layers until finished
    while (!items.empty()) {
        // Initialize stores for next layer
        std::unordered_map<Itemset,PartitionTidList> newStore;
        PrefixTree<Itemset, Itemset> newCandStore;

        // We will process the itemsets in the BFS layer, checking for CFDs
        // "next" stores, for every itemset i in the current layer, 
        // the itemsets j with which i should be expanded to produce the next layer 
        // This next layer cannot be instantiated before all itemsets have been processed
        // for CFDs, because of pruning.
        std::vector<std::vector<std::pair<Itemset,int> > > next(items.size());
        for (int i = 0; i < items.size(); i++) {
            MinerNode<PartitionTidList>& inode = items[i];
            const Itemset iset = join(inode.fPrefix, inode.fItem);
            auto insect = intersection(iset, inode.fCands);
            for (int out : insect) {
                Itemset sub = subset(iset, out);
                // Discard Variable -> Constant and Constant -> Variable CFDs
                if (out < 0) {
                    if (sub.size() && !has(sub, [](int si) -> bool { return si < 0; })) continue;
                }
                else {
                    if (!sub.size() || has(sub, [](int si) -> bool { return si < 0; })) continue;
                }
                auto storedSub = fStore.find(sub);
                if (storedSub == fStore.end()) {
                    continue;
                }
                double e = PartitionTable::partitionError(storedSub->second, inode.fTids);
                double conf = 1 - (e / inode.fSupp);
                if (conf >= fMinConf) {
                    fCFDs.emplace_back(sub, out);
                    inode.fCands = subset(inode.fCands, out);
                    inode.fCands = intersection(inode.fCands, iset);
                    pruneCands(items, sub, out);
                }
            }
            if (inode.fCands.empty()) continue;
            // Check whether constant part of the itemset is free/generator
            if (!fGens.addMinGen(where(iset, [](int i){return i >= 0;}), inode.fSupp, inode.fHash)) continue;

            // Find all j's with which i can be extended
            newStore[iset] = inode.fTids;
            newCandStore.insert(iset, inode.fCands);
            auto nodeAttrs = fDb.getAttrVector(iset);
            for (int j = i+1; j < items.size(); j++) {
                if (j == i) continue;
                const auto& jnode = items[j];
                if (jnode.fPrefix != inode.fPrefix) continue;
                if (std::binary_search(nodeAttrs.begin(), nodeAttrs.end(), fDb.getAttrIndex(jnode.fItem))) continue;
                next[i].emplace_back(iset, j);
            }
        }

        // Generate next BFS layer
        std::vector<MinerNode<PartitionTidList> > suffix;
        for (int i = 0; i < items.size(); i++) {
            std::vector<PartitionTidList*> expands;
            std::vector<MinerNode<PartitionTidList> > tmpSuffix;
            for (auto& newsetTup : next[i]) {
                // Prune candidate RHS set
                int j = newsetTup.second;
                Itemset newset = join(newsetTup.first, items[j].fItem);
                auto c = items[i].fCands;
                for (int zz : newset) {
                    auto zsub = subset(newset, zz);
                    auto storedSub = newStore.find(zsub);
                    if (storedSub == newStore.end()) {
                        c.clear();
                        break;
                    }
                    const Itemset &subCands = *newCandStore.find(zsub);
                    c = intersection(c, subCands);
                }
                if (c.size()) {
                    expands.push_back(&items[j].fTids);
                    tmpSuffix.emplace_back(items[j].fItem);
                    tmpSuffix.back().fCands = c;
                    tmpSuffix.back().fPrefix = newsetTup.first;
                }
            }

            // Compute tidlists for all extensions of item i at once
            const auto exps = PartitionTable::intersection(items[i].fTids, expands);
            for (int e = 0; e < exps.size(); e++) {
                if (support(exps[e]) >= fMinSup) {
                    suffix.emplace_back(tmpSuffix[e].fItem, exps[e]);
                    suffix.back().fCands = tmpSuffix[e].fCands;
                    suffix.back().fPrefix = tmpSuffix[e].fPrefix;
                }
            }
        }

        // Initiate next layer
        fStore.swap(newStore);
        fCandStore = newCandStore;
        items.swap(suffix);
    }
}

// Returns the next layer of the itemset lattice, as a list of Itemset, TidList pairs
// Only used once, to get tidlists of singleton items
std::vector<std::pair<Itemset,SimpleTidList> > CTane::getItemsetLayer() {
    std::vector<std::pair<Itemset,SimpleTidList> > res;
    std::vector<MinerNode<SimpleTidList> > suffix;
    for (int i = 0; i < fItemsLayer.size(); i++) {
        const auto &inode = fItemsLayer[i];
        Itemset iset = join(inode.fPrefix, inode.fItem);
        // Check if iset is a free itemset
        if (!fGens.addMinGen(iset, inode.fSupp, inode.fHash)) continue;

        res.emplace_back(iset, inode.fTids);

        // Construct next BFS layer
        for (int j = i + 1; j < fItemsLayer.size(); j++) {
            const auto &jnode = fItemsLayer[j];
            if (jnode.fPrefix != inode.fPrefix) continue;
            Itemset jset = join(jnode.fPrefix, jnode.fItem);
            Itemset newset = join(iset, jset);
            SimpleTidList ijtids = intersection(inode.fTids, jnode.fTids);
            int ijsupp = ijtids.size();
            if (ijsupp >= fMinSup && ijsupp != inode.fSupp) {
                int jtem = newset.back();
                newset.pop_back();
                suffix.emplace_back(jtem, std::move(ijtids), ijsupp, newset);
            }
        }
    }
    std::sort(suffix.begin(), suffix.end());
    fItemsLayer.swap(suffix);
    return res;
}

// Attempt to prune candidate RHS sets C+ in current layer "items",
// after discovering CFD rhs -> lhs
// All sets that precede the CFD's itemset cset are pruned according to details in the paper
void CTane::pruneCands(std::vector<MinerNode<PartitionTidList> >& items, const Itemset& rhs, int lhs) {
    Itemset cset = join(rhs, lhs);
    for (int i = 0; i < items.size(); i++) {
        MinerNode<PartitionTidList> &inode = items[i];
        const Itemset iset = join(inode.fPrefix, inode.fItem);
        if (precedes(rhs, iset)) {
            if (std::find(inode.fCands.begin(), inode.fCands.end(), lhs) != inode.fCands.end()) {
                inode.fCands = subset(inode.fCands, lhs);
            }
            inode.fCands = intersection(inode.fCands, rhs);
        }
    }
}
