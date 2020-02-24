//
// Created by hww1996 on 2020/2/24.
//

#ifndef KVTRANS_SERIALWRTIE_H
#define KVTRANS_SERIALWRTIE_H

#include <deque>
#include <atomic>
#include <mutex>

#include "MVCC.h"

namespace KVTrans {

    struct WriteNode {
        WriteNode(std::deque<MVCCInfo*> *writeList);

        /**
         *
         * @return  0 pending 1 OK -1 error
         */
        int getNote();

        std::deque<MVCCInfo*> *writeList_;
        std::atomic<int> note_;
    };

    class SerialWrite {
    public:
        static SerialWrite *getInstance(TransDB *db);

        int pushToWrite(WriteNode *node);

    private:
        SerialWrite(TransDB *db);

        static int writeNodes();

        static std::mutex instanceLock;
        static SerialWrite *serialWrite;

        static std::mutex queLock;
        static std::deque<WriteNode *> writeQueue;
        static TransDB *db_;
    };

} // namespace KVTrans

#endif //KVTRANS_SERIALWRTIE_H
