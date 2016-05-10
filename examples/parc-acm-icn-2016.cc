/*
 * Copyright (c) 2016, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL XEROX OR PARC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* ################################################################################
 * #
 * # PATENT NOTICE
 * #
 * # This software is distributed under the BSD 2-clause License (see LICENSE
 * # file).  This BSD License does not make any patent claims and as such, does
 * # not act as a patent grant.  The purpose of this section is for each contributor
 * # to define their intentions with respect to intellectual property.
 * #
 * # Each contributor to this source code is encouraged to state their patent
 * # claims and licensing mechanisms for any contributions made. At the end of
 * # this section contributors may each make their own statements.  Contributor's
 * # claims and grants only apply to the pieces (source code, programs, text,
 * # media, etc) that they have contributed directly to this software.
 * #
 * # There is no guarantee that this section is complete, up to date or accurate. It
 * # is up to the contributors to maintain their section in this file up to date
 * # and up to the user of the software to verify any claims herein.
 * #
 * # Do not remove this header notification.  The contents of this section must be
 * # present in all distributions of the software.  You may only modify your own
 * # intellectual property statements.  Please provide contact information.
 *
 * - Palo Alto Research Center, Inc
 * This software distribution does not grant any rights to patents owned by Palo
 * Alto Research Center, Inc (PARC). Rights to these patents are available via
 * various mechanisms. As of January 2016 PARC has committed to FRAND licensing any
 * intellectual property used by its contributions to this software. You may
 * contact PARC at cipo@parc.com for more information or visit http://www.ccnx.org
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/random-variable-stream.h"

#include "ns3/ccns3Sim-module.h"


using namespace ns3;
using namespace ns3::ccnx;

#define DEBUG 0
#define DEBUG_TRACE 0

static Time _consumerStartTime = Seconds(2);
static Time _consumerStopTime = Seconds(500);

static Time _producerStartTime = Seconds(1);
static Time _producerStopTime = Seconds(500);

static Time _simulationStopTime = Seconds(50);

static Time _linkFailureTime = Seconds(33);
static Time _statsReportTime = Seconds(38);

static Time _helloInterval = Seconds(1);
static Time _neighborTimeout = Seconds(3); // 3 missed hellos

static Time _routeTimeout = Seconds(10); // flood every 10 seconds

static UniformRandomVariable _uniformRandomVariable;

typedef std::vector< NetDeviceContainer > DevicePairsListType;

/*
 * A vector that lists each pair of devices that make up a PPP link.  When we want to
 * fail a link, we pick a random entry from the vector and then fail each device on the link.
 */
static DevicePairsListType _links;


static ApplicationContainer
AssignProducers(CCNxStackHelper &stackHelper, NodeContainer &nodes, int anchorCount, int prefixCount)
{
  ApplicationContainer allApps;

  if (nodes.GetN() < anchorCount) {
      std::cout << "Not enough nodes for the number of anchors" << std::endl;
      return allApps;
  }

  // pick the random anchors
  std::vector< Ptr<Node> > anchors;
  while (anchors.size() < anchorCount) {
      int anchorIndex = random() % nodes.GetN();
      Ptr<Node> node = nodes.Get(anchorIndex);

      std::vector< Ptr<Node> >::iterator i = find(anchors.begin(), anchors.end(), node);
      if (i == anchors.end()) {
	  anchors.push_back(node);
      }
  }

  Ptr <const CCNxName> prefix = Create <CCNxName> ("ccnx:/name=ccnx/name=consumer/name=producer");
  for (int i = 0; i < prefixCount; i++) {
      char buffer[255];
      sprintf(buffer, "ccnx:/name=acm/name=icn/name=%04u", i);
      Ptr <const CCNxName> prefix = Create <CCNxName> (buffer);

      Ptr <CCNxContentRepository> globalContentRepository = Create <CCNxContentRepository> (prefix, 1000, 10000);

      int anchorIndex = random() % anchors.size();
      CCNxProducerHelper producerHelper (globalContentRepository);
      ApplicationContainer producerApps = producerHelper.Install (anchors[anchorIndex]);
      producerApps.Start (_producerStartTime);
      producerApps.Stop (_producerStopTime);
      allApps.Add(producerApps);

      std::cout << "Producer " << i << " assigned to anchor " << anchors[anchorIndex]->GetId() << std::endl;
  }

  return  allApps;
}


typedef struct {
  std::string	srcName;
  std::string	dstName;
  std::string	bw;
  unsigned	metric;
  std::string	delay;
  unsigned	queueLength;
} AdjacencyEntry;

