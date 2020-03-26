#ifndef KVTRANS_TRANSDB_H
#define KVTRANS_TRANSDB_H

#include <string>
#include <deque>

namespace KVTrans {
    enum DBStatus {
        OK,
        NOTFOUND,
        INERTERNERROR
    };
    class TransDB {
    public:
        virtual DBStatus get(const std::string &key, std::string &value) = 0;

        virtual DBStatus writeBatch(const std::unordered_map<std::string, std::string> &writeKV, int version) = 0;

        virtual int getVersion() = 0;

        virtual void release() = 0;
    };
} // namespace KVTrans

#endif //KVTRANS_TRANSDB_H
