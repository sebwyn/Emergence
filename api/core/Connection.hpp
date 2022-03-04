
#include "Protocol.hpp"
#include "Socket.hpp"
#include "Task.hpp"

typedef unsigned int uint;

struct ConnectId {
    std::string ip;
    ushort port;
    ConnectId() : ip(""), port(0) {}
    ConnectId(std::string ip, ushort port) : ip(ip), port(port) {}
    ConnectId(const std::string &buffer) {
        uint cIndex = buffer.find(':');
        ip = buffer.substr(0, cIndex);
        port = std::stoi(buffer.substr(cIndex + 1));
    }

    bool operator==(const ConnectId &other) {
        if (ip == other.ip && port == other.port) {
            return true;
        } else {
            return false;
        }
    }

    std::string toString() { return ip + ":" + std::to_string(port); }
};

struct MessageFrom {
    ConnectId from;
    Protocol::Packet packet;
};

enum Station { connecting, connected, disconnected };

// by default this is a threaded connection
// TODO: think about adding a non-threaded connection
class Connection : public Task {
  public:
    // by default a connection is not established, the first time
    // receive gets called on it it becomes established
    Connection(UdpSocket &socket);

    void connect(ConnectId other, const std::string &message = "");
    void disconnect();

    virtual void sendMessage(const std::string &message) {
        if (station == connected) {
            toSend.access([&message](std::vector<std::string> &data) {
                data.push_back(message);
            });
        }
    }

    std::vector<Protocol::AppData> getMessages() {
        std::vector<Protocol::AppData> out;
        receivedFrom.access(
            [&out](std::vector<Protocol::AppData> &data) { out = data; });
        return out;
    }
    void flushMessages() {
        receivedFrom.access(
            [](std::vector<Protocol::AppData> &data) { data.clear(); });
    }

    void pushToReceive(Protocol::Packet packet) {
        toReceive.access([&packet](std::vector<Protocol::Packet> &data) {
            data.push_back(packet);
        });
    }

    ConnectId getOther() { return other; }
    Station getStation() { return station.load(); }

    void setFps(uint _fps) { fps.store(_fps); }

  protected:
    void execute() override;

    // updates station to either connected or disconnected
    // this can be read after a call and acted on appropriately
    void update();
    void receive(Protocol::Packet packet);

    void sendKeepAlive();
    void send(
        std::vector<Protocol::AppData> &messages,
        std::function<void(Protocol::PacketHandled)> onResend =
            [](Protocol::PacketHandled) {});

    void onConnect();
    void onDisconnect() {}
    // determine if a seq number is acked in a given packet
    bool acked(Protocol::Header header, uint seq);
    // set receivedPackets and localSeqNum based on an incoming packet
    void ack(Protocol::Packet packet);
    void updateMode();

    // interfaces for sending and receiving messages between the main thread and
    // the connection
    Threaded<std::vector<std::string>> toSend;
    Threaded<std::vector<Protocol::AppData>> receivedFrom;
    Threaded<std::vector<Protocol::Packet>> toReceive;          
    std::atomic<uint> fps = 30;

    std::atomic<Station> station = disconnected;
    // while this is acquired by the main thread, it is never set on the
    // connection thread, so no need to make atomic
    ConnectId other;

    // all of these variables should only be referenced by the connection thread
    UdpSocket &socket;

    std::string connectionMessage;
    uint currentMessage = 0;

    uint remoteSeqNum = 0;
    uint localSeqNum = 0;
    std::bitset<32> receivedPackets;

    std::chrono::time_point<std::chrono::high_resolution_clock> lastPacketTime,
        lastSentTime;

    // can replaces this with a circular buffer of messages for better
    // performance
    std::map<uint, Protocol::PacketHandled> sentPackets;

    bool rttDefined;
    double rtt = 0.0;
    uint returnToGood = 15000;
    int trustedLevel = 0;
    Globals::ConnectionState mode = Globals::ConnectionState::GOOD;
    std::chrono::high_resolution_clock::time_point modeStart, lastTrustTime;
};