static NodeContainer
ReadFile(std::string inputFileName, CCNxStackHelper &ccnxStack)
{
  NodeContainer nodes;
  NetDeviceContainer devices;

  std::ifstream istream;
  istream.open (inputFileName.c_str ());

  if ( !istream.is_open () )
    {
      std::cout << "Could not open file for reading: " << inputFileName << std::endl;
      return nodes;
    }

  // Store nodes by name
  std::map<std::string, Ptr<Node> > nodeTable;

  std::istringstream lineBuffer;
  std::string line;

  unsigned lineNumber = 0;

  while (!istream.eof ())
    {
      lineNumber++;
      line.clear ();
      lineBuffer.clear ();

      getline (istream,line);
      if (line.size() > 0 && line.c_str()[0] != '#') {
	  lineBuffer.clear ();
	  lineBuffer.str (line);

	  AdjacencyEntry entry;
	  lineBuffer >> entry.srcName;
	  lineBuffer >> entry.dstName;
	  lineBuffer >> entry.bw;
	  lineBuffer >> entry.metric;
	  lineBuffer >> entry.delay;
	  lineBuffer >> entry.queueLength;

//	  std::cout << "link " << entry.srcName << " <-> " << entry.dstName << std::endl;

	  Ptr<Node> src = nodeTable[entry.srcName];
	  if (src == 0) {
	      src = CreateObject<Node>();
	      nodeTable[entry.srcName] = src;
	      nodes.Add(src);
	      std::cout << "Add node " << entry.srcName << " id " << src->GetId() << std::endl;
	  }

	  Ptr<Node> dst = nodeTable[entry.dstName];
	  if (dst == 0) {
	      dst = CreateObject<Node>();
	      nodeTable[entry.dstName] = dst;
	      nodes.Add(dst);
	      std::cout << "Add node " << entry.dstName << " id " << dst->GetId() << std::endl;
	  }

	  PointToPointHelper ppp;
	  ppp.SetDeviceAttribute ("DataRate", StringValue (entry.bw));
	  ppp.SetChannelAttribute ("Delay", StringValue (entry.delay));
	  ppp.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(entry.queueLength));

	  NetDeviceContainer devpair = ppp.Install (src, dst);
	  devices.Add(devpair);
	  _links.push_back(devpair);

	  std::cout << "linkid " << _links.size() -1 << " : " << entry.srcName << " <-> " << entry.dstName << std::endl;

      }
    }

  ccnxStack.Install (nodes);
  ccnxStack.AddInterfaces (devices);

  return nodes;
}

static void
Progress(Time interval)
{
  ns3::LogTimePrinter timePrinter = ns3::LogGetTimePrinter ();
  (*timePrinter)(std::cout);
  std::cout << " Progress Report" << std::endl;
  Simulator::Schedule (interval, &Progress, interval);
}

static void
ReportStats(Ptr<OutputStreamWrapper> trace, CCNxStandardForwarderHelper &standardHelper)
{
  NodeContainer nodes = NodeContainer::GetGlobal();

  for (int i = 0; i < nodes.GetN(); ++i) {
      Ptr<Node> node = nodes.Get(i);
      standardHelper.PrintForwardingStatistics(trace, node);
      //standardHelper.PrintForwardingTable(trace, node);
  }
}

static void
ReportComputationCost(Ptr<OutputStreamWrapper> trace, NfpRoutingHelper &nfpHelper)
{
  NodeContainer nodes = NodeContainer::GetGlobal();

  for (int i = 0; i < nodes.GetN(); ++i) {
      Ptr<Node> node = nodes.Get(i);
      nfpHelper.PrintComputationCost(trace, node);
  }
}

/*
 * Pick a link between two nodes with at least 2 interfaces each.  Do not fail a singly-connected edge.
 */
static unsigned
PickLinkToFail()
{
  // pick a node, then pick a device on that node
  unsigned linkIndex = random() % _links.size();
  return linkIndex;
}

static void
FailLink(Ptr<OutputStreamWrapper> trace, CCNxStandardForwarderHelper &standardHelper, NfpRoutingHelper &nfpHelper)
{
  LogComponentEnable ("NfpRoutingProtocol", (LogLevel) (LOG_LEVEL_WARN | LOG_PREFIX_ALL ));

  ns3::LogTimePrinter timePrinter = ns3::LogGetTimePrinter ();
  (*timePrinter)(std::cout);
  std::cout << "*** Fail Link ***" << std::endl;

  // report the current computation costs so we can subtract that out later
  ReportComputationCost(trace, nfpHelper);

  int linkIndex = PickLinkToFail();

  std::cout << "Failing link index " << linkIndex << std::endl;

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
  em->SetRate(1.0);
  em->SetUnit(RateErrorModel::ERROR_UNIT_PACKET);
  em->Enable();

  Ptr<NetDevice> d0 = _links[linkIndex].Get(0);
  d0->SetAttribute("ReceiveErrorModel", PointerValue(em));

  Ptr<NetDevice> d1 = _links[linkIndex].Get(1);
  d1->SetAttribute("ReceiveErrorModel", PointerValue(em));

  LogComponentEnable ("NfpRoutingProtocol", (LogLevel) (LOG_LEVEL_WARN | LOG_PREFIX_ALL ));
  LogComponentEnable ("CCNxStandardForwarder", LOG_LEVEL_WARN);
}

