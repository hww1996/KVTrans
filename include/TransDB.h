#ifndef KVTRANS_TRANSDB_H
#define KVTRANS_TRANSDB_H

#include <string>
#include <deque>

#include "MVCC.h"

namespace KVTrans {
    enum DBStatus {
        OK,
        NOTFOUND,
        INERTERNERROR
    };
    class MVCCInfo;
    class TransDB {
    public:
        virtual DBStatus get(const std::string &key, MVCCInfo &mvccInfo) = 0;

        virtual DBStatus writeBatch(const std::unordered_map<std::string, std::string> &writeKV, int version) = 0;

        virtual int getVersion() = 0;

        virtual void release() = 0;
    };
} // namespace KVTrans

#endif //KVTRANS_TRANSDB_H
