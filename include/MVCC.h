//
// Created by 黄卫玮 on 2020/2/22.
//

#include <shared_mutex>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>
#include <atomic>

#ifndef KVTRANS_MVCC_H
#define KVTRANS_MVCC_H

#include "TransDB.h"

namespace KVTrans {
    struct MVCCInfo {
        uint64_t version_;
        bool isDel_;
        std::string key_;
        std::string value_;
        MVCCInfo(uint64_t version, bool isDel, const std::string &key, const std::string &value);
        int encode(std::string &ans);
        int decode(const std::string &key, const std::string &val);
        void clone(MVCCInfo &mvccInfo);
    };

    class MVCC {
    public:
        static int getVersion();
        static int commitVersion(int newVersion);

        static int getForUpdate(TransDB *db, int transid, const std::string &key, MVCCInfo &mvccInfo);
        static int get(TransDB *db, uint64_t version, const std::string &key, MVCCInfo &mvccInfo);  // 有可能需要排他锁
        static int apppendVersion(int transid, const std::string &key, MVCCInfo &mvccInfo);

        static int getRowLock(int transId, const std::string &key);
        static int releaseRowLocks(int transId, const std::unordered_set<std::string> &keys);
    private:

        class RowInfo {
        public:
            RowInfo();
            RowInfo(const std::string &key);
            RowInfo(const std::string &key ,int lockTransId);

            int setLock(int transId); // 需要排他锁
            void releaseLock(int transId);
            int getLock();

            int getForUpdate(TransDB *db, int transid, MVCCInfo &mvccInfo);
            int get(TransDB *db, uint64_t version, MVCCInfo &mvccInfo); // 有可能需要排他锁
            int apppendVersion(int transid, MVCCInfo &mvccInfo);

        private:
            std::list<MVCCInfo> history_;
            int lockTransId_;
            std::string key_;
        };

        static std::shared_mutex mvccInfoLock;
        static std::unordered_map<std::string, RowInfo> mvccMap;  // 假如key，那么这个RowInfo的history_不为空。
        static std::atomic<uint64_t> version_;
    };
} // namespace KVTrans

#endif //KVTRANS_MVCC_H
