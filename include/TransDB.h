#ifndef KVTRANS_TRANSDB_H
#define KVTRANS_TRANSDB_H

#include <string>
#include <deque>

#include "MVCC.h"

namespace KVTrans {
    class MVCCInfo;
    class TransDB {
    public:
        virtual int get(const std::string &key, MVCCInfo &mvccInfo) = 0;

        virtual int writeBatch(const std::deque<MVCCInfo> &q) = 0;
    };
} // namespace KVTrans

#endif //KVTRANS_TRANSDB_H
