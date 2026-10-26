/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef TCP_AQDC_BBR_H
#define TCP_AQDC_BBR_H

#include "tcp-bbr.h"

#include "ns3/traced-callback.h"

namespace ns3
{

/**
 * @ingroup congestionOps
 * @brief Course-project reconstruction of AQDC for native ns-3 BBRv1.
 *
 * Li et al., IET Communications (2026), doi:10.1049/cmu2.70141,
 * Algorithm 1 and Sections 3.3--3.4. Debt and targets are in bytes.
 * Pacing, bandwidth filtering, Startup, Drain and ProbeBW remain native BBR.
 * See doc/aqdc-week2.md for the explicit interpretation choices.
 */
class TcpAqdcBbr : public TcpBbr
{
  public:
    static TypeId GetTypeId();
    TcpAqdcBbr() = default;
    TcpAqdcBbr(const TcpAqdcBbr& other);

    std::string GetName() const override;
    Ptr<TcpCongestionOps> Fork() override;

    void UpdateBottleneckBandwidth(Ptr<TcpSocketState> tcb,
                                   const TcpRateOps::TcpRateSample& sample) override;
    void HandleProbeRTT(Ptr<TcpSocketState> tcb) override;
    void ModulateCwndForProbeRTT(Ptr<TcpSocketState> tcb) override;
    void ExitProbeRTT() override;
    void CongestionStateSet(Ptr<TcpSocketState> tcb,
                            TcpSocketState::TcpCongState_t state) override;
    void CwndEvent(Ptr<TcpSocketState> tcb, TcpSocketState::TcpCAEvent_t event) override;

    double GetQueueDebt() const;
    uint32_t GetCompensationTarget() const;
    bool IsCompensating() const;

    /** Exact lifecycle events: 0 = entry, 1 = successful completion. */
    typedef void (*CompensationCallback)(uint8_t event,
                                         double debtBytes,
                                         uint32_t targetBytes,
                                         uint32_t inflightBytes);

  protected:
    /** Observe the filtered bandwidth, not a raw per-ACK delivery-rate sample. */
    void ObserveBandwidth(DataRate bandwidth, Time minRtt, bool probing);
    /** One-to-one target; the isolated extension point for Week 3's repayment rule. */
    virtual uint32_t CalculateTarget(DataRate bandwidth, Time minRtt, uint32_t segmentSize) const;

    uint32_t CalculateScaledTarget(DataRate bandwidth, Time minRtt, uint32_t segmentSize,
                                   double coefficient) const;

  private:
    DataRate m_previousBandwidth{0};
    bool m_havePreviousBandwidth{false};
    TracedValue<double> m_queueDebt{0.0};
    TracedValue<uint32_t> m_compensationTarget{0};
    bool m_compensating{false};
    TracedCallback<uint8_t, double, uint32_t, uint32_t> m_compensation;
};

} // namespace ns3
#endif // TCP_AQDC_BBR_H
