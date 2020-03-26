//
// Created by hww1996 on 2020/2/22.
//
#include <chrono>
#include <thread>

#include "MVCC.h"

namespace KVTrans {
    MVCCInfo::MVCCInfo(uint64_t version,
                       bool isDel,
                       const std::string &key,
                       const std::string &value) : version_(version), isDel_(isDel), key_(key), value_(value) {}

    int MVCCInfo::encode(std::string &ans) {
        return 0;
    }

    int MVCCInfo::decode(const std::string &key, const std::string &val) {
        return 0;
    }

    void MVCCInfo::clone(MVCCInfo &mvccInfo) {
        version_ = mvccInfo.version_;
        isDel_ = mvccInfo.isDel_;
        key_.assign(mvccInfo.key_);
        value_.assign(mvccInfo.value_);
    }

    MVCC::RowInfo::RowInfo() : lockTransId_(-1) {}

    MVCC::RowInfo::RowInfo(const std::string &key) : key_(key), lockTransId_(-1) {}

    MVCC::RowInfo::RowInfo(const std::string &key, int lockTransId) : key_(key), lockTransId_(lockTransId) {}

    int MVCC::RowInfo::setLock(int transId) {
        if (lockTransId_ != -1) {
            if (lockTransId_ == transId) {
                return 0;
            }
            return -1;
        }
        lockTransId_ = transId;
        return 0;
    }

    void MVCC::RowInfo::releaseLock(int transId) {
        if (transId == lockTransId_) {
            lockTransId_ = -1;
        }
    }

    int MVCC::RowInfo::getLock() {
        return lockTransId_;
    }

    int MVCC::RowInfo::getForUpdate(TransDB *db, int transid, MVCCInfo &mvccInfo) {
        if (transid != lockTransId_) {  //判断行锁
            return -1;
        }
        if (history_.empty()) {
            std::string strMvccInfo;
            DBStatus s = db->get(key_, strMvccInfo);
            mvccInfo.decode(key_, strMvccInfo);
            history_.push_back(mvccInfo);
            return 0;
        }
        auto it = history_.end();
        --it;
        mvccInfo.clone(*it);
        return 0;
    }

    int MVCC::RowInfo::get(TransDB *db, uint64_t version, MVCCInfo &mvccInfo) {
        if (lockTransId_ != -1) {
            return -1;
        }
        auto it = history_.begin();
        if (it == history_.end()) {
            return -2;
        }
        mvccInfo.clone(*it);
        ++it;
        for (; it != history_.end(); ++it) {
            if (it->version_ > version) {
                break;
            }
            mvccInfo.clone(*it);
        }
        return 0;
    }

    int MVCC::RowInfo::apppendVersion(int transid, MVCCInfo &mvccInfo) {
        if (transid != lockTransId_) { //判断行锁
            return -1;
        }
        history_.push_back(mvccInfo);
        return 0;
    }


    std::shared_mutex MVCC::mvccInfoLock;
    std::unordered_map<std::string, MVCC::RowInfo> MVCC::mvccMap;
    std::atomic<uint64_t> MVCC::version_(1);

    int MVCC::getVersion() {
        int nowVersion = version_.load();
        return nowVersion;
    }

    int MVCC::commitVersion(int newVersion) {
        int nowVersion = version_.load();
        version_ = newVersion;
        return nowVersion;
    }

    int MVCC::getForUpdate(TransDB *db, int transid, const std::string &key, MVCCInfo &mvccInfo) {
        int ret = 0;
        // 先上行锁
        ret = getRowLock(transid, key);
        mvccInfoLock.lock_shared();
        mvccMap[key].getForUpdate(db, transid, mvccInfo); // 从version中直接获取
        mvccInfoLock.unlock_shared();
        return ret;
    }

    int MVCC::get(TransDB *db, uint64_t version, const std::string &key, KVTrans::MVCCInfo &mvccInfo) {
        int ret = 0;
        mvccInfoLock.lock_shared();
        if (mvccMap.find(key) == mvccMap.end()) {
            ret = -1; // 说明key不存在
        } else {
            while (mvccMap[key].get(db, version, mvccInfo) == -1) { // 假如有行锁就阻塞
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); //阻塞
            }
        }
        mvccInfoLock.unlock_shared();
        if (0 == ret) {
            return ret;
        }
        MVCCInfo temp(0, true, key, "");
        std::string strMVCCInfo;
        DBStatus s = db->get(key, strMVCCInfo);
        temp.decode(key, strMVCCInfo);
        mvccInfo.key_ = key;
        mvccInfo.isDel_ = true;
        mvccInfo.version_ = 0;
        mvccInfoLock.lock();
        if (mvccMap.find(key) == mvccMap.end()) { // 读取并放入key中
            mvccMap.insert(std::pair<std::string, RowInfo>(key, RowInfo(key, 0)));
            mvccMap[key].apppendVersion(0, mvccInfo);
            if (NOTFOUND != s) {
                mvccMap[key].apppendVersion(0, temp);
            }
            mvccMap[key].releaseLock(0);
            ret = 0;
        }
        mvccInfoLock.unlock();

        mvccInfoLock.lock_shared();
        while (mvccMap[key].get(db, version, mvccInfo) == -1) { // 假如有行锁就阻塞
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); //阻塞
        }
        mvccInfoLock.unlock_shared();
        return ret;
    }

    int MVCC::apppendVersion(int transid, const std::string &key, MVCCInfo &mvccInfo) {
        int ret = 0;
        mvccInfoLock.lock_shared();
        if (mvccMap.find(key) == mvccMap.end()) {
            ret = -1;
        } else {
            ret = mvccMap[key].apppendVersion(transid, mvccInfo);
        }
        mvccInfoLock.unlock_shared();
        return ret;
    }

    int MVCC::getRowLock(int transId, const std::string &key) {
        int ret = 0;
        while (true) {
            mvccInfoLock.lock();
            if (mvccMap.find(key) == mvccMap.end()) {
                mvccMap.insert(std::pair<std::string, RowInfo>(key, RowInfo(key, transId)));
            } else {
                ret = mvccMap[key].setLock(transId);
            }
            mvccInfoLock.unlock();
            if (0 == ret) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); //阻塞
        }
        return ret;
    }

    int MVCC::releaseRowLocks(int transId, const std::unordered_set<std::string> &keys) {
        int count = 0;
        mvccInfoLock.lock_shared();
        for (auto it = keys.begin(); it != keys.end(); ++it) {
            if (mvccMap.find(*it) != mvccMap.end()) {
                mvccMap[*it].releaseLock(transId);
                count++;
            }
        }
        mvccInfoLock.unlock_shared();
        return count;
    }

} // namespace KVTrans