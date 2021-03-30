#include "extensions/openpower-pels/data_interface.hpp"
#include "extensions/openpower-pels/host_interface.hpp"

#include <fcntl.h>

#include <filesystem>
#include <sdeventplus/source/io.hpp>

#include <gmock/gmock.h>

namespace openpower
{
namespace pels
{

class MockDataInterface : public DataInterfaceBase
{
  public:
    MockDataInterface()
    {
    }
    MOCK_METHOD(std::string, getMachineTypeModel, (), (const override));
    MOCK_METHOD(std::string, getMachineSerialNumber, (), (const override));
    MOCK_METHOD(std::string, getServerFWVersion, (), (const override));
    MOCK_METHOD(std::string, getBMCFWVersion, (), (const override));
    MOCK_METHOD(std::string, getBMCFWVersionID, (), (const override));
    MOCK_METHOD(bool, getHostPELEnablement, (), (const override));
    MOCK_METHOD(std::string, getBMCState, (), (const override));
    MOCK_METHOD(std::string, getChassisState, (), (const override));
    MOCK_METHOD(std::string, getHostState, (), (const override));
    MOCK_METHOD(std::string, getMotherboardCCIN, (), (const override));
    MOCK_METHOD(void, getHWCalloutFields,
                (const std::string&, std::string&, std::string&, std::string&),
                (const override));
    MOCK_METHOD(std::string, getLocationCode, (const std::string&),
                (const override));
    MOCK_METHOD(const std::vector<std::string>&, getSystemNames, (),
                (const override));
    MOCK_METHOD(std::string, expandLocationCode, (const std::string&, uint16_t),
                (const override));
    MOCK_METHOD(std::string, getInventoryFromLocCode,
                (const std::string&, uint16_t), (const override));

    void changeHostState(bool newState)
    {
        setHostUp(newState);
    }

    void setHMCManaged(bool managed)
    {
        _hmcManaged = managed;
    }
};

/**
 * @brief The mock HostInterface class
 *
 * This replaces the PLDM calls with a FIFO for the asynchronous
 * responses.
 */
class MockHostInterface : public HostInterface
{
  public:
    /**
     * @brief Constructor
     *
     * @param[in] event - The sd_event object
     * @param[in] dataIface - The DataInterface class
     */
    MockHostInterface(sd_event* event, DataInterfaceBase& dataIface) :
        HostInterface(event, dataIface)
    {
        char templ[] = "/tmp/cmdfifoXXXXXX";
        std::filesystem::path dir = mkdtemp(templ);
        _fifo = dir / "fifo";
    }

    /**
     * @brief Destructor
     */
    virtual ~MockHostInterface()
    {
        std::filesystem::remove_all(_fifo.parent_path());
    }

    MOCK_METHOD(CmdStatus, sendNewLogCmd, (uint32_t, uint32_t), (override));

    /**
     * @brief Cancels waiting for a command response
     */
    virtual void cancelCmd() override
    {
        _inProgress = false;
        _source = nullptr;
    }

    /**
     * @brief Returns the amount of time to wait before retrying after
     *        a failed send command.
     *
     * @return milliseconds - The amount of time to wait
     */
    virtual std::chrono::milliseconds getSendRetryDelay() const override
    {
        return std::chrono::milliseconds(2);
    }

    /**
     * @brief Returns the amount of time to wait before retrying after
     *        a command receive.
     *
     * @return milliseconds - The amount of time to wait
     */
    virtual std::chrono::milliseconds getReceiveRetryDelay() const override
    {
        return std::chrono::milliseconds(2);
    }

    /**
     * @brief Returns the amount of time to wait before retrying if the
     *        host firmware's PEL storage was full and it can't store
     *        any more logs until it is freed up somehow.
     *
     * @return milliseconds - The amount of time to wait
     */
    virtual std::chrono::milliseconds getHostFullRetryDelay() const override
    {
        return std::chrono::milliseconds(400);
    }

    /**
     * @brief Returns the number of commands processed
     */
    size_t numCmdsProcessed() const
    {
        return _cmdsProcessed;
    }

    /**
     * @brief Writes the data passed in to the FIFO
     *
     * @param[in] hostResponse - use a 0 to indicate success
     *
     * @return CmdStatus - success or failure
     */
    CmdStatus send(uint8_t hostResponse)
    {
        // Create a FIFO once.
        if (!std::filesystem::exists(_fifo))
        {
            if (mkfifo(_fifo.c_str(), 0622))
            {
                ADD_FAILURE() << "Failed mkfifo " << _fifo << strerror(errno);
                exit(-1);
            }
        }

        // Open it and register the reponse callback to
        // be used on FD activity.
        int fd = open(_fifo.c_str(), O_NONBLOCK | O_RDWR);
        EXPECT_TRUE(fd >= 0) << "Unable to open FIFO";

        auto callback = [this](sdeventplus::source::IO& source, int fd,
                               uint32_t events) {
            this->receive(source, fd, events);
        };

        try
        {
            _source = std::make_unique<sdeventplus::source::IO>(
                _event, fd, EPOLLIN,
                std::bind(callback, std::placeholders::_1,
                          std::placeholders::_2, std::placeholders::_3));
        }
        catch (std::exception& e)
        {
            ADD_FAILURE() << "Event exception: " << e.what();
            close(fd);
            return CmdStatus::failure;
        }

        // Write the fake host reponse to the FIFO
        auto bytesWritten = write(fd, &hostResponse, sizeof(hostResponse));
        EXPECT_EQ(bytesWritten, sizeof(hostResponse));

        _inProgress = true;

        return CmdStatus::success;
    }

  protected:
    /**
     * @brief Reads the data written to the fifo and then calls
     *        the subscriber's callback.
     *
     * Nonzero data indicates a command failure (for testing bad path).
     *
     * @param[in] source - The event source object
     * @param[in] fd - The file descriptor used
     * @param[in] events - The event bits
     */
    void receive(sdeventplus::source::IO& source, int fd,
                 uint32_t events) override
    {
        if (!(events & EPOLLIN))
        {
            return;
        }

        _inProgress = false;

        int newFD = open(_fifo.c_str(), O_NONBLOCK | O_RDONLY);
        ASSERT_TRUE(newFD >= 0) << "Failed to open FIFO";

        // Read the host success/failure response from the FIFO.
        uint8_t data;
        auto bytesRead = read(newFD, &data, sizeof(data));
        EXPECT_EQ(bytesRead, sizeof(data));

        close(newFD);

        ResponseStatus status = ResponseStatus::success;
        if (data != 0)
        {
            status = ResponseStatus::failure;
        }

        callResponseFunc(status);

        // Keep account of the number of commands responses for testing.
        _cmdsProcessed++;
    }

  private:
    /**
     * @brief The event source for the fifo
     */
    std::unique_ptr<sdeventplus::source::IO> _source;

    /**
     * @brief the path to the fifo
     */
    std::filesystem::path _fifo;

    /**
     * @brief The number of commands processed
     */
    size_t _cmdsProcessed = 0;
};

} // namespace pels
} // namespace openpower
