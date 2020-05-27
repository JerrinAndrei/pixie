#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <unistd.h>
#include <string_view>
#include <thread>

#include "src/common/system/tcp_socket.h"
#include "src/shared/metadata/metadata.h"
#include "src/shared/types/column_wrapper.h"
#include "src/shared/types/types.h"
#include "src/stirling/bcc_bpf_interface/socket_trace.h"
#include "src/stirling/data_table.h"
#include "src/stirling/socket_trace_connector.h"
#include "src/stirling/testing/client_server_system.h"
#include "src/stirling/testing/common.h"
#include "src/stirling/testing/socket_trace_bpf_test_fixture.h"

namespace pl {
namespace stirling {

using ::pl::stirling::testing::ColWrapperIsEmpty;
using ::pl::stirling::testing::ColWrapperSizeIs;
using ::pl::stirling::testing::FindRecordIdxMatchesPID;
using ::pl::stirling::testing::FindRecordsMatchingPID;
using ::pl::system::TCPSocket;
using ::pl::types::ColumnWrapper;
using ::pl::types::ColumnWrapperRecordBatch;
using ::testing::Each;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::SizeIs;
using ::testing::StrEq;
using ::testing::UnorderedElementsAre;

constexpr std::string_view kHTTPReqMsg1 = R"(GET /endpoint1 HTTP/1.1
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:67.0) Gecko/20100101 Firefox/67.0

)";

constexpr std::string_view kHTTPReqMsg2 = R"(GET /endpoint2 HTTP/1.1
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:67.0) Gecko/20100101 Firefox/67.0

)";

constexpr std::string_view kHTTPRespMsg1 = R"(HTTP/1.1 200 OK
Content-Type: application/json; msg1
Content-Length: 0

)";

constexpr std::string_view kHTTPRespMsg2 = R"(HTTP/1.1 200 OK
Content-Type: application/json; msg2
Content-Length: 0

)";

constexpr std::string_view kNoProtocolMsg = R"(This is not an HTTP message)";

// TODO(yzhao): We'd better rewrite the test to use BCCWrapper directly, instead of
// SocketTraceConnector, to avoid triggering the userland parsing code, so these tests do not need
// change if we alter output format.

// This test requires docker container with --pid=host so that the container's PID and the
// host machine are identical.
// See https://stackoverflow.com/questions/33328841/pid-mapping-between-docker-and-host

// TODO(oazizi): HTTP tracing should be based on server-side tracing. Tests need adjustment.
class SocketTraceBPFTest : public testing::SocketTraceBPFTest</* TClientSideTracing */ true> {};

TEST_F(SocketTraceBPFTest, Framework) {
  ConfigureCapture(kProtocolHTTP, kRoleAll);

  testing::SendRecvScript script({
      {{kHTTPReqMsg1}, {kHTTPRespMsg1}},
      {{kHTTPReqMsg2}, {kHTTPRespMsg2}},
  });

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>(script);

  DataTable data_table(kHTTPTable);
  source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
  types::ColumnWrapperRecordBatch& record_batch = *data_table.ActiveRecordBatch();

  ASSERT_THAT(record_batch, Each(ColWrapperSizeIs(4)));
}

// TODO(chengruizhe): Add test targeted at checking IPs.

