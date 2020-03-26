//
// Created by hww1996 on 2020/2/24.
//

#include <thread>
#include <chrono>
#include <string>
#include <unordered_map>

#include "serialwrtie.h"

namespace KVTrans {
    WriteNode::WriteNode(std::deque<MVCCInfo*> *writeList) : writeList_(writeList), note_(0) {}

    int WriteNode::getNote() {
        int now = note_.load();
        return now;
    }

    std::mutex SerialWrite::instanceLock;
    SerialWrite *SerialWrite::serialWrite = nullptr;
    std::mutex SerialWrite::queLock;
    std::deque<WriteNode *> SerialWrite::writeQueue;
    TransDB *SerialWrite::db_ = nullptr;

    SerialWrite::SerialWrite(TransDB *db) {
        db_ = db;
        std::thread t(writeNodes);
        t.detach();
    }

    SerialWrite *SerialWrite::getInstance(TransDB *db) {
        {
            std::lock_guard<std::mutex> lockGuard(instanceLock);
            if (serialWrite == nullptr) {
                serialWrite = new SerialWrite(db);
            }
        }
        return serialWrite;
    }

    int SerialWrite::pushToWrite(WriteNode *node) {
        int ret = 0;
        {
            std::lock_guard<std::mutex> lockGuard(queLock);
            writeQueue.push_back(node);
        }
        return ret;
    }

    int SerialWrite::writeNodes() {
        int ret = 0;
        while (true) {
            {
                std::lock_guard<std::mutex> lockGuard(queLock);
                while (!writeQueue.empty()) {
                    WriteNode *needWrite = writeQueue.front();
                    writeQueue.pop_front();
                    int nowVersion = MVCC::getVersion();
                    auto &tempWriteList = *(needWrite->writeList_);
                    std::unordered_map<std::string, std::string> wb;
                    for (int i = 0; i < tempWriteList.size(); i++) {
                        tempWriteList[i]->version_ = nowVersion;
                        nowVersion++;
                        std::string tempAns;
                        tempWriteList[i]->encode(tempAns);
                        wb[tempWriteList[i]->key_] = tempAns;
                    }
                    DBStatus s = db_->writeBatch(wb, nowVersion);
                    if (OK == s) {
                        needWrite->note_.store(1);
                        MVCC::commitVersion(nowVersion);
                    } else {
                        needWrite->note_.store(-1);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return ret;
    }

} // namespace KVTrans
