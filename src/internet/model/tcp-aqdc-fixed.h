/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef TCP_AQDC_FIXED_H
#define TCP_AQDC_FIXED_H
#include "tcp-aqdc-bbr.h"
namespace ns3
{
/** Stage 1 ablation: constant debt multiplier, no adaptive update. */
class TcpAqdcFixed : public TcpAqdcBbr
{
  public:
    static TypeId GetTypeId();
    TcpAqdcFixed() = default;
    TcpAqdcFixed(const TcpAqdcFixed& other);
    std::string GetName() const override;
    Ptr<TcpCongestionOps> Fork() override;
    void SetCompensationCoefficient(double coefficient);
    double GetCompensationCoefficient() const;
  protected:
    uint32_t CalculateTarget(DataRate bandwidth, Time minRtt, uint32_t segmentSize) const override;
  private:
    double m_coefficient{1.0};
};
}
#endif
