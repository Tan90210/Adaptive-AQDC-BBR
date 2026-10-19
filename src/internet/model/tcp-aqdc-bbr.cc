/* SPDX-License-Identifier: GPL-2.0-only */
#include "tcp-aqdc-bbr.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(TcpAqdcBbr);

TypeId
TcpAqdcBbr::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TcpAqdcBbr")
            .SetParent<TcpBbr>()
            .SetGroupName("Internet")
            .AddConstructor<TcpAqdcBbr>()
            .AddTraceSource("QueueDebt",
                            "Accumulated estimated queue debt in bytes (not measured queue size)",
                            MakeTraceSourceAccessor(&TcpAqdcBbr::m_queueDebt),
                            "ns3::TracedValueCallback::Double")
            .AddTraceSource("CompensationTarget",
                            "Inflight target fixed at the start of each compensation episode",
                            MakeTraceSourceAccessor(&TcpAqdcBbr::m_compensationTarget),
                            "ns3::TracedValueCallback::Uint32")
            .AddTraceSource("Compensation",
                            "Entry/completion event with debt, target and actual inflight bytes",
                            MakeTraceSourceAccessor(&TcpAqdcBbr::m_compensation),
                            "ns3::TcpAqdcBbr::CompensationCallback");
    return tid;
}

TcpAqdcBbr::TcpAqdcBbr(const TcpAqdcBbr& other)
    : TcpBbr(other)
{
    // Forked connections start with their own debt history, target and callbacks.
}

std::string
TcpAqdcBbr::GetName() const
{
    return "TcpAqdcBbr";
}

Ptr<TcpCongestionOps>
TcpAqdcBbr::Fork()
{
    return CopyObject<TcpAqdcBbr>(this);
}

double
TcpAqdcBbr::GetQueueDebt() const
{
    return m_queueDebt.Get();
}

uint32_t
TcpAqdcBbr::GetCompensationTarget() const
{
    return m_compensationTarget.Get();
}

bool
TcpAqdcBbr::IsCompensating() const
{
    return m_compensating;
}

void
TcpAqdcBbr::ObserveBandwidth(DataRate bandwidth, Time minRtt, bool probing)
{
    bool valid = bandwidth.GetBitRate() > 0 && minRtt != Time::Max() &&
                 minRtt.IsStrictlyPositive();
    if (!valid)
    {
        m_havePreviousBandwidth = false;
        return;
    }

    // Seed the reference without charging the first measurement as probing debt.
    // During Startup/Drain/ProbeRTT we track the reference but charge no debt.
    if (m_havePreviousBandwidth && probing && bandwidth > m_previousBandwidth)
    {
        double increment = (bandwidth.GetBitRate() - m_previousBandwidth.GetBitRate()) /
                           8.0 * minRtt.GetSeconds();
        m_queueDebt = m_queueDebt.Get() + increment;
    }
    m_previousBandwidth = bandwidth; // A decrease is not a repayment event.
    m_havePreviousBandwidth = true;
}

void
TcpAqdcBbr::UpdateBottleneckBandwidth(Ptr<TcpSocketState> tcb,
                                    const TcpRateOps::TcpRateSample& sample)
{
    TcpBbr::UpdateBottleneckBandwidth(tcb, sample);
    if (sample.m_delivered < 0 || !sample.m_interval.IsStrictlyPositive())
    {
        return;
    }
    ObserveBandwidth(GetBbrBandwidth(), GetBbrMinRtt(), GetBbrState() == BBR_PROBE_BW);
}

uint32_t
TcpAqdcBbr::CalculateTarget(DataRate bandwidth, Time minRtt, uint32_t segmentSize) const
{
    uint32_t floor = static_cast<uint32_t>(
        std::min<uint64_t>(4ULL * segmentSize, std::numeric_limits<uint32_t>::max()));
    if (minRtt == Time::Max() || !minRtt.IsStrictlyPositive() || bandwidth.GetBitRate() == 0)
    {
        return floor;
    }
    double bdp = bandwidth.GetBitRate() / 8.0 * minRtt.GetSeconds();
    // Subtract in floating point: unsigned BDP - debt could wrap to a huge window.
    double target = std::max<double>(floor, bdp - m_queueDebt.Get());
    return static_cast<uint32_t>(
        std::min<double>(std::floor(target), std::numeric_limits<uint32_t>::max()));
}

void
TcpAqdcBbr::HandleProbeRTT(Ptr<TcpSocketState> tcb)
{
    MarkProbeRttAppLimited(tcb);
    if (!m_compensating)
    {
        m_compensationTarget =
            CalculateTarget(GetBbrBandwidth(), GetBbrMinRtt(), tcb->m_segmentSize);
        m_compensating = true;
        m_compensation(0, m_queueDebt.Get(), m_compensationTarget.Get(),
                       tcb->m_bytesInFlight.Get());
    }

    // Section 3.4's printed inequality names the target on the left. We compare
    // ACTUAL inflight bytes, otherwise target <= BDP - debt is a tautology.
    if (tcb->m_bytesInFlight.Get() <= m_compensationTarget.Get())
    {
        m_compensation(1, m_queueDebt.Get(), m_compensationTarget.Get(),
                       tcb->m_bytesInFlight.Get());
        RefreshMinRttTimestamp();
        RestoreCwnd(tcb);
        ExitProbeRTT();
    }
}

void
TcpAqdcBbr::ModulateCwndForProbeRTT(Ptr<TcpSocketState> tcb)
{
    if (GetBbrState() == BBR_PROBE_RTT && m_compensating)
    {
        // Called LAST by native SetCwnd, including its recovery/zero-ACK paths.
        tcb->m_cWnd = std::min(tcb->m_cWnd.Get(), m_compensationTarget.Get());
    }
}

void
TcpAqdcBbr::ExitProbeRTT()
{
    m_queueDebt = 0.0;
    m_compensating = false;
    // Keep the last bandwidth reference: repayment does not create a new probe.
    TcpBbr::ExitProbeRTT();
}

void
TcpAqdcBbr::CongestionStateSet(Ptr<TcpSocketState> tcb, TcpSocketState::TcpCongState_t state)
{
    TcpBbr::CongestionStateSet(tcb, state);
    // Native recovery entry can assign cwnd outside SetCwnd.
    ModulateCwndForProbeRTT(tcb);
}

void
TcpAqdcBbr::CwndEvent(Ptr<TcpSocketState> tcb, TcpSocketState::TcpCAEvent_t event)
{
    TcpBbr::CwndEvent(tcb, event);
    // COMPLETE_CWR restores cwnd; it must not undo an ongoing compensation cap.
    ModulateCwndForProbeRTT(tcb);
}

} // namespace ns3
