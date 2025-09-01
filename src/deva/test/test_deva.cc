#include <gtest/gtest.h>
#include "pain/base/scope_exit.h"
#include "pain/proto/deva.pb.h"
#include "deva/mock/mock_deva.h"
#include "deva/sdk/rpc_client.h"

namespace {

class TestDeva : public ::testing::Test {
public:
    pain::Status mkdir(const std::string& path, pain::proto::deva::MkdirResponse* response) {
        pain::proto::deva::MkdirRequest request;
        request.set_path(path);
        return pain::deva::call_rpc(
            _mock_deva.group().c_str(), &pain::proto::deva::DevaService::Mkdir, &request, response);
    }

    pain::Status open(const std::string& path,
                      const pain::proto::deva::OpenFlag& flags,
                      pain::proto::deva::OpenFileResponse* response) {
        pain::proto::deva::OpenFileRequest request;
        request.set_path(path);
        request.set_flags(flags);
        return pain::deva::call_rpc(
            _mock_deva.group().c_str(), &pain::proto::deva::DevaService::OpenFile, &request, response);
    }

    pain::Status readdir(const std::string& path, pain::proto::deva::ReadDirResponse* response) {
        pain::proto::deva::ReadDirRequest request;
        request.set_path(path);
        return pain::deva::call_rpc(
            _mock_deva.group().c_str(), &pain::proto::deva::DevaService::ReadDir, &request, response);
    }

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

    pain::proto::deva::OpenFileResponse response;
    status = open("/test.txt", pain::proto::deva::OpenFlag::OPEN_CREATE, &response);

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

    pain::proto::deva::MkdirResponse mkdir_response;
    status = mkdir("/test", &mkdir_response);
    ASSERT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
    std::cout << "mkdir response: " << mkdir_response.DebugString() << std::endl;

    pain::proto::deva::OpenFileResponse response;
    status = open("/test/test.txt", pain::proto::deva::OpenFlag::OPEN_CREATE, &response);
    EXPECT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
    std::cout << "response: " << response.DebugString() << std::endl;
}

TEST_F(TestDeva, ReadDir) {
    _mock_deva.start();
    SCOPE_EXIT {
        _mock_deva.stop();
    };
    std::string leader;
    auto status = _mock_deva.wait_for_leader(&leader);
    EXPECT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
    std::cout << "leader: " << leader << std::endl;

    pain::proto::deva::MkdirResponse mkdir_response;
    status = mkdir("/test", &mkdir_response);
    ASSERT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
    std::cout << "mkdir response: " << mkdir_response.DebugString() << std::endl;

    for (int i = 0; i < 10; i++) { // NOLINT(readability-magic-numbers)
        pain::proto::deva::OpenFileResponse response;
        status = open(fmt::format("/test/test_file_{}.txt", i), pain::proto::deva::OpenFlag::OPEN_CREATE, &response);
        ASSERT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
        std::cout << "response: " << response.DebugString() << std::endl;
    }

    for (int i = 0; i < 10; i++) { // NOLINT(readability-magic-numbers)
        pain::proto::deva::MkdirResponse response;
        status = mkdir(fmt::format("/test/test_dir_{}", i), &response);
        ASSERT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
        std::cout << "mkdir response: " << response.DebugString() << std::endl;
    }

    pain::proto::deva::ReadDirResponse readdir_response;
    status = readdir("/test", &readdir_response);
    ASSERT_TRUE(status.ok()) << status.error_str() << "(" << status.error_code() << ")";
    std::cout << "readdir response: " << readdir_response.DebugString() << std::endl;
    ASSERT_EQ(readdir_response.entries_size(), 20);
    for (int i = 0; i < 10; i++) { // NOLINT(readability-magic-numbers)
        EXPECT_EQ(readdir_response.entries(i).name(), fmt::format("test_file_{}.txt", i));
        EXPECT_EQ(readdir_response.entries(i).type(), pain::proto::FileType::FILE_TYPE_FILE);
    }
    for (int i = 0; i < 10; i++) { // NOLINT(readability-magic-numbers)
        EXPECT_EQ(readdir_response.entries(i + 10).name(), fmt::format("test_dir_{}", i));
        EXPECT_EQ(readdir_response.entries(i + 10).type(), pain::proto::FileType::FILE_TYPE_DIRECTORY);
    }
}

} // namespace
