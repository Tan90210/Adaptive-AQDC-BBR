/* SPDX-License-Identifier: GPL-2.0-only */
#include "ns3/tcp-aqdc-bbr.h"
#include "ns3/tcp-aqdc-fixed.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/tcp-rate-ops.h"
#include "ns3/test.h"

#include <limits>

namespace ns3
{

// Expose the small arithmetic helpers, not writable BBR internals.
class AqdcTestController : public TcpAqdcBbr
{
  public:
    using TcpAqdcBbr::CalculateTarget;
    using TcpAqdcBbr::ObserveBandwidth;
};

class AqdcDebtTest : public TestCase
{
  public:
    AqdcDebtTest()
        : TestCase("Debt uses positive filtered-bandwidth increments, byte units and valid history")
    {
    }

  private:
    void DoRun() override
    {
        auto cc = CreateObject<AqdcTestController>();
        cc->ObserveBandwidth(DataRate("4Mbps"), MilliSeconds(100), true);
        NS_TEST_ASSERT_MSG_EQ(cc->GetQueueDebt(), 0, "First observation must seed the reference");
        cc->ObserveBandwidth(DataRate("8Mbps"), MilliSeconds(100), true);
        NS_TEST_ASSERT_MSG_EQ_TOL(cc->GetQueueDebt(), 50000.0, 1e-9,
                                  "4 Mb/s * 0.1 s / 8 must be 50,000 bytes");
        cc->ObserveBandwidth(DataRate("6Mbps"), MilliSeconds(100), true);
        NS_TEST_ASSERT_MSG_EQ(cc->GetQueueDebt(), 50000, "A decrease does not repay debt");
        cc->ObserveBandwidth(DataRate("8Mbps"), MilliSeconds(100), true);
        cc->ObserveBandwidth(DataRate("8Mbps"), MilliSeconds(100), true);
        NS_TEST_ASSERT_MSG_EQ(cc->GetQueueDebt(), 75000, "Count the new increase exactly once");

        auto invalid = CreateObject<AqdcTestController>();
        invalid->ObserveBandwidth(DataRate("4Mbps"), Time::Max(), true);
        invalid->ObserveBandwidth(DataRate("8Mbps"), Seconds(0), true);
        invalid->ObserveBandwidth(DataRate("8Mbps"), MilliSeconds(100), true);
        NS_TEST_ASSERT_MSG_EQ(invalid->GetQueueDebt(), 0, "Invalid RTT must not seed false debt");
        invalid->ObserveBandwidth(DataRate("12Mbps"), MilliSeconds(100), false);
        NS_TEST_ASSERT_MSG_EQ(invalid->GetQueueDebt(), 0, "Startup/Drain is not charged");
        invalid->ObserveBandwidth(DataRate("16Mbps"), MilliSeconds(100), true);
        NS_TEST_ASSERT_MSG_EQ(invalid->GetQueueDebt(), 50000, "Resume with the current reference");
        Simulator::Destroy();
    }
};

class AqdcTargetTest : public TestCase
{
  public:
    AqdcTargetTest()
        : TestCase("One-to-one target has a four-MSS floor and cannot underflow or overflow")
    {
    }

  private:
    void DoRun() override
    {
        auto cc = CreateObject<AqdcTestController>();
        NS_TEST_ASSERT_MSG_EQ(cc->CalculateTarget(DataRate("8Mbps"), MilliSeconds(100), 1000),
                               100000, "Zero debt gives one BDP");
        cc->ObserveBandwidth(DataRate("4Mbps"), MilliSeconds(100), true);
        cc->ObserveBandwidth(DataRate("8Mbps"), MilliSeconds(100), true);
        NS_TEST_ASSERT_MSG_EQ(cc->CalculateTarget(DataRate("8Mbps"), MilliSeconds(100), 1000),
                               50000, "Repay 50 KB from a 100 KB BDP");
        NS_TEST_ASSERT_MSG_EQ(cc->CalculateTarget(DataRate("1Mbps"), MilliSeconds(100), 1000),
                               4000, "Debt larger than BDP must not wrap unsigned arithmetic");
        NS_TEST_ASSERT_MSG_EQ(cc->CalculateTarget(DataRate(0), Time::Max(), 1000),
                               4000, "Uninitialized path model uses the safe native minimum");
        NS_TEST_ASSERT_MSG_EQ(
            cc->CalculateTarget(DataRate(std::numeric_limits<uint64_t>::max()), Seconds(1), 1000),
            std::numeric_limits<uint32_t>::max(), "Large estimates must saturate the window");
        Simulator::Destroy();
    }
};

struct AqdcFixture
{
    Ptr<TcpSocketState> state = CreateObject<TcpSocketState>();
    Ptr<TcpAqdcBbr> cc = CreateObject<TcpAqdcBbr>();
    TcpRateOps::TcpRateSample sample;

