/* SPDX-License-Identifier: GPL-2.0-only */
// Fixed-coefficient AQDC: fixed-coefficient ablation on the shared baseline topology.
#include "baseline-experiment.h"
int main(int argc, char* argv[])
{
    return ns3::aqdc_project::RunBaseline(argc, argv, ns3::TcpAqdcFixed::GetTypeId(),
                                         "aqdc-fixed", true);
}
