/* SPDX-License-Identifier: GPL-2.0-only */
// Week 2: original AQDC reproduction on exactly the Week 1 network.
#include "baseline-experiment.h"

int
main(int argc, char* argv[])
{
    return ns3::aqdc_project::RunBaseline(argc, argv, ns3::TcpAqdcBbr::GetTypeId(), "aqdc-baseline");
}