    AqdcFixture()
    {
        state->m_segmentSize = 1000;
        state->m_initialCWnd = 10;
        state->m_cWnd = 10000;
        state->m_srtt = MilliSeconds(100);
        state->m_minRtt = MilliSeconds(100);
        state->m_lastRtt = MilliSeconds(100);
        cc->SetRateOps(CreateObject<TcpRateLinux>());
        cc->CongestionStateSet(state, TcpSocketState::CA_OPEN);
        sample.m_interval = MilliSeconds(100);
        sample.m_delivered = 1000;
        sample.m_ackedSacked = 1000;
        sample.m_deliveryRate = DataRate("4Mbps");
        cc->UpdateBottleneckBandwidth(state, sample); // Startup reference only.
        cc->EnterProbeBW();
    }

    void AddDebt()
    {
        sample.m_deliveryRate = DataRate("8Mbps");
        cc->UpdateBottleneckBandwidth(state, sample);
    }

    void StartCompensation(uint32_t inflight)
    {
        state->m_cWnd = 200000;
        state->m_bytesInFlight = inflight;
        cc->SaveCwnd(state);
        cc->EnterProbeRTT();
        cc->HandleProbeRTT(state);
    }
};

class AqdcFilteredUpdateTest : public TestCase
{
  public:
    AqdcFilteredUpdateTest()
        : TestCase("Rate pipeline charges filtered estimate increases rather than raw ACK slopes")
    {
    }

  private:
    void DoRun() override
    {
        AqdcFixture f;
        f.AddDebt();
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetQueueDebt(), 50000, "Positive probe must add debt");
        f.sample.m_deliveryRate = DataRate("4Mbps");
        f.cc->UpdateBottleneckBandwidth(f.state, f.sample);
        f.sample.m_deliveryRate = DataRate("6Mbps");
        f.cc->UpdateBottleneckBandwidth(f.state, f.sample);
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetQueueDebt(), 50000,
                               "4 -> 6 raw Mb/s does not change the 8 Mb/s max filter");
        f.sample.m_delivered = -1;
        f.sample.m_deliveryRate = DataRate("12Mbps");
        f.cc->UpdateBottleneckBandwidth(f.state, f.sample);
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetQueueDebt(), 50000, "Invalid samples must not add debt");
        f.sample.m_delivered = 1000;
        f.sample.m_interval = Seconds(0);
        f.cc->UpdateBottleneckBandwidth(f.state, f.sample);
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetQueueDebt(), 50000, "Zero-interval samples are invalid");
        Simulator::Destroy();
    }
};

class AqdcLifecycleTest : public TestCase
{
  public:
    AqdcLifecycleTest()
        : TestCase("Native window update enforces target; actual inflight controls exit and reset")
    {
    }

  private:
    uint32_t m_entries{0};
    uint32_t m_completions{0};

    void Event(uint8_t event, double debt, uint32_t target, uint32_t inflight)
    {
        if (event == 0)
        {
            ++m_entries;
        }
        else
        {
            ++m_completions;
            NS_TEST_ASSERT_MSG_EQ((inflight <= target), true,
                                   "Cannot complete until actual outstanding bytes reach target");
        }
        NS_TEST_ASSERT_MSG_EQ(debt, 50000, "Event must expose the debt being compensated");
    }

    void DoRun() override
    {
        AqdcFixture f;
        f.AddDebt();
        f.cc->TraceConnectWithoutContext("Compensation", MakeCallback(&AqdcLifecycleTest::Event, this));
        f.StartCompensation(120000);
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetCompensationTarget(), 50000, "Wrong entry target");
        NS_TEST_ASSERT_MSG_EQ(f.cc->IsCompensating(), true, "Must remain active above target");
        f.sample.m_deliveryRate = DataRate("12Mbps");
        f.cc->UpdateBottleneckBandwidth(f.state, f.sample);
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetQueueDebt(), 50000, "ProbeRTT must not accumulate new debt");
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetCompensationTarget(), 50000,
                               "Model updates must not move the entry snapshot");
        f.cc->SetCwnd(f.state, f.sample);
        NS_TEST_ASSERT_MSG_EQ(f.state->m_cWnd.Get(), 50000, "Native update must not overwrite cap");
        f.state->m_cWnd = 150000;
        f.sample.m_ackedSacked = 0;
        f.cc->SetCwnd(f.state, f.sample);
        NS_TEST_ASSERT_MSG_EQ(f.state->m_cWnd.Get(), 50000, "Zero-ACK path must also enforce cap");
        f.cc->HandleProbeRTT(f.state);
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetQueueDebt(), 50000, "Waiting must not silently clear debt");
        f.state->m_congState = TcpSocketState::CA_RECOVERY;
        f.cc->CongestionStateSet(f.state, TcpSocketState::CA_RECOVERY);
        NS_TEST_ASSERT_MSG_EQ(f.state->m_cWnd.Get(), 50000, "Recovery entry must enforce cap");
        f.sample.m_ackedSacked = 1000;
        f.cc->SetCwnd(f.state, f.sample);
        NS_TEST_ASSERT_MSG_EQ(f.state->m_cWnd.Get(), 50000, "Recovery must not bypass AQDC cap");
        f.cc->CwndEvent(f.state, TcpSocketState::CA_EVENT_COMPLETE_CWR);
        NS_TEST_ASSERT_MSG_EQ(f.state->m_cWnd.Get(), 50000, "Recovery exit must not restore above cap");
        f.state->m_bytesInFlight = 50000;
        f.cc->HandleProbeRTT(f.state);
        NS_TEST_ASSERT_MSG_EQ(f.cc->IsCompensating(), false, "Target reached: finish immediately");
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetQueueDebt(), 0, "Reset debt on completion");
        NS_TEST_ASSERT_MSG_EQ((f.cc->GetBbrState() != TcpBbr::BBR_PROBE_RTT), true,
                               "Restore native Startup/ProbeBW state");
        NS_TEST_ASSERT_MSG_EQ(f.state->m_cWnd.Get(), 200000, "Restore the saved congestion window");
        NS_TEST_ASSERT_MSG_EQ(m_entries, 1, "Exactly one entry event");
        NS_TEST_ASSERT_MSG_EQ(m_completions, 1, "Exactly one completion event");
        Simulator::Destroy();
    }
};

class AqdcZeroDebtAndForkTest : public TestCase
{
  public:
    AqdcZeroDebtAndForkTest()
        : TestCase("Zero debt, idle inflight, repeat episodes and independent fork state")
    {
    }

  private:
    void DoRun() override
    {
        AqdcFixture zero;
        zero.StartCompensation(0);
        NS_TEST_ASSERT_MSG_EQ(zero.cc->IsCompensating(), false,
                               "Idle, zero-debt connection must not wait for a timer");
        NS_TEST_ASSERT_MSG_EQ(zero.cc->GetCompensationTarget(), 50000, "Zero debt target is BDP");
        AqdcFixture f;
        f.AddDebt();
        f.StartCompensation(120000);
        auto fork = DynamicCast<TcpAqdcBbr>(f.cc->Fork());
        NS_TEST_ASSERT_MSG_NE(fork, nullptr, "Fork must preserve controller type");
        NS_TEST_ASSERT_MSG_EQ(fork->GetQueueDebt(), 0, "New socket must not inherit parent's debt");
        NS_TEST_ASSERT_MSG_EQ(fork->IsCompensating(), false, "New socket is not repaying parent debt");
        NS_TEST_ASSERT_MSG_EQ(fork->GetCompensationTarget(), 0, "No stale target on a new socket");
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetQueueDebt(), 50000, "Fork must leave parent unchanged");
        f.state->m_bytesInFlight = 0; // App-limited sender drains completely.
        f.cc->HandleProbeRTT(f.state);
        f.StartCompensation(0);
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetCompensationTarget(), 100000, "Recompute target for new episode");
        NS_TEST_ASSERT_MSG_EQ(f.cc->IsCompensating(), false, "Repeated zero-debt episode can finish");
        Simulator::Destroy();
    }
};

class AqdcSlowDrainTest : public TestCase
{
  public:
    AqdcSlowDrainTest()
        : TestCase("Slow drainage does not time out at 200 ms; completion depends on inflight")
    {
    }

