#ifndef SRC_DATA_TYPES_H_
#define SRC_DATA_TYPES_H_

#include <vector>
#include <utility>
#include <unordered_map>
#include <map>
#include <set>
#include <string>
#include "../util/setutil.h"

#ifndef uint
typedef unsigned int uint;
#endif
typedef std::vector<int> Transaction;
typedef int Item;
typedef std::vector<Item> SimpleTidList;
struct PartitionTidList {
    SimpleTidList fTids;
    int fNrSets;
    static const int SEP;// = -1;
    inline
    bool operator==(const PartitionTidList& b) {
        return fNrSets == b.fNrSets && fTids == b.fTids;
    }
    bool operator!=(const PartitionTidList& b) {
        return fNrSets != b.fNrSets || fTids != b.fTids;
    }
};


typedef std::vector<Item> Itemset;
typedef std::vector<int> Diffset;
typedef std::pair<Itemset, int> CFD;
typedef std::vector<CFD> CFDList;

inline
Itemset itemset(int i) {
    Itemset res(1);
    res[0] = i;
    return res;
}

inline
PartitionTidList convert(const SimpleTidList& tids) {
    return { tids, 1 };
}

inline
SimpleTidList convert(const PartitionTidList& tids) {
    auto res = tids.fTids;
    std::sort(res.begin(), res.end());
    res.erase(res.begin(), res.begin()+tids.fNrSets-1);
    return res;
}

inline
bool lessthan(const PartitionTidList& lhs, const PartitionTidList& rhs) {
     return lhs.fNrSets < rhs.fNrSets;
}

inline
bool lessthan(const SimpleTidList& lhs, const SimpleTidList& rhs) {
    return lhs.size() > rhs.size();
}

struct pairhash {
public:
	template <typename T1, typename T2> 
	size_t operator()(const std::pair<T1,T2>& p) const
	{
		return std::hash<T1>()(p.first) ^ std::hash<T2>()(p.second);
	}
};

namespace std {
template <typename T1, typename T2>  struct hash<pair<T1,T2> >
{
    size_t operator()(const pair<T1,T2>& p) const
    {
        return hash<T1>()(p.first) ^ hash<T2>()(p.second);
    }
};
}

namespace std {
template <typename T> struct hash<vector<T> >
{
    size_t operator()(const vector<T>& xs) const
    {
        size_t res = 0;
        for (const T& x : xs) {
            res ^= hash<T>()(x) + 0x9e3779b9 + (res << 6) + (res >> 2);
        }
        return res;
    }
};
}

class PartitionTable {
public:
    static int fDbSize;

    static PartitionTidList intersection(const PartitionTidList &lhs, const PartitionTidList &rhs);
    static std::vector<PartitionTidList> intersection(const PartitionTidList &lhs, const std::vector<PartitionTidList*> rhses);
    static int partitionError(const PartitionTidList&, const PartitionTidList&);
};

#endif //SRC_DATA_TYPES_H_