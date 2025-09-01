#include <gtest/gtest.h>
#include "pain/base/scope_exit.h"
#include "pain/proto/deva.pb.h"
#include "deva/mock/mock_deva.h"
#include "deva/sdk/rpc_client.h"

namespace {

class TestDeva : public ::testing::Test {
protected:
    pain::deva::mock::MockDeva _mock_deva;
};

TEST_F(TestDeva, Basic) {
    _mock_deva.start();
    SCOPE_EXIT {
        _mock_deva.stop();
    };
    std::string leader;
    auto status = _mock_deva.wait_for_leader(&leader);
    EXPECT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
    std::cout << "leader: " << leader << std::endl;
    auto it = std::find_if(
        _mock_deva.node_addrs().begin(), _mock_deva.node_addrs().end(), [&leader](const std::string& addr) {
            return leader.find(addr) != std::string::npos;
        });
    EXPECT_NE(it, _mock_deva.node_addrs().end()) << "leader: " << leader;
}

TEST_F(TestDeva, StartTwoNodes) {
    _mock_deva.start(0);
    _mock_deva.start(1);

    SCOPE_EXIT {
        _mock_deva.stop(0);
        _mock_deva.stop(1);
    };
    std::string leader;
    auto status = _mock_deva.wait_for_leader(&leader);
    EXPECT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
    std::cout << "leader: " << leader << std::endl;
    auto it = std::find_if(
        _mock_deva.node_addrs().begin(), _mock_deva.node_addrs().end(), [&leader](const std::string& addr) {
            return leader.find(addr) != std::string::npos;
        });
    EXPECT_NE(it, _mock_deva.node_addrs().end()) << "leader: " << leader;
}

TEST_F(TestDeva, CreateFile) {
    _mock_deva.start();
    SCOPE_EXIT {
        _mock_deva.stop();
    };
    std::string leader;
    auto status = _mock_deva.wait_for_leader(&leader);
    EXPECT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
    std::cout << "leader: " << leader << std::endl;

    pain::proto::deva::OpenFileRequest request;
    pain::proto::deva::OpenFileResponse response;
    request.set_path("/test.txt");
    request.set_flags(pain::proto::deva::OpenFlag::OPEN_CREATE);
    status = pain::deva::call_rpc(
        _mock_deva.group().c_str(), &pain::proto::deva::DevaService::OpenFile, &request, &response);

    EXPECT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
    std::cout << "response: " << response.DebugString() << std::endl;
}

TEST_F(TestDeva, CreateDirectoryAndFile) {
    _mock_deva.start();
    SCOPE_EXIT {
        _mock_deva.stop();
    };
    std::string leader;
    auto status = _mock_deva.wait_for_leader(&leader);
    EXPECT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
    std::cout << "leader: " << leader << std::endl;

    pain::proto::deva::MkdirRequest mkdir_request;
    pain::proto::deva::MkdirResponse mkdir_response;
    mkdir_request.set_path("/test");
    status = pain::deva::call_rpc(
        _mock_deva.group().c_str(), &pain::proto::deva::DevaService::Mkdir, &mkdir_request, &mkdir_response);
    ASSERT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
    std::cout << "mkdir response: " << mkdir_response.DebugString() << std::endl;

    pain::proto::deva::OpenFileRequest request;
    pain::proto::deva::OpenFileResponse response;
    request.set_path("/test/test.txt");
    request.set_flags(pain::proto::deva::OpenFlag::OPEN_CREATE);
    status = pain::deva::call_rpc(
        _mock_deva.group().c_str(), &pain::proto::deva::DevaService::OpenFile, &request, &response);

    EXPECT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
    std::cout << "response: " << response.DebugString() << std::endl;
}

} // namespace