  private:
    void DoRun() override
    {
        AqdcFixture f;
        f.AddDebt();
        f.StartCompensation(120000);
        Simulator::Schedule(Seconds(1), [&] {
            f.cc->HandleProbeRTT(f.state);
            NS_TEST_ASSERT_MSG_EQ(f.cc->IsCompensating(), true,
                                   "Elapsed time alone must not clear unpaid debt");
            NS_TEST_ASSERT_MSG_EQ(f.cc->GetQueueDebt(), 50000, "Debt persists while above target");
            f.state->m_bytesInFlight = 50000;
            f.cc->HandleProbeRTT(f.state);
            NS_TEST_ASSERT_MSG_EQ(f.cc->IsCompensating(), false,
                                   "Reaching the target finishes without additional delay");
        });
        Simulator::Run();
        Simulator::Destroy();
    }
};

class FixedTestController : public TcpAqdcFixed
{
  public:
    using TcpAqdcBbr::ObserveBandwidth;
    using TcpAqdcFixed::CalculateTarget;
};
class AqdcFixedTest : public TestCase
{
  public:
    AqdcFixedTest() : TestCase("Fixed coefficient arithmetic, floor, fork and episode snapshot") {}
  private:
    void DoRun() override
    {
        auto cc = CreateObject<FixedTestController>();
        cc->ObserveBandwidth(DataRate("6.4Mbps"), MilliSeconds(100), true);
        cc->ObserveBandwidth(DataRate("8Mbps"), MilliSeconds(100), true);
        const double coefficients[] = {0.5, 1.0, 1.5, 2.0};
        const uint32_t expected[] = {90000, 80000, 70000, 60000};
        for (unsigned i = 0; i < 4; ++i)
        {
            cc->SetAttribute("CompensationCoefficient", DoubleValue(coefficients[i]));
            NS_TEST_ASSERT_MSG_EQ(cc->CalculateTarget(DataRate("8Mbps"), MilliSeconds(100), 1448),
                                  expected[i], "Scaled byte target incorrect");
        }
        NS_TEST_ASSERT_MSG_EQ(cc->CalculateTarget(DataRate("1Mbps"), MilliSeconds(100), 1448),
                              5792, "Must apply four-MSS floor without unsigned wrap");
        auto fork = DynamicCast<TcpAqdcFixed>(cc->Fork());
        NS_TEST_ASSERT_MSG_NE(fork, nullptr, "Fork preserves fixed controller type");
        NS_TEST_ASSERT_MSG_EQ(fork->GetCompensationCoefficient(), 2.0, "Fork retains configuration");
        NS_TEST_ASSERT_MSG_EQ(fork->GetQueueDebt(), 0, "Fork has independent debt history");
        AqdcFixture f;
        f.cc = CreateObject<TcpAqdcFixed>();
        f.cc->SetRateOps(CreateObject<TcpRateLinux>());
        f.cc->CongestionStateSet(f.state, TcpSocketState::CA_OPEN);
        f.cc->UpdateBottleneckBandwidth(f.state, f.sample);
        f.cc->EnterProbeBW();
        f.AddDebt();
        auto fixed = DynamicCast<TcpAqdcFixed>(f.cc);
        fixed->SetCompensationCoefficient(1.5);
        f.StartCompensation(120000);
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetCompensationTarget(), 25000, "Entry target uses coefficient");
        fixed->SetCompensationCoefficient(0.5);
        f.cc->HandleProbeRTT(f.state);
        NS_TEST_ASSERT_MSG_EQ(f.cc->GetCompensationTarget(), 25000, "Active target stays fixed");
        f.state->m_bytesInFlight = 25000;
        f.cc->HandleProbeRTT(f.state);
        NS_TEST_ASSERT_MSG_EQ(f.cc->IsCompensating(), false, "Actual inflight controls completion");
        Simulator::Destroy();
    }
};

class TcpAqdcTestSuite : public TestSuite
{
  public:
    TcpAqdcTestSuite()
        : TestSuite("tcp-aqdc", Type::UNIT)
    {
        AddTestCase(new AqdcFixedTest(), TestCase::Duration::QUICK);
        AddTestCase(new AqdcDebtTest(), TestCase::Duration::QUICK);
        AddTestCase(new AqdcTargetTest(), TestCase::Duration::QUICK);
        AddTestCase(new AqdcFilteredUpdateTest(), TestCase::Duration::QUICK);
        AddTestCase(new AqdcLifecycleTest(), TestCase::Duration::QUICK);
        AddTestCase(new AqdcZeroDebtAndForkTest(), TestCase::Duration::QUICK);
        AddTestCase(new AqdcSlowDrainTest(), TestCase::Duration::QUICK);
    }
};

static TcpAqdcTestSuite g_tcpAqdcTestSuite;
} // namespace ns3