TEST_F(SocketTraceBPFTest, WriteRespCapture) {
  ConfigureCapture(kProtocolHTTP, kRoleServer);

  testing::SendRecvScript script({
      {{kHTTPReqMsg1}, {kHTTPRespMsg1}},
      {{kHTTPReqMsg2}, {kHTTPRespMsg2}},
  });

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>(script);

  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch record_batch =
        FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kHTTPUPIDIdx, getpid());

    ASSERT_THAT(record_batch, Each(ColWrapperSizeIs(2)));

    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(0), HasSubstr("msg1"));
    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(1), HasSubstr("msg2"));

    // Additional verifications. These are common to all HTTP1.x tracing, so we decide to not
    // duplicate them on all relevant tests.
    EXPECT_EQ(1, record_batch[kHTTPMajorVersionIdx]->Get<types::Int64Value>(0).val);
    EXPECT_EQ(static_cast<uint64_t>(HTTPContentType::kJSON),
              record_batch[kHTTPContentTypeIdx]->Get<types::Int64Value>(0).val);
    EXPECT_EQ(1, record_batch[kHTTPMajorVersionIdx]->Get<types::Int64Value>(1).val);
    EXPECT_EQ(static_cast<uint64_t>(HTTPContentType::kJSON),
              record_batch[kHTTPContentTypeIdx]->Get<types::Int64Value>(1).val);
  }

  // Check that MySQL table did not capture any data.
  {
    DataTable data_table(kMySQLTable);
    source_->TransferData(ctx_.get(), kMySQLTableNum, &data_table);
    types::ColumnWrapperRecordBatch record_batch =
        FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kMySQLUPIDIdx, getpid());

    ASSERT_THAT(record_batch, Each(ColWrapperIsEmpty()));
  }
}

TEST_F(SocketTraceBPFTest, SendRespCapture) {
  ConfigureCapture(TrafficProtocol::kProtocolHTTP, kRoleServer);

  testing::SendRecvScript script({
      {{kHTTPReqMsg1}, {kHTTPRespMsg1}},
      {{kHTTPReqMsg2}, {kHTTPRespMsg2}},
  });

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Recv, &TCPSocket::Send>(script);

  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch record_batch =
        FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kHTTPUPIDIdx, getpid());

    ASSERT_THAT(record_batch, Each(ColWrapperSizeIs(2)));

    // These 2 EXPECTs require docker container with --pid=host so that the container's PID and the
    // host machine are identical.
    // See https://stackoverflow.com/questions/33328841/pid-mapping-between-docker-and-host

    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(0), HasSubstr("msg1"));
    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(1), HasSubstr("msg2"));
  }

  // Check that MySQL table did not capture any data.
  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch record_batch =
        FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kMySQLUPIDIdx, getpid());

    ASSERT_THAT(record_batch, Each(ColWrapperIsEmpty()));
  }
}

TEST_F(SocketTraceBPFTest, ReadRespCapture) {
  ConfigureCapture(TrafficProtocol::kProtocolHTTP, kRoleClient);

  testing::SendRecvScript script({
      {{kHTTPReqMsg1}, {kHTTPRespMsg1}},
      {{kHTTPReqMsg2}, {kHTTPRespMsg2}},
  });

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>(script);

  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch record_batch =
        FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kHTTPUPIDIdx, getpid());

    ASSERT_THAT(record_batch, Each(ColWrapperSizeIs(2)));

    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(0), HasSubstr("msg1"));
    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(1), HasSubstr("msg2"));
  }

  // Check that MySQL table did not capture any data.
  {
    DataTable data_table(kMySQLTable);
    source_->TransferData(ctx_.get(), kMySQLTableNum, &data_table);
    types::ColumnWrapperRecordBatch record_batch =
        FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kMySQLUPIDIdx, getpid());

    ASSERT_THAT(record_batch, Each(ColWrapperIsEmpty()));
  }
}

TEST_F(SocketTraceBPFTest, RecvRespCapture) {
  ConfigureCapture(TrafficProtocol::kProtocolHTTP, kRoleClient);

  testing::SendRecvScript script({
      {{kHTTPReqMsg1}, {kHTTPRespMsg1}},
      {{kHTTPReqMsg2}, {kHTTPRespMsg2}},
  });

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Recv, &TCPSocket::Send>(script);

  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch record_batch =
        FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kHTTPUPIDIdx, getpid());

    ASSERT_THAT(record_batch, Each(ColWrapperSizeIs(2)));

    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(0), HasSubstr("msg1"));
    EXPECT_THAT(record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(1), HasSubstr("msg2"));
  }

  // Check that MySQL table did not capture any data.
  {
    DataTable data_table(kMySQLTable);
    source_->TransferData(ctx_.get(), kMySQLTableNum, &data_table);
    types::ColumnWrapperRecordBatch record_batch =
        FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kMySQLUPIDIdx, getpid());

    ASSERT_THAT(record_batch, Each(ColWrapperIsEmpty()));
  }
}