static void
FailureFinished(Ptr<OutputStreamWrapper> trace, CCNxStandardForwarderHelper &standardHelper, NfpRoutingHelper &nfpHelper)
{
  ns3::LogTimePrinter timePrinter = ns3::LogGetTimePrinter ();
  (*timePrinter)(std::cout);
  std::cout << "*** Fail Finish ***" << std::endl;

  ReportComputationCost(trace, nfpHelper);

  // LogComponentEnable ("NfpRoutingProtocol", (LogLevel) (LOG_LEVEL_INFO | LOG_PREFIX_ALL ));
}


void
RunSimulation (std::string inputFileName, int anchorCount, int prefixCount, uint64_t seed)
{
  LogComponentEnableAll (LOG_PREFIX_ALL);
  LogComponentEnable ("CCNxStandardLayer3", LOG_LEVEL_WARN);
  LogComponentEnable ("NfpRoutingProtocol", LOG_LEVEL_ERROR);
  LogComponentEnable ("CCNxStandardForwarder", LOG_LEVEL_ERROR);
  LogComponentEnable ("CCNxPacket", LOG_LEVEL_ERROR);
  LogComponentEnable ("CCNxConnectionDevice", LOG_LEVEL_ERROR);
  LogComponentEnable ("CCNxCodecFixedHeader", LOG_LEVEL_ERROR);
  LogComponentEnable ("CCNxForwarder", LOG_LEVEL_ERROR);
  LogComponentEnable ("CCNxMessagePortal", LOG_LEVEL_ERROR);
  LogComponentEnable ("CCNxProducer", LOG_LEVEL_ERROR);
  LogComponentEnable ("CCNxConsumer", LOG_LEVEL_ERROR);

  Time::SetResolution (Time::NS);

  Ptr<OutputStreamWrapper> trace = Create<OutputStreamWrapper> (&std::cout);


  /*
   * Setup a Forwarder and associate it with the nodes created.
   */

  CCNxStackHelper ccnxStack;

  CCNxStandardForwarderHelper standardHelper;
  ccnxStack.SetForwardingHelper (standardHelper);

  NfpRoutingHelper nfpHelper;
  nfpHelper.Set ("HelloInterval", TimeValue (_helloInterval));
  nfpHelper.Set ("NeighborTimeout", TimeValue (_neighborTimeout));
  nfpHelper.Set ("RouteTimeout", TimeValue (_routeTimeout));

  //nfpHelper.PrintRoutingTableAllNodesWithInterval (Time (Seconds (5)), trace);

  ccnxStack.SetRoutingHelper (nfpHelper);

  // This will call ccnxStack.Install(), so make sure all the other helpers are specified before here.
  NodeContainer nodes = ReadFile(inputFileName, ccnxStack);

  // Requires that NFP be installed on nodes, so after ccnxStack.Install()
  seed += nfpHelper.SetSteams(nodes, seed);

  ApplicationContainer apps = AssignProducers(ccnxStack, nodes, anchorCount, prefixCount);

  Simulator::Stop (_simulationStopTime); // Allow an extra second for simulator to complete shutdown.

  // periodic status
  Simulator::Schedule (Seconds (1), &Progress, Seconds(1));

  // schedule the link failure and then the stats reporting after converged
  Simulator::Schedule (_linkFailureTime, &FailLink, trace, standardHelper, nfpHelper);
  Simulator::Schedule (_statsReportTime, &FailureFinished, trace, standardHelper, nfpHelper);

  std::cout << "*** starting simulation ***" << std::endl;

  Simulator::Run ();

  std::cout << "*** finishing simulation ***" << std::endl;

  ReportStats(trace, standardHelper);
  ReportComputationCost(trace, nfpHelper);

  Simulator::Destroy ();
}

int
main (int argc, char *argv[])
{
  std::string input ("topo.txt");
  unsigned anchors = 30;
  std::string prefixes ("210");
  uint64_t seed = time(NULL);

  CommandLine cmd;
  cmd.AddValue ("input", "Name of the input file.", input);
  cmd.AddValue ("anchors", "Anchor count", anchors);
  cmd.AddValue ("seed", "Random number seed.", seed);
  cmd.Parse (argc, argv);

  _uniformRandomVariable.SetStream(seed);
  seed++;

  RunSimulation (input, anchors, atoi(prefixes.c_str()), seed);
  return 0;
}
