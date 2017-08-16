#include "types.h"
#include "../util/output.h"

int PartitionTable::fDbSize;
const int PartitionTidList::SEP = -1;

std::vector<PartitionTidList> PartitionTable::intersection(const PartitionTidList &lhs, const std::vector<PartitionTidList*> rhses) {
    std::unordered_map<int, int> eqIndices(support(lhs.fTids));
    std::vector<std::vector<int> > eqClasses(lhs.fNrSets);
    // Construct a lookup from tid to equivalence class
    int eix = 0;
    int count = 0;
    for (int ix = 0; ix <= lhs.fTids.size(); ix++) {
        count++;
        if (ix == lhs.fTids.size() || lhs.fTids[ix] == PartitionTidList::SEP) {
            eqClasses[eix++].reserve(count);
            count = 0;
        }
        else {
            eqIndices[lhs.fTids[ix]] = eix+1;
        }
    }
    std::vector<PartitionTidList> res;
    for (const PartitionTidList* rhs : rhses) {
        res.emplace_back();
        res.back().fNrSets = 0;
        res.back().fTids.reserve(lhs.fTids.size());
        for (int ix = 0; ix <= rhs->fTids.size(); ix++) {
            if (ix == rhs->fTids.size() || rhs->fTids[ix] == PartitionTidList::SEP) {
                for (auto& eqcl : eqClasses) {
                    if (eqcl.size()) {
                        res.back().fTids.insert(res.back().fTids.end(),eqcl.begin(),eqcl.end());
                        res.back().fTids.push_back(PartitionTidList::SEP);
                        res.back().fNrSets++;
                        eqcl.clear();
                    }
                }
            }
            else {
                const int jt = rhs->fTids[ix];
                if (eqIndices.count(jt)) {
                    eqClasses[eqIndices[jt] - 1].push_back(jt);
                }
            }
        }
        if (res.back().fTids.size() && res.back().fTids.back() == PartitionTidList::SEP) {
            res.back().fTids.pop_back();
        }
    }
    return res;
}

PartitionTidList PartitionTable::intersection(const PartitionTidList &lhs, const PartitionTidList &rhs) {
    std::unordered_map<int, int> eqIndices;
    eqIndices.reserve(lhs.fTids.size() + 1 - lhs.fNrSets);
    std::vector<std::vector<int> > eqClasses(lhs.fNrSets);

    // Construct a lookup from tid to equivalence class
    int eix = 0;
    int count = 0;
    for (int ix = 0; ix <= lhs.fTids.size(); ix++) {
        count++;
        if (lhs.fTids[ix] == PartitionTidList::SEP || ix == lhs.fTids.size()) {
            eqClasses[eix].reserve(count);
            count = 0;
            eix++;
        }
        else {
            eqIndices[lhs.fTids[ix]] = eix+1;
        }
    }
    // For each rhs partition, for each eq class: spread all tids over eq classes in lhs
    PartitionTidList res;
    res.fNrSets = 0;
    for (int ix = 0; ix <= rhs.fTids.size(); ix++) {
        if (rhs.fTids[ix] == PartitionTidList::SEP || ix == rhs.fTids.size()) {
            for (auto& eqcl : eqClasses) {
                if (eqcl.size()) {
                    res.fTids.insert(res.fTids.end(),eqcl.begin(),eqcl.end());
                    res.fTids.push_back(PartitionTidList::SEP);
                    res.fNrSets++;
                    eqcl.clear();
                }
            }
        }
        else {
            const int jt = rhs.fTids[ix];
            if (eqIndices[jt]) {
                eqClasses[eqIndices[jt] - 1].push_back(jt);
            }
        }
    }
    if (res.fTids.size() && res.fTids.back() == PartitionTidList::SEP) {
        res.fTids.pop_back();
    }
    return res;
}

int PartitionTable::partitionError(const PartitionTidList& x, const PartitionTidList& xa) {
    int e = 0;

    std::map<int,int> bigt;
    //bigt.reserve(xa.fNrSets);
    int count = 0;
    for (int pi = 0; pi <= xa.fTids.size(); pi++) {
        if (pi == xa.fTids.size() || xa.fTids[pi] == PartitionTidList::SEP) {
            bigt[xa.fTids[pi-1]] = count;
            count = 0;
        }
        else {
            count++;
        }
    }

    int eix = 0;
    count = 0;
    int m = 0;
    for (int cix = 0; cix <= x.fTids.size(); cix++) {
        if (cix == x.fTids.size() || x.fTids[cix] == PartitionTidList::SEP) {
            e += count - m;
            eix++;
            m = 0;
            count = 0;
        }
        else {
            count++;
            int t = x.fTids[cix];
            if (bigt.count(t)) {
                if (bigt[t] > m) {
                    m = bigt[t];
                }
            }
        }
    }
    return e;
}