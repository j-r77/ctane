#include <fstream>
#include <iostream>
#include "util/output.h"
#include "data/databasereader.h"
#include "algorithms/ctane.h"


int main(int argc, char *argv[]) {
    if (argc == 4) {
        std::ifstream dbfile(argv[1]);
        Database db = DatabaseReader::fromTable(dbfile, ',');
        int minsup = atoi(argv[2]);
        double minconf = atof(argv[3]);
        CTane ctane(db);
        ctane.mine(minsup, minconf);
        std::cout << "Discovered " << ctane.getCFDs().size() << " CFDs:" << std::endl;
        Output::printCFDList(ctane.getCFDs(), db);
    }
}
