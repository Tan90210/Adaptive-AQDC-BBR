/* SPDX-License-Identifier: GPL-2.0-only */
// Week 1: stock BBRv1; results/bbr-baseline-vN/ contains four .dat files.
#include "baseline-experiment.h"

int
main(int argc, char* argv[])
{
    return ns3::aqdc_project::RunBaseline(argc, argv, ns3::TcpBbr::GetTypeId(), "bbr-baseline");
}
