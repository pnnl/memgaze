<!-- -*-Mode: markdown;-*- -->
<!-- $Id$ -->

MemGaze
=============================================================================

**Home**:
  - https://github.com/pnnl/memgaze/memfriend

  - [Performance Lab for EXtreme Computing and daTa](https://github.com/perflab-exact)


**About**: A detailed understanding of data locality is an essential
tool for effective hardware/software co-design. Today's locality
analysis focuses on single memory locations and therefore can fail to
provide sufficient insight into correlations between memory regions
and data structures.

MemFriend, a new analysis module within the
[MemGaze](https://github.com/pnnl/memgaze) framework, introduces
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
  - Nathan R. Tallent (PNNL) ([www](https://hpc.pnnl.gov/people/tallent)), ([www](https://www.pnnl.gov/people/nathan-tallent))
  - Yasodha Suriyakumar (Portland State University)
  - Andrés Marquez (PNNL)


References
-----------------------------------------------------------------------------

* Yasodha Suriyakumar, Nathan R. Tallent, Andrés Marquez, and Karen Karavanic, "MemFriend: Understanding memory performance with spatial-temporal affinity," in Proc. of the International Symposium on Memory Systems (MemSys 2024), September 2024.

* Ozgur O. Kilic, Nathan R. Tallent, Yasodha Suriyakumar, Chenhao Xie, Andrés Marquez, and Stephane Eranian, "MemGaze: Rapid and effective load-level memory and data analysis," in Proc. of the 2022 IEEE Conf. on Cluster Computing, IEEE, Sep 2022.

Acknowledgements
-----------------------------------------------------------------------------

This work was supported by the U.S. Department of Energy's Office of
Advanced Scientific Computing Research:

- Orchestration for Distributed & Data-Intensive Scientific Exploration

- Advanced Memory to Support Artificial Intelligence for Science