TEST_F(SocketTraceBPFTest, NoProtocolWritesNotCaptured) {
  ConfigureCapture(TrafficProtocol::kProtocolHTTP, kRoleAll);
  ConfigureCapture(TrafficProtocol::kProtocolMySQL, kRoleClient);

  testing::SendRecvScript script({
      {{kNoProtocolMsg}, {kNoProtocolMsg}},
      {{kNoProtocolMsg}, {kNoProtocolMsg}},
  });

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>(script);

  // Check that HTTP table did not capture any data.
  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch record_batch =
        FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kHTTPUPIDIdx, getpid());

    ASSERT_THAT(record_batch, Each(ColWrapperIsEmpty()));
  }

  // Check that MySQL table did not capture any data.
  {
    DataTable data_table(kMySQLTable);
    source_->TransferData(ctx_.get(), kMySQLTableNum, &data_table);
    types::ColumnWrapperRecordBatch record_batch =
        FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kMySQLUPIDIdx, getpid());

    ASSERT_THAT(record_batch, Each(ColWrapperIsEmpty()));
  }
}

TEST_F(SocketTraceBPFTest, MultipleConnections) {
  ConfigureCapture(TrafficProtocol::kProtocolHTTP, kRoleClient);

  // Two separate connections.

  testing::SendRecvScript script1({
      {{kHTTPReqMsg1}, {kHTTPRespMsg1}},
  });
  testing::ClientServerSystem system1;
  system1.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>(script1);

  testing::SendRecvScript script2({
      {{kHTTPReqMsg2}, {kHTTPRespMsg2}},
  });
  testing::ClientServerSystem system2;
  system2.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>(script2);

  {
    DataTable data_table(kHTTPTable);
    source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
    types::ColumnWrapperRecordBatch record_batch =
        FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kHTTPUPIDIdx, getpid());

    ASSERT_THAT(record_batch, Each(ColWrapperSizeIs(2)));

    std::vector<std::pair<int64_t, std::string>> results;
    for (int i = 0; i < 2; ++i) {
      results.emplace_back(std::make_pair(
          md::UPID(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(i).val).pid(),
          record_batch[kHTTPRespHeadersIdx]->Get<types::StringValue>(i)));
    }

    EXPECT_THAT(results, UnorderedElementsAre(Pair(getpid(), HasSubstr("msg1")),
                                              Pair(getpid(), HasSubstr("msg2"))));
  }
}

TEST_F(SocketTraceBPFTest, StartTime) {
  ConfigureCapture(TrafficProtocol::kProtocolHTTP, kRoleClient);

  testing::SendRecvScript script({
      {{kHTTPReqMsg1}, {kHTTPRespMsg1}},
      {{kHTTPReqMsg2}, {kHTTPRespMsg2}},
  });

  testing::ClientServerSystem system;
  system.RunClientServer<&TCPSocket::Recv, &TCPSocket::Send>(script);

  // Kernel uses monotonic clock as start_time, so we must do the same.
  auto now = std::chrono::steady_clock::now();

  // Use a time window to make sure the recorded PID start_time is right.
  // Being super generous with the window, just in case test runs slow.
  auto time_window_start_tp = now - std::chrono::minutes(30);
  auto time_window_end_tp = now + std::chrono::minutes(5);

  // Start times are reported by Linux in what is essentially 10 ms units.
  constexpr int64_t kNsecondsPerSecond = 1000 * 1000 * 1000;
  constexpr int64_t kClockTicks = 100;
  constexpr int64_t kDivFactor = kNsecondsPerSecond / kClockTicks;

  auto time_window_start = time_window_start_tp.time_since_epoch().count() / kDivFactor;
  auto time_window_end = time_window_end_tp.time_since_epoch().count() / kDivFactor;

  DataTable data_table(kHTTPTable);
  source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
  types::ColumnWrapperRecordBatch record_batch =
      FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kHTTPUPIDIdx, getpid());

  ASSERT_THAT(record_batch, Each(ColWrapperSizeIs(2)));

  md::UPID upid0(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(0).val);
  EXPECT_EQ(getpid(), upid0.pid());
  EXPECT_LT(time_window_start, upid0.start_ts());
  EXPECT_GT(time_window_end, upid0.start_ts());

  md::UPID upid1(record_batch[kHTTPUPIDIdx]->Get<types::UInt128Value>(1).val);
  EXPECT_EQ(getpid(), upid1.pid());
  EXPECT_LT(time_window_start, upid1.start_ts());
  EXPECT_GT(time_window_end, upid1.start_ts());
}

