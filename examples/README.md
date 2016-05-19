# ccns3Examples

## acme-forwarder.cc

An example of substituting a different forwarder instead of CCNxStandardForwarder.

## Topology-driven simulations

- topo.txt : An AT&T topology
- large-consumer-producer.cc: 

### parc-acm-icn-2016

Files used to generate simulations for the PARC ACM/ICN 2016 paper on
ccns3Sim.

- parc-acm-icn-2016.awk: used to analyze statistics
- parc-acm-icn-2016.cc: Runs the simulation.  See ./waf --run "parc-acm-icn-2016 --help"
- parc-acm-icn-2016.plt: gnuplot script
- parc-acm-icn-2016.sh: script to run the variations of command-line parameters

