//
// Created by fquinn on 6/8/20.
//

#include "Benchmarker.h"
#include "omnmmsgpayloadfunctions.h"
// C++ Header
#include <mama/MamaMsg.h>
// C Header
#include <mama/msg.h>
#include <mama/integration/msg.h>

// We always want the asserts to apply - this is not a release binary tool.
#undef NDEBUG
#include <assert.h>

#define SEND_TIME_FID 16
#define PADDING_FID 10004
#define SEQ_NUM_FID 10

#define ITERATION_COUNT 100000000
#define ONE_MILLION 1000000

using namespace Wombat;

char gPadding[200];

void populateTestMamaMsg(MamaMsg* msg) {
    uint32_t seqNum = 1;
    // Use a message format similar to mamaproducer / consumer
    mamaPayloadBridge omnmBridge;
    MamaDateTime dateTime;
    dateTime.setToNow();
    mama_getPayloadBridge(&omnmBridge, "omnmmsg");
    msg->createForPayloadBridge(omnmBridge);
    msg->addU32(NULL, SEQ_NUM_FID, seqNum);
    msg->addDateTime(NULL, SEND_TIME_FID, dateTime);
    msg->addString(NULL, PADDING_FID, gPadding);
}

void Benchmarker::runIterationTests(uint64_t repeats, bool readOnly, bool directAccess) {
    MamaMsg* msg = new MamaMsg();
    populateTestMamaMsg(msg);
    uint32_t seqNum = 1;

    class : public MamaMsgFieldIterator{
    public:
        void onField (const MamaMsg& msg,
                const MamaMsgField& field,
                void* closure) {
            switch (field.getFid()) {
                case SEQ_NUM_FID:
                    mSeqNum = field.getU32();
                    break;
                case SEND_TIME_FID:
                    field.getDateTime(mDateTime);
                    break;
                case PADDING_FID:
                    mPadding = field.getString();
                    break;
            }
        }
        uint32_t mSeqNum;
        MamaDateTime mDateTime;
        const char* mPadding;
    } cb;

    MamaDateTime dateTime;
    msg->getDateTime(NULL, SEND_TIME_FID, dateTime);
    for (uint64_t i = 1; i <= repeats; i++) {
        // Update some fields
        if (!readOnly) {
            dateTime.setToNow();
            msg->updateU32(NULL, SEQ_NUM_FID, i);
            msg->updateDateTime(NULL, SEND_TIME_FID, dateTime);
        }

        // Parse these fields to populate values
        if (!directAccess) {
            msg->iterateFields(cb, NULL, NULL);
        } else {
            cb.mSeqNum = msg->getU32(NULL, SEQ_NUM_FID);
            msg->getDateTime(NULL, SEND_TIME_FID, cb.mDateTime);
            cb.mPadding = msg->getString(NULL, PADDING_FID);
        }

        // Assert equality
        if (!readOnly) {
            assert (cb.mSeqNum == i);
        } else {
            assert (cb.mSeqNum == seqNum);
        }
        assert (cb.mDateTime.getEpochTimeMicroseconds() == dateTime.getEpochTimeMicroseconds());
        assert (memcmp(gPadding, cb.mPadding, sizeof(gPadding)) == 0);
    }
}

void Benchmarker::runSerializationTests(uint64_t repeats) {
    MamaDateTime dateTime;
    MamaMsg* msg = new MamaMsg();
    populateTestMamaMsg(msg);
    mamaMsg mmsg = msg->getUnderlyingMsg();
    msgPayload payload;
    mamaMsgImpl_getPayload(mmsg, &payload);
    for (uint64_t i = 1; i <= repeats; i++) {
        const void* buf;
        mama_size_t bufLen;
        omnmmsgPayload_serialize(payload, &buf, &bufLen);
        omnmmsgPayload_unSerialize(payload, buf, bufLen);
    }
}

int main(int argc, char* argv[]) {
    Mama::loadBridge("qpid");
    Mama::loadPayloadBridge("omnmmsg");
    Mama::open();
    Benchmarker* benchmarker = new Benchmarker();

    memset(gPadding, 'A', sizeof(gPadding));
    gPadding[sizeof(gPadding)-1] = '\0';

    MamaDateTime overallStart;
    MamaDateTime overallFinish;
    MamaDateTime start;
    MamaDateTime finish;

    overallStart.setToNow();

    start.setToNow();
    benchmarker->runIterationTests(ITERATION_COUNT, false, false);
    finish.setToNow();
    uint64_t timeTaken = finish.getEpochTimeMicroseconds() - start.getEpochTimeMicroseconds();
    printf("Benchmark for iterations with updates and writes: %fs\n", ((float)timeTaken) / ONE_MILLION);

    start.setToNow();
    benchmarker->runIterationTests(ITERATION_COUNT, true, false);
    finish.setToNow();
    timeTaken = finish.getEpochTimeMicroseconds() - start.getEpochTimeMicroseconds();
    printf("Benchmark for iterations with (mostly) just reads: %fs\n", ((float)timeTaken) / ONE_MILLION);

    start.setToNow();
    benchmarker->runIterationTests(ITERATION_COUNT, false, true);
    finish.setToNow();
    timeTaken = finish.getEpochTimeMicroseconds() - start.getEpochTimeMicroseconds();
    printf("Benchmark for iterations with direct field lookup (no iteration): %fs\n", ((float)timeTaken) / ONE_MILLION);

    start.setToNow();
    benchmarker->runSerializationTests(ITERATION_COUNT);
    finish.setToNow();
    timeTaken = finish.getEpochTimeMicroseconds() - start.getEpochTimeMicroseconds();
    printf("Benchmark for Serialization / Deserialization tests: %fs\n", ((float)timeTaken) / ONE_MILLION);

    overallFinish.setToNow();
    uint64_t overallTimeTaken = overallFinish.getEpochTimeMicroseconds() - overallStart.getEpochTimeMicroseconds();
    printf("Total for all tests: %fs\n", ((float)overallTimeTaken) / ONE_MILLION);
}