// TODO(yzhao): Apply this pattern to other syscall pairs. An issue is that other syscalls do not
// use scatter buffer. One approach would be to concatenate inner vector to a single string, and
// then feed to the syscall. Another caution is that value-parameterized tests actually discourage
// changing functions being tested according to test parameters. The canonical pattern is using test
// parameters as inputs, but keep the function being tested fixed.
enum class SyscallPair {
  kSendRecvMsg,
  kWriteReadv,
};

class SyscallPairBPFTest : public testing::SocketTraceBPFTest</* TClientSideTracing */ false>,
                           public ::testing::WithParamInterface<SyscallPair> {};

TEST_P(SyscallPairBPFTest, EventsAreCaptured) {
  ConfigureCapture(kProtocolHTTP, kRoleAll);

  testing::SendRecvScript script({
      {{kHTTPReqMsg1},
       {"HTTP/1.1 200 OK\r\n", "Content-Type: json\r\n", "Content-Length: 1\r\n\r\na"}},
      {{kHTTPReqMsg2},
       {"HTTP/1.1 404 Not Found\r\n", "Content-Type: json\r\n", "Content-Length: 2\r\n\r\nbc"}},
  });

  testing::ClientServerSystem system;
  switch (GetParam()) {
    case SyscallPair::kSendRecvMsg:
      system.RunClientServer<&TCPSocket::RecvMsg, &TCPSocket::SendMsg>(script);
      break;
    case SyscallPair::kWriteReadv:
      system.RunClientServer<&TCPSocket::ReadV, &TCPSocket::WriteV>(script);
      break;
  }

  DataTable data_table(kHTTPTable);
  source_->TransferData(ctx_.get(), kHTTPTableNum, &data_table);
  types::ColumnWrapperRecordBatch record_batch =
      FindRecordsMatchingPID(*data_table.ActiveRecordBatch(), kHTTPUPIDIdx, getpid());

  // 2 for sendmsg() and 2 for recvmsg().
  ASSERT_THAT(record_batch, Each(ColWrapperSizeIs(4)));

  for (int i : {0, 2}) {
    EXPECT_EQ(200, record_batch[kHTTPRespStatusIdx]->Get<types::Int64Value>(i).val);
    EXPECT_THAT(std::string(record_batch[kHTTPRespBodyIdx]->Get<types::StringValue>(i)),
                StrEq("a"));
    EXPECT_THAT(std::string(record_batch[kHTTPRespMessageIdx]->Get<types::StringValue>(i)),
                StrEq("OK"));
  }
  for (int i : {1, 3}) {
    EXPECT_EQ(404, record_batch[kHTTPRespStatusIdx]->Get<types::Int64Value>(i).val);
    EXPECT_THAT(std::string(record_batch[kHTTPRespBodyIdx]->Get<types::StringValue>(i)),
                StrEq("bc"));
    EXPECT_THAT(std::string(record_batch[kHTTPRespMessageIdx]->Get<types::StringValue>(i)),
                StrEq("Not Found"));
  }
}

INSTANTIATE_TEST_SUITE_P(IOVecSyscalls, SyscallPairBPFTest,
                         ::testing::Values(SyscallPair::kSendRecvMsg, SyscallPair::kWriteReadv));

}  // namespace stirling
}  // namespace pl
