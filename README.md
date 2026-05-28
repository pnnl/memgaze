<!-- -*-Mode: markdown;-*- -->
<!-- $Id$ -->

MemGaze
=============================================================================

**Home**:
  - https://github.com/pnnl/memgaze

  - [Performance Lab for EXtreme Computing and daTa](https://github.com/perflab-exact)


**About**: As memory systems are the primary bottleneck in many
workloads, effective hardware/software co-design requires a detailed
understanding of memory behavior. Unfortunately, current analysis of
word-level sequences of memory accesses incurs time slowdowns of
O(100×).

MemGaze is a memory analysis toolset that combines high-resolution
trace analysis and low overhead measurement, both with respect to time
and space.

MemGaze provides high-resolution by collecting world-level memory
access traces, where the highest resolution supported is back-to-back
sequences. In particular, it leverages emerging Processor Tracing
support to collect data. It achieves low-overhead in space and time by
leveraging sampling and various methods of hardware support for
collecting traces.

MemGaze provides several post-mortem trace processing methods,
including multi-resolution analysis for locations vs. operations;
accesses vs. spatio-temporal reuse, and reuse (distance, rate, volume)
vs. access patterns.

Memgaze now includes MemFriend, a new analysis module that introduces
spatial and temporal locality analysis that captures affinity (access
correlation) between pairs of memory locations. MemFriend's
multi-resolution analysis identifies significant memory segments and
simultaneously prunes the analysis space such that time and space
complexity is modest. MemFriend creates signatures, selectable at 3D,
2D, and 1D resolutions, that provide novel insights and enable
predictive reasoning about application performance. The results aid
data layout optimizations, and data placement decisions.


**Contacts**: (_firstname_._lastname_@pnnl.gov)
  - Nathan R. Tallent ([www](https://hpc.pnnl.gov/people/tallent)), ([www](https://www.pnnl.gov/people/nathan-tallent))

**Contributors**:
  - Nathan R. Tallent ([www](https://nathantallent.github.io))
  - Yasodha Suriyakumar (Portland State University)
  - Venkata Challa ([www](https://www.researchgate.net/scientific-contributions/Prajwal-Challa-2231777668)) (U. Texas, Arlington)
  - Andrés Marquez (PNNL)
  - Ozgur Kilic (Now BNL)
  - Onur Cankur (University of Maryland)
  - Chenhao Xie (PNNL)
  - Stephane Eranian (Google)


<!--
There is a separation between trace collection and analysis. The trace format is very simple.

To collect high-resolution low-overhead traces, we need a processor tracing support. The current implementation is for Intel.

One can collect traces from GPUs/CPUs using slow methods and write to the simple format. We are developing a generic method using PIN (MEMPRINT: Constructing Program Memory Footprint Estimations using statistical methods from Sparsely Sampled Pin-based Memory Traces).

The analyses then consume the generic format.
-->


References
-----------------------------------------------------------------------------

* Dhruv Gajaria, Prajwal Challa, Yasodha Suriyakuma, Jospeh Manzano, Nathan Tallent, and Andrés Márquez. "An Integrated Framework for Memory-Centric Analysis: From Trace Collection to Co-Design." Frontiers in High Performance Computing, Vol. 4-2026, 2026. ([doi: 10.3389/fhpcp.2026.1801169](https://doi.org/10.3389/fhpcp.2026.1801169))

* Yasodha Suriyakumar, Nathan R. Tallent, Andrés Marquez, and Karen Karavanic. "MemFriend: Understanding Memory Performance with Spatial-Temporal Affinity," Proc. of the International Symposium on Memory Systems (MemSys 2024), MemSys '24, ACM, September 2024 ([doi: 10.1145/3695794.3695820](https://doi.org/10.1145/3695794.3695820))

* Ozgur O. Kilic, Nathan R. Tallent, Yasodhadevi Suriyakumar, Chenhao Xie, Andrés Marquez, and Stephane Eranian. "MemGaze: Rapid and Effective Load-Level Memory and Data Analysis." Proc. of the 2022 IEEE Conf. on Cluster Computing, CLUSTER '22, IEEE, September 2022. ([doi: 10.1109/CLUSTER51413.2022.00058](https://doi.org/10.1109/CLUSTER51413.2022.00058))

* Ozgur O. Kilic, Nathan R. Tallent, and Ryan D. Friese. "Rapid Memory Footprint Access Diagnostics" Proc. of the 2020 IEEE Intl. Symp. on Performance Analysis of Systems and Software, ISPASS '20, pp. 273-284, IEEE Computer Society, October 2020. ([doi: 10.1109/ISPASS48437.2020.00047](https://doi.org/10.1109/ISPASS48437.2020.00047))

* Ozgur O. Kilic, Nathan R. Tallent, and Ryan D. Friese. "Rapidly Measuring Loop Footprints." Proc. of IEEE Intl. Conf. on Cluster Computing (Workshop on Monitoring and Analysis for High Performance Computing Systems Plus Applications), CLUSTER Workshops '19, pp. 1-9, IEEE Computer Society, September 2019. ([doi: 10.1109/CLUSTER.2019.8891025](https://doi.org/10.1109/CLUSTER.2019.8891025))


Acknowledgements
-----------------------------------------------------------------------------

This work was supported by the U.S. Department of Energy's Office of
Advanced Scientific Computing Research:

- Advanced Memory to Support Artificial Intelligence for Science

- Orchestration for Distributed & Data-Intensive Scientific Exploration


