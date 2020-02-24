//
// Created by hww1996 on 2020/2/22.
//

#ifndef KVTRANS_TRANS_H
#define KVTRANS_TRANS_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mutex>

#include "mvcc.h"

namespace KVTrans {
    enum ILEVEL {
        RR, RC
    };

    class Trans {
    public:
        Trans(TransDB *db);

        DBStatus get(const std::string &key, std::string &value);

        DBStatus getForUpdate(const std::string &key, std::string &value);

        DBStatus put(const std::string &key, const std::string &value);

        DBStatus del(const std::string &key);

        ILEVEL getTransLevel();

        void setTransLevle(ILEVEL type);

        bool begin();

        bool commit();

        bool rollback();

        int getTransID();

        ~TransDB();

    private:
        TransDB *db_;
        std::unordered_map<std::string, MVCCInfo> writeMap_;
        std::unordered_set<std::string> keyLocks_;
        ILEVEL transLevel;
        bool transStart;
        int transID;
        static int gTransID;
        static std::mutex gTransIDLock;
    };
} // namespace KVTrans

#endif //KVTRANS_TRANS_H
