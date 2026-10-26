/* SPDX-License-Identifier: GPL-2.0-only */
#include "tcp-aqdc-fixed.h"
#include "ns3/double.h"
#include <cmath>
namespace ns3
{
NS_OBJECT_ENSURE_REGISTERED(TcpAqdcFixed);
TypeId TcpAqdcFixed::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpAqdcFixed")
        .SetParent<TcpAqdcBbr>().SetGroupName("Internet")
        .AddConstructor<TcpAqdcFixed>()
        .AddAttribute("CompensationCoefficient", "Positive constant multiplier of estimated debt",
                      DoubleValue(1.0),
                      MakeDoubleAccessor(&TcpAqdcFixed::SetCompensationCoefficient,
                                         &TcpAqdcFixed::GetCompensationCoefficient),
                      MakeDoubleChecker<double>());
    return tid;
}
TcpAqdcFixed::TcpAqdcFixed(const TcpAqdcFixed& other)
    : TcpAqdcBbr(other), m_coefficient(other.m_coefficient) {}
std::string
TcpAqdcFixed::GetName() const
{
    return "TcpAqdcFixed";
}
Ptr<TcpCongestionOps>
TcpAqdcFixed::Fork()
{
    return CopyObject<TcpAqdcFixed>(this);
}
void TcpAqdcFixed::SetCompensationCoefficient(double coefficient)
{
    NS_ABORT_MSG_IF(!std::isfinite(coefficient) || coefficient <= 0,
                    "CompensationCoefficient must be finite and positive");
    m_coefficient = coefficient;
}
double
TcpAqdcFixed::GetCompensationCoefficient() const
{
    return m_coefficient;
}
uint32_t TcpAqdcFixed::CalculateTarget(DataRate bw, Time rtt, uint32_t mss) const
{
    return CalculateScaledTarget(bw, rtt, mss, m_coefficient);
}
}
