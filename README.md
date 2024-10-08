# Ensō

[![docs](https://github.com/crossroadsfpga/enso/actions/workflows/docs.yml/badge.svg)](https://github.com/crossroadsfpga/enso/actions/workflows/docs.yml)
[![DOI](https://zenodo.org/badge/248301431.svg)](https://zenodo.org/badge/latestdoi/248301431)

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/crossroadsfpga/enso/master/docs/assets/enso-white.svg">
  <source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/crossroadsfpga/enso/master/docs/assets/enso-black.svg">
  <img align="right" width="200" alt="Enso" src="./docs/assets/enso-black.svg">
</picture>

Ensō is a high-performance streaming interface for NIC-application communication.

Ensō's design encompasses both *hardware* and *software*. The hardware component targets an FPGA NIC[^1] and implements the Ensō interface. The software component uses this interface and exposes simple communication primitives called [Ensō Pipes](https://enso.cs.cmu.edu/primitives/rx_enso_pipe/). Applications can use Ensō Pipes to send and receive data in different formats, such as raw packets, application-level messages, or TCP-like byte streams.

Refer to the [OSDI '23 paper](https://www.usenix.org/conference/osdi23/presentation/sadok) for details about the design and to the [documentation](https://enso.cs.cmu.edu/) to find out how to use Ensō for your own projects.

[^1]: Network Interface Cards (NICs) are the hardware devices that connect a computer to the network. They are responsible for transmitting data from the CPU to the network and vice versa. FPGAs are reconfigurable hardware devices. They can be reconfigured to implement arbitrary hardware designs. Here we use an FPGA to implement a NIC with the Ensō interface but the same interface could also be implemented in a traditional fixed-function hardware.


## Why Ensō?

Traditionally, NICs expose a *packetized* interface that software (applications or the kernel) must use to communicate with the NIC. Ensō provides two main advantages over this interface:

- **Flexibility:** While NICs were traditionally in charge of delivering raw packets to software, an increasing amount of high-level functionality is now performed on the NIC. The packetized interface, however, forces data to be fragmented into packets that are then scattered across memory. This prevents the NIC and the application from communicating efficiently using higher-level abstractions such as application-level messages or TCP streams. Ensō instead allows the NIC and the application to communicate using a contiguous stream of bytes, which can be used to represent *arbitrary* data.
- **Performance:** By forcing hardware and software to synchronize buffers for every packet, the packetized interface imposes significant per-packet overhead both in terms of CPU cycles as well as PCIe bandwidth. This results in significant performance degradation, in particular when using small requests. Ensō's use of a byte stream interface allows the NIC and the application to exchange multiple packets (or messages) at once, which reduces the number of CPU cycles and PCIe transactions required to communicate each request. Moreover, by placing packets (or messages) contiguously in memory, Ensō makes better use of the CPU prefetcher, vastly reducing the number of cache misses.


## Getting started

- [Setup](https://enso.cs.cmu.edu/getting_started/)
- Understanding the primitives: [RX Ensō Pipe](https://enso.cs.cmu.edu/primitives/rx_enso_pipe/), [TX Ensō Pipe](https://enso.cs.cmu.edu/primitives/tx_enso_pipe/), [RX/TX Ensō Pipe](https://enso.cs.cmu.edu/primitives/rx_tx_enso_pipe/)
- Examples: [Echo Server](https://github.com/crossroadsfpga/enso/blob/master/software/examples/echo.cpp), [Packet Capture](https://github.com/crossroadsfpga/enso/blob/master/software/examples/capture.cpp), [EnsōGen Packet Generator](https://github.com/crossroadsfpga/enso/blob/master/software/examples/ensogen.cpp)
- API References: [Software](https://enso.cs.cmu.edu/software/), [Hardware](https://enso.cs.cmu.edu/hardware/)

## Hermes Additions
In order to accommodate [Hermes](https://github.com/kaajalbgupta/hermes), some modifications were made to the Enso interface.

### Hybrid Backend
The hybrid backend is introduced here: in which Enso Pipes and notification buffers are split in communication. Applications running with the hybrid backend will only receive data in their Enso Pipes from the NIC, while the notifications will go to the [IOKernel](https://github.com/kaajalbgupta/shinkansen_sw). This communication is set up by having applications access the notification buffer ID of the IOKernel and using it when registering their Enso Pipes with the NIC in the backend.

This backend can be set up as follows:

```
cd enso/
meson setup --native-file gcc.ini -Ddev_backend=hybrid build_hybrid
cd build_hybrid/
sudo ninja install
```

### Callbacks
As Hermes uses Enso as a dependency, for Hermes to make decisions in the Enso codebase itself, a few callbacks were added that could use internal Enso information in Hermes.

### Ensogen
A few new options were incorporated in Ensogen to accommodate the Poisson scheduling of packets in a PCAP file and to include information on the number of cycles to spin for each packet.

To get the poisson bitstream, run:
```
sudo -i
./enso/scripts/update_bitstream.sh /home/kaajalg/poisson.sof
```

Then, must load the machine with that bitstream: `enso enso/ --host mxhost --fpga 1-12`. The correct sha256 for this bitstream is `2f12a0862f51c2ca5c293216bfe46de60db7f27523ef3ee9114286d0ecbab2b7`.

An example command incorporating some new features is:
```
sudo enso/build/software/examples/ensogen enso/frontend/pcaps/64_1_100_0_1200000_100000.pcap 1 175 --core 0  --queues 4 --save stats.csv --pcie-addr 0000:65:00.0 --rtt-hist hist --distribution constant --poisson
```
The distribution option specifies the distribution of the number of cycles to spin for per packet provided to the receiving thread. This information is kept at the start of each packet (at offset 0). The distribution can be either constant, exponential, or bimodal. The constant option is simply 10 us per packet. The exponential option is the exponential distribution with a mean of 10 us. The bimodal option is 55 us 10% of the time, and 5 us the rest of the time.

When the poisson option is enabled, per-packet rate limiting occurs instead of per-device. When per-packet rate limiting is enabled, the NIC uses the delay in each packet (which is present at the offset kPacketRttOffset) to decide when to send the packet. We can generate PCAP files that have packets with the correct delays for a certain intended packet rate with `./hardware/input_gen/generate_synthetic_trace`. An example usage of it that creates the PCAP above (64_1_100_0_1200000_100000):
```
build/hardware/input_gen/generate_synthetic_trace 64 1 100 0 --output-pcap frontend/pcaps/64_1_100_0_1200000_100000.pcap --request-rate 1200000 --request-rate 100000
```
This creates a PCAP file with packet size 64 bytes, with one source IP, a hundred destination IPs, and has two request rates (that will alternate between each other in 1 second intervals): 1200 kpps and 100 kpps. To create a PCAP that only has a single request rate, only one request rate need be specified. 
