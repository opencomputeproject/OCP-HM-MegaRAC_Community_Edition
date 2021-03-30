#include "message.hpp"
#include <gtest/gtest.h>
#include <stdlib.h>

namespace {
    uint8_t p[] ={0x3, 0x32, 0x34, 0x36};

event_record_t build_event_record(
        const char* message,
        const char* severity,
        const char* association,
        const char* reportedby,
        const uint8_t* p,
        size_t n)
{
    return event_record_t{
            const_cast<char*> (message),
            const_cast<char*> (severity),
            const_cast<char*> (association),
            const_cast<char*> (reportedby),
            const_cast<uint8_t*> (p),
            n};

}
}

class TestEnv : public testing::Test
{
public:
    char eventsDir[15] = "./eventsXXXXXX";
    char *eventsDirPoint = nullptr;
    TestEnv() {
        eventsDirPoint = mkdtemp(eventsDir);
        if(eventsDirPoint == NULL)
        {
            throw std::bad_alloc();
        }
        CreateEventPath();
    }
    ~TestEnv() {
        RemoveEventPath();
    }
    void CreateEventPath()
    {
        char *cmd = nullptr;
        int resultAsp = 0;
        int resultSys = 0;
        resultAsp = asprintf(&cmd, "exec rm -r %s 2> /dev/null", eventsDir);
        if(resultAsp == -1){
           throw std::bad_alloc();
        }
        resultSys = system(cmd);
        if(resultSys != 0){
           throw std::system_error();
        }
        resultAsp = asprintf(&cmd, "exec mkdir  %s 2> /dev/null", eventsDir);
        if(resultAsp == -1){
            throw std::bad_alloc();
        }
        resultSys = system(cmd);
        if(resultSys != 0){
            throw std::system_error();
        }
        free(cmd);
    }
    void RemoveEventPath()
    {
        char *cmd = nullptr;
        int resultAsp = 0;
        int resultSys = 0;

        resultAsp = asprintf(&cmd, "exec rm -r %s 2> /dev/null", eventsDir);
        if(resultAsp == -1){
            throw std::bad_alloc();
        }
        resultSys = system(cmd);
        if(resultSys != 0){
            throw std::system_error();
        }
        free(cmd);
    }
};

class TestEventManager : public TestEnv
{
public:
    event_manager eventManager;

    TestEventManager()
      : eventManager(eventsDir, 0, 0)
    {
        // empty
    }
    uint16_t prepareEventLog1() {
        auto rec = build_event_record("Testing Message1", "Info",
                           "Association", "Test", p, 4);
        return eventManager.create(&rec);
    }
    uint16_t prepareEventLog2() {
        auto rec = build_event_record("Testing Message2", "Info",
                            "Association", "Test", p, 4);
        return eventManager.create(&rec);
    }
};

TEST_F(TestEventManager, ConstructDestruct)
{
}

TEST_F(TestEventManager, BuildEventLogZero) {
   EXPECT_EQ(0, eventManager.get_managed_size());
   EXPECT_EQ(0, eventManager.next_log());
   eventManager.next_log_refresh();
   EXPECT_EQ(0, eventManager.next_log());
   EXPECT_EQ(0, eventManager.latest_log_id());
   EXPECT_EQ(0, eventManager.log_count());
}

TEST_F(TestEventManager, BuildEventLogOne) {
   auto msgId = prepareEventLog1();
   EXPECT_EQ(1,  msgId);
   EXPECT_EQ(75, eventManager.get_managed_size());
   EXPECT_EQ(1,  eventManager.log_count());
   EXPECT_EQ(1,  eventManager.latest_log_id());
   eventManager.next_log_refresh();
   /* next_log() uses readdir which reads differently per
      system so just make sure its not zero. */
   EXPECT_NE(0,  eventManager.next_log());
   eventManager.next_log_refresh();
}

TEST_F(TestEventManager, BuildEventLogTwo) {
   auto msgId = prepareEventLog1();
   EXPECT_EQ(1, msgId);
   msgId = prepareEventLog2();
   EXPECT_EQ(2, msgId);
   EXPECT_EQ(150, eventManager.get_managed_size());
   EXPECT_EQ(2,   eventManager.log_count());
   EXPECT_EQ(2,   eventManager.latest_log_id());
   eventManager.next_log_refresh();
   /* next_log() uses readdir which reads differently per
      system so just make sure its not zero. */
   EXPECT_NE(0,   eventManager.next_log());
   EXPECT_NE(0,   eventManager.next_log());
}

/* Read Log 1 and 2 */
TEST_F(TestEventManager, ReadLogs) {
   auto msgId = prepareEventLog1();
   EXPECT_EQ(1, msgId);
   msgId = prepareEventLog2();
   EXPECT_EQ(2, msgId);
   event_record_t *prec;

   EXPECT_EQ(1, eventManager.open(1, &prec));
   EXPECT_STREQ("Testing Message1", prec->message);
   eventManager.close(prec);

   EXPECT_EQ(2, eventManager.open(2, &prec));
   EXPECT_STREQ("Testing Message2", prec->message);
   eventManager.close(prec);
}

/* Lets delete the earlier log, then create a new event manager
   the latest_log_id should still be 2 */
