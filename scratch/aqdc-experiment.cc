/* SPDX-License-Identifier: GPL-2.0-only */
// Run from the repository root: ./ns3 run aqdc-experiment
// Each run creates results/bbr-baseline-vN/ with exactly four .dat files.

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace ns3;
namespace fs = std::filesystem;

namespace
{
std::ofstream cwndFile, rttFile, queueFile, throughputFile;
Ptr<PacketSink> sink;
Ptr<QueueDisc> queueDisc;
constexpr double startTime = 0.1;
constexpr double stopTime = 120;
constexpr double drainTime = 1;
constexpr double sampleInterval = 0.2;
constexpr uint32_t segmentSize = 1448;
uint64_t previousRx = 0;
double previousRxTime = startTime;

void
Open(std::ofstream& stream, const fs::path& path)
{
    stream.open(path);
    NS_ABORT_MSG_IF(!stream.is_open(), "Cannot write " << path);
    stream << std::setprecision(12);
}

void
CwndChanged(uint32_t, uint32_t value)
{
    cwndFile << Now().GetSeconds() << ' ' << double(value) / segmentSize << '\n';
}

void
RttChanged(Time, Time value)
{
    rttFile << Now().GetSeconds() << ' ' << value.GetSeconds() * 1000 << '\n';
}

void
AttachTraces(Ptr<BulkSendApplication> application)
{
    auto socket = application->GetSocket();
    NS_ABORT_MSG_IF(!socket, "BulkSend socket was not created");
    NS_ABORT_MSG_IF(!socket->TraceConnectWithoutContext("CongestionWindow",
                                                       MakeCallback(&CwndChanged)),
                    "Missing congestion-window trace");
    NS_ABORT_MSG_IF(!socket->TraceConnectWithoutContext("RTT", MakeCallback(&RttChanged)),
                    "Missing RTT trace");
}

void
SampleQueue()
{
    queueFile << Now().GetSeconds() << ' ' << queueDisc->GetNPackets() << '\n';
    if (Now().GetSeconds() + sampleInterval < stopTime + drainTime)
    {
        Simulator::Schedule(Seconds(sampleInterval), &SampleQueue);
    }
}

void
SampleThroughput()
{
    double now = Now().GetSeconds();
    uint64_t received = sink->GetTotalRx();
    double mbps = 8.0 * (received - previousRx) / (now - previousRxTime) / 1e6;
    throughputFile << now << "s " << mbps << " Mbps\n";
    previousRx = received;
    previousRxTime = now;
    double next = std::min(now + sampleInterval, stopTime);
    if (next > now + 1e-9)
    {
        Simulator::Schedule(Seconds(next - now), &SampleThroughput);
    }
}
} // namespace

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);

    fs::path root = fs::absolute("results");
    fs::create_directories(root);
    fs::path output;
    // Atomically reserve the next free version, preserving previous baselines.
    for (uint64_t version = 1;; ++version)
    {
        output = root / ("bbr-baseline-v" + std::to_string(version));
        if (fs::create_directory(output))
        {
            break;
        }
    }
    Open(cwndFile, output / "cwnd.dat");
    Open(rttFile, output / "rtt.dat");
    Open(queueFile, output / "queueSize.dat");
    Open(throughputFile, output / "throughput.dat");

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpBbr::GetTypeId()));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segmentSize));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(4194304));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(6291456));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(2));

    // sender -- R1 -- R2 -- receiver; round-trip propagation delay is 50 ms.
    NodeContainer nodes;
    nodes.Create(4);
    PointToPointHelper edge;
    edge.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    edge.SetChannelAttribute("Delay", TimeValue(Seconds(0.05 / 6)));
    edge.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize("1p")));
    PointToPointHelper core;
    core.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    core.SetChannelAttribute("Delay", TimeValue(Seconds(0.05 / 6)));
    core.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize("1p")));
    auto left = edge.Install(nodes.Get(0), nodes.Get(1));
    auto middle = core.Install(nodes.Get(1), nodes.Get(2));
    auto right = edge.Install(nodes.Get(2), nodes.Get(3));
    InternetStackHelper internet;
    internet.Install(nodes);
    // Install FIFOs before IP address assignment can install default queue discs.
    TrafficControlHelper accessTc;
    accessTc.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("1000p")));
    accessTc.Install(left);
    accessTc.Install(right);
    TrafficControlHelper coreTc;
    // 1 BDP = 10 Mbps * 50 ms / 8 = 62,500 bytes.
    coreTc.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize("62500B")));
    auto queues = coreTc.Install(middle);
    queueDisc = queues.Get(0); // Forward bottleneck queue, R1 -> R2.

    Ipv4AddressHelper addresses;
    addresses.SetBase("10.1.1.0", "255.255.255.0");
    addresses.Assign(left);
    addresses.NewNetwork();
    addresses.Assign(middle);
    addresses.NewNetwork();
    auto receiverAddresses = addresses.Assign(right);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 50001;
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    auto sinkApps = sinkHelper.Install(nodes.Get(3));
    sink = DynamicCast<PacketSink>(sinkApps.Get(0));
    sinkApps.Start(Seconds(0));
    sinkApps.Stop(Seconds(stopTime + drainTime));
    BulkSendHelper sender("ns3::TcpSocketFactory",
                          InetSocketAddress(receiverAddresses.GetAddress(1), port));
    sender.SetAttribute("MaxBytes", UintegerValue(0));
    auto senderApps = sender.Install(nodes.Get(0));
    senderApps.Start(Seconds(startTime));
    senderApps.Stop(Seconds(stopTime));
    // Socket creation occurs in the application start event.
    Simulator::Schedule(Seconds(startTime) + TimeStep(1), &AttachTraces,
                        DynamicCast<BulkSendApplication>(senderApps.Get(0)));
    Simulator::ScheduleNow(&SampleQueue);
    Simulator::Schedule(Seconds(startTime + sampleInterval), &SampleThroughput);
    std::cout << "Results: " << output << '\n';
    Simulator::Stop(Seconds(stopTime + drainTime));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