TEST_F(TestEventManager, DeleteLogOne) {
   auto msgId = prepareEventLog1();
   EXPECT_EQ(1, msgId);
   msgId = prepareEventLog2();
   EXPECT_EQ(2, msgId);
   EXPECT_EQ(0, eventManager.remove(1));
   EXPECT_EQ(75, eventManager.get_managed_size());

   event_manager eventq(eventsDir, 0, 0);
   EXPECT_EQ(2, eventq.latest_log_id());
   EXPECT_EQ(1, eventq.log_count());
   eventManager.next_log_refresh();
}

/* Travese log list stuff */
TEST_F(TestEventManager, TraverseLogListOne) {
   event_manager eventa(eventsDir, 0, 0);
   EXPECT_EQ(0, eventa.next_log());

   auto rec = build_event_record("Testing list", "Info",
                            "Association", "Test", p, 4);
   EXPECT_EQ(1, eventa.create(&rec));
   EXPECT_EQ(2, eventa.create(&rec));

   event_manager eventb(eventsDir, 0, 0);
   /* next_log() uses readdir which reads differently per
      system so just make sure its not zero. */
   EXPECT_NE(0, eventb.next_log());
}

TEST_F(TestEnv, MaxLimitSize76) {

   event_manager eventd(eventsDir, 75, 0);
   auto rec = build_event_record("Testing Message1", "Info",
                            "Association", "Test", p, 4);
   EXPECT_EQ(0, eventd.create(&rec));

   event_manager evente(eventsDir, 76, 0);
   rec = build_event_record("Testing Message1", "Info",
                            "Association", "Test", p, 4);
   EXPECT_EQ(1, evente.create(&rec));
}

TEST_F(TestEnv, MaxLimitSize149) {
   event_manager eventf(eventsDir, 149, 0);
   auto rec = build_event_record("Testing Message1", "Info",
                            "Association", "Test", p, 4);
   EXPECT_EQ(1, eventf.create(&rec));
   EXPECT_EQ(0, eventf.create(&rec));
}

TEST_F(TestEnv, MaxLimitLog1) {
   event_manager eventg(eventsDir, 300, 1);
   auto rec = build_event_record("Testing Message1", "Info",
                            "Association", "Test", p, 4);
   EXPECT_EQ(1, eventg.create(&rec));
   EXPECT_EQ(0, eventg.create(&rec));
   EXPECT_EQ(1, eventg.log_count());
}

TEST_F(TestEnv, MaxLimitLog3) {
   event_manager eventh(eventsDir, 600, 3);
   auto rec = build_event_record("Testing Message1", "Info",
                            "Association", "Test", p, 4);
   EXPECT_EQ(1, eventh.create(&rec));
   EXPECT_EQ(2, eventh.create(&rec));
   EXPECT_EQ(3, eventh.create(&rec));
   EXPECT_EQ(0, eventh.create(&rec));
   EXPECT_EQ(3, eventh.log_count());
}

/* Create an abundence of logs, then restart with a limited set  */
/* You should not be able to create new logs until the log size  */
/* dips below the request number                                 */
TEST_F(TestEnv, CreateLogsRestartSetOne) {
   event_manager eventi(eventsDir, 600, 3);
   auto rec = build_event_record("Testing Message1", "Info",
                            "Association", "Test", p, 4);
   EXPECT_EQ(1, eventi.create(&rec));
   EXPECT_EQ(2, eventi.create(&rec));
   EXPECT_EQ(3, eventi.create(&rec));
   EXPECT_EQ(0, eventi.create(&rec));
   EXPECT_EQ(3, eventi.log_count());

   event_manager eventj(eventsDir, 600, 1);
   EXPECT_EQ(3, eventj.log_count());
   EXPECT_EQ(0, eventj.create(&rec));
   EXPECT_EQ(3, eventj.log_count());

   /* Delete logs to dip below the requested limit */
   EXPECT_EQ(0, eventj.remove(3));
   EXPECT_EQ(2, eventj.log_count());
   EXPECT_EQ(0, eventj.create(&rec));
   EXPECT_EQ(0, eventj.remove(2));
   EXPECT_EQ(1, eventj.log_count());
   EXPECT_EQ(0, eventj.create(&rec));
   EXPECT_EQ(0, eventj.remove(1));
   EXPECT_EQ(0, eventj.log_count());
   EXPECT_EQ(7, eventj.create(&rec));
}

/* Create an abundence of logs, then restart with a limited set  */
/* You should not be able to create new logs until the log size  */
/* dips below the request number                                 */
TEST_F(TestEnv, CreateLogsRestartSetTwo) {
   event_manager eventk(eventsDir, 600, 100);
   auto rec = build_event_record("Testing Message1", "Info",
                            "Association", "Test", p, 4);
   EXPECT_EQ(1, eventk.create(&rec));
   EXPECT_EQ(2, eventk.create(&rec));
   /* Now we have consumed 150 bytes */
   event_manager eventl(eventsDir, 151, 100);
   EXPECT_EQ(0, eventl.create(&rec));
   EXPECT_EQ(0, eventl.remove(2));
   EXPECT_EQ(4, eventl.create(&rec));
}
