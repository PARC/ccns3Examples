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

/*
 * Simulation script to run the experiments in the PARC ccns3Sim paper submitted
 * to ACM ICN 2016.
 *
 * Usage:
 * ./waf --run parc-acm-icn-2016 --command-template="%s --seed=10 --test=[prefix_delete, link_failure, link_recovery, add_replicas] [--replicas=n]"
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

// The time at which we measure the stats to capture initialization cost
static Time _initTime = Seconds(12);

static Time _testStartTime = Seconds(33);
static Time _testStopTime = Seconds(38);

static Time _helloInterval = Seconds(1);
static Time _neighborTimeout = Seconds(3); // 3 missed hellos

static Time _routeTimeout = Seconds(10); // flood every 10 seconds

static UniformRandomVariable _uniformRandomVariable;

typedef std::vector< NetDeviceContainer > DevicePairsListType;

enum TimeSeriesEvents {
  EVENT_INIT = 0,
  EVENT_TEST_START = 1,
  EVENT_TEST_FINISH = 2,
  EVENT_SIM_FINISH = 3
};

static std::vector< NfpComputationCost > _costTimeSeries;
static std::vector< NfpStats > _statsTimeSeries;

typedef enum {
  TEST_LINK_FAILURE,
  TEST_LINK_RECOVERY,
  TEST_PREFIX_DELETE,
  TEST_ADD_REPLICAS,
  TEST_UNKNOWN
} TestType;

static struct {
  TestType	testType;
  std::string	argument;
} testStringToType[] = {
    { TEST_LINK_FAILURE, "link_failure" },
    { TEST_LINK_RECOVERY, "link_recovery" },
    { TEST_PREFIX_DELETE, "prefix_delete" },
    { TEST_ADD_REPLICAS, "add_replicas" },
    { TEST_UNKNOWN, "" },
};

static TestType
TestStringToType(std::string s)
{
  for (int i = 0; testStringToType[i].testType != TEST_UNKNOWN; ++i) {
      if (s == testStringToType[i].argument) {
	  return testStringToType[i].testType;
      }
  }
  return TEST_UNKNOWN;
}

// ================
// Test configuration

typedef struct {
  std::string inputFileName;
  unsigned anchorCount;
  unsigned prefixCount;
  unsigned replicaCount;	// used for the add_replicas test
  uint64_t seed;
  bool detailed;
  TestType testType;

  std::vector< Ptr<Node> > anchors;
  ApplicationContainer apps;
} TestData;

static TestData _testData;

// ================

/*
 * A vector that lists each pair of devices that make up a PPP link.  When we want to
 * fail a link, we pick a random entry from the vector and then fail each device on the link.
 */
static DevicePairsListType _links;

static Ptr <CCNxContentRepository>
CreateRepository(size_t prefixIndex)
{
  char buffer[255];
  sprintf(buffer, "ccnx:/name=acm/name=icn/name=%06zu", prefixIndex);
  Ptr <const CCNxName> prefix = Create <CCNxName> (buffer);

  Ptr <CCNxContentRepository> repo = Create <CCNxContentRepository> (prefix, 1000, 10000);
  return repo;
}

/**
 * Adds a producer to an anchor.  It will use 'prefixIndex' as part of the repository name.
 *
 * @param prefixIndex
 */
static void
AddProducerToAnchor(size_t anchorIndex, Ptr <CCNxContentRepository> repo, Time start, Time stop)
{
  CCNxProducerHelper producerHelper (repo);
  ApplicationContainer producerApps = producerHelper.Install (_testData.anchors[anchorIndex]);
  producerApps.Start (start);
  producerApps.Stop (stop);
  _testData.apps.Add(producerApps);

  if (_testData.detailed) {
      std::cout << "Producer " << *repo->GetRepositoryPrefix() << " assigned to anchor " << _testData.anchors[anchorIndex]->GetId() << std::endl;
  }
}

static void
AssignProducers(CCNxStackHelper &stackHelper, NodeContainer &nodes, int anchorCount, int prefixCount)
{

  if (nodes.GetN() < anchorCount) {
      std::cout << "Not enough nodes for the number of anchors" << std::endl;
      return;
  }

  // pick the random anchors
  while (_testData.anchors.size() < anchorCount) {
      int anchorIndex = _uniformRandomVariable.GetInteger(0, nodes.GetN()-1);
      Ptr<Node> node = nodes.Get(anchorIndex);

      std::vector< Ptr<Node> >::iterator i = find(_testData.anchors.begin(), _testData.anchors.end(), node);
      if (i == _testData.anchors.end()) {
	  _testData.anchors.push_back(node);
      }
  }

  int prefixDeleteIndex = -1;
  if (_testData.testType == TEST_PREFIX_DELETE) {
      // The selected prefix will stop advertising at _testStopTime, instead of the normal _producerStopTime
      prefixDeleteIndex = _uniformRandomVariable.GetInteger(0, prefixCount-1);
      std::cerr << "Prefix delete index " << prefixDeleteIndex << std::endl;
  }

  for (int prefixIndex = 0; prefixIndex < prefixCount; prefixIndex++) {
      Ptr <CCNxContentRepository> repo = CreateRepository(prefixIndex);

      for (int replica = 0; replica < _testData.replicaCount; replica++) {
	int anchorIndex = _uniformRandomVariable.GetInteger(0, _testData.anchors.size() - 1);

	if (prefixDeleteIndex == prefixIndex && replica == 0) {
	    AddProducerToAnchor(anchorIndex, repo, _producerStartTime, _testStopTime);
	} else {
	    AddProducerToAnchor(anchorIndex, repo, _producerStartTime, _producerStopTime);
	}
      }
  }
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
	      if (_testData.detailed) {
		  std::cout << "Add node " << entry.srcName << " id " << src->GetId() << std::endl;
	      }
	  }

	  Ptr<Node> dst = nodeTable[entry.dstName];
	  if (dst == 0) {
	      dst = CreateObject<Node>();
	      nodeTable[entry.dstName] = dst;
	      nodes.Add(dst);
	      if (_testData.detailed) {
		  std::cout << "Add node " << entry.dstName << " id " << dst->GetId() << std::endl;
	      }
	  }

	  PointToPointHelper ppp;
	  ppp.SetDeviceAttribute ("DataRate", StringValue (entry.bw));
	  ppp.SetChannelAttribute ("Delay", StringValue (entry.delay));
	  ppp.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(entry.queueLength));

	  NetDeviceContainer devpair = ppp.Install (src, dst);
	  devices.Add(devpair);
	  _links.push_back(devpair);

	  if (_testData.detailed) {
	      std::cout << "linkid " << _links.size() -1 << " : " << entry.srcName << " <-> " << entry.dstName << std::endl;
	  }

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

__attribute__((unused))
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

/**
 * Calculate the total, network-wide computation cost from time 0 to now.
 * Will append the cost in _costTimeSeries[], so one can look at the deltas between
 * calls to this function.
 *
 * @param trace [in] The output stream to write the results to
 * @param nfpHelper [in] The NFP helper to query for the total
 */
static void
ReportComputationCost(Ptr<OutputStreamWrapper> trace, NfpRoutingHelper &nfpHelper)
{
  NodeContainer nodes = NodeContainer::GetGlobal();

  NfpComputationCost costTotal;
  NfpStats statsTotal;

  for (int i = 0; i < nodes.GetN(); ++i) {
      Ptr<Node> node = nodes.Get(i);
      costTotal += nfpHelper.GetComputationCost(node);
      statsTotal += nfpHelper.GetStats(node);
  }

  std::ostream *os = trace->GetStream();

  ns3::LogTimePrinter timePrinter = ns3::LogGetTimePrinter ();
  (*timePrinter)(*os);
  *os << " Total Computation Cost " << costTotal;
  *os << " Messages Sent " << statsTotal.GetAdvertiseSent() + statsTotal.GetWithdrawSent();
  *os << " Packets Sent " << statsTotal.GetPayloadsSent();
  *os << std::endl;

  *os << statsTotal << std::endl;

  _costTimeSeries.push_back(costTotal);
  _statsTimeSeries.push_back(statsTotal);
}

/*
 * Pick a link between two nodes with at least 2 interfaces each.  Do not fail a singly-connected edge.
 */
static unsigned
PickLinkToFail()
{
  // pick a node, then pick a device on that node
  unsigned linkIndex = _uniformRandomVariable.GetInteger(0, _links.size()-1);
  return linkIndex;
}

static void
FailLink(Ptr<OutputStreamWrapper> trace, CCNxStandardForwarderHelper &standardHelper, NfpRoutingHelper &nfpHelper)
{
  LogComponentEnable ("NfpRoutingProtocol", (LogLevel) (LOG_LEVEL_INFO | LOG_PREFIX_ALL ));

  ns3::LogTimePrinter timePrinter = ns3::LogGetTimePrinter ();
  (*timePrinter)(std::cout);
  std::cout << "*** Fail Link ***" << std::endl;

  // report the current computation costs so we can subtract that out later
  ReportComputationCost(trace, nfpHelper);

  int linkIndex = PickLinkToFail();

  std::cerr << "Failing link index " << linkIndex << std::endl;

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
TestFinished(Ptr<OutputStreamWrapper> trace, CCNxStandardForwarderHelper &standardHelper, NfpRoutingHelper &nfpHelper)
{
  ns3::LogTimePrinter timePrinter = ns3::LogGetTimePrinter ();
  (*timePrinter)(std::cout);
  std::cout << "*** Fail Finish ***" << std::endl;

  ReportComputationCost(trace, nfpHelper);

  LogComponentDisable("NfpRoutingProtocol", LOG_LEVEL_ALL);
  LogComponentEnable ("NfpRoutingProtocol", (LogLevel) (LOG_LEVEL_ERROR | LOG_PREFIX_ALL ));

  LogComponentDisable("CCNxStandardForwarder", LOG_LEVEL_ALL);
  LogComponentEnable ("CCNxStandardForwarder", (LogLevel) (LOG_LEVEL_ERROR | LOG_PREFIX_ALL ));
}

static void
AddReplicas(Ptr<OutputStreamWrapper> trace, CCNxStandardForwarderHelper &standardHelper, NfpRoutingHelper &nfpHelper)
{
  LogComponentEnable ("NfpRoutingProtocol", (LogLevel) (LOG_LEVEL_WARN | LOG_PREFIX_ALL ));

  // report the current computation costs so we can subtract that out later
  ReportComputationCost(trace, nfpHelper);

	// we need to create the producer apps at time 0 and scheudle them to start at the test time
	Ptr <CCNxContentRepository> repo = CreateRepository(_testData.prefixCount + 2);
	for (int i = 0; i < _testData.replicaCount; i++) {
	    int anchorIndex = _uniformRandomVariable.GetInteger(0, _testData.anchors.size()-1);
	    // stop them at the end of the simulation, not during the measurement period as we only
	    // want to measure adding replicas, not removing them.
	    AddProducerToAnchor(anchorIndex, repo, _testStartTime, _producerStopTime);
	}
}

static void
PrefixDelete(Ptr<OutputStreamWrapper> trace, CCNxStandardForwarderHelper &standardHelper, NfpRoutingHelper &nfpHelper)
{
  LogComponentEnable ("NfpRoutingProtocol", (LogLevel) (LOG_LEVEL_WARN | LOG_PREFIX_ALL ));

  // report the current computation costs so we can subtract that out later
  ReportComputationCost(trace, nfpHelper);

  // The producer stop time for the selected anchor was set in the AssignProducersToAnchors() call
}

static void
ScheduleTest(Ptr<OutputStreamWrapper> trace, CCNxStandardForwarderHelper &standardHelper, NfpRoutingHelper &nfpHelper)
{
  switch (_testData.testType)
  {
    case TEST_LINK_FAILURE:
      // schedule the link failure and then the stats reporting after converged
      Simulator::Schedule (_testStartTime, &FailLink, trace, standardHelper, nfpHelper);
      Simulator::Schedule (_testStopTime, &TestFinished, trace, standardHelper, nfpHelper);
      break;

    case TEST_ADD_REPLICAS:
      // schedule the link failure and then the stats reporting after converged
      Simulator::Schedule (_testStartTime, &AddReplicas, trace, standardHelper, nfpHelper);
      Simulator::Schedule (_testStopTime, &TestFinished, trace, standardHelper, nfpHelper);
      break;

    case TEST_PREFIX_DELETE:
      // schedule the link failure and then the stats reporting after converged
      Simulator::Schedule (_testStartTime, &PrefixDelete, trace, standardHelper, nfpHelper);
      Simulator::Schedule (_testStopTime, &TestFinished, trace, standardHelper, nfpHelper);
      break;

    default:
      NS_ASSERT_MSG(false, "Unimplemented test type " << _testData.testType);
  }
}
void
RunSimulation ()
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
  NodeContainer nodes = ReadFile(_testData.inputFileName, ccnxStack);

  // Requires that NFP be installed on nodes, so after ccnxStack.Install()
  _testData.seed += nfpHelper.SetSteams(nodes, _testData.seed);

  AssignProducers(ccnxStack, nodes, _testData.anchorCount, _testData.prefixCount);

  Simulator::Stop (_simulationStopTime); // Allow an extra second for simulator to complete shutdown.

  // periodic status
  Simulator::Schedule (Seconds (10), &Progress, Seconds(10));

  // The stats after initialization
  Simulator::Schedule (_initTime, &TestFinished, trace, standardHelper, nfpHelper);

  ScheduleTest(trace, standardHelper, nfpHelper);

  std::cout << "*** starting simulation ***" << std::endl;

  Simulator::Run ();

  std::cout << "*** finishing simulation ***" << std::endl;

//  ReportStats(trace, standardHelper);
  ReportComputationCost(trace, nfpHelper);

  uint64_t costDelta = _costTimeSeries[EVENT_TEST_FINISH].GetTotalCost() - _costTimeSeries[EVENT_TEST_START].GetTotalCost();
  uint64_t messagesDelta = _statsTimeSeries[EVENT_TEST_FINISH].GetAdvertiseSent() + _statsTimeSeries[EVENT_TEST_FINISH].GetWithdrawSent() -
      _statsTimeSeries[EVENT_TEST_START].GetAdvertiseSent() - _statsTimeSeries[EVENT_TEST_START].GetWithdrawSent();
  uint64_t packetsDelta = _statsTimeSeries[EVENT_TEST_FINISH].GetPayloadsSent() - _statsTimeSeries[EVENT_TEST_START].GetPayloadsSent();
  uint64_t helloDelta = _statsTimeSeries[EVENT_TEST_FINISH].GetHellosSent() - _statsTimeSeries[EVENT_TEST_START].GetHellosSent();

  std::cout << std::endl << "Delta comp " << costDelta;
  std::cout << " msgs " << messagesDelta;
  std::cout << " hellos " << helloDelta;
  std::cout << " pkts " << packetsDelta;

  std::cout << " Init comp " << _costTimeSeries[EVENT_INIT].GetTotalCost();
  std::cout << " msgs " << _statsTimeSeries[EVENT_INIT].GetAdvertiseSent() + _statsTimeSeries[EVENT_INIT].GetWithdrawSent();
  std::cout << " hellos " << _statsTimeSeries[EVENT_INIT].GetHellosSent();
  std::cout << " pkts " << _statsTimeSeries[EVENT_INIT].GetPayloadsSent();

  std::cout << " Total comp " << _costTimeSeries[EVENT_SIM_FINISH].GetTotalCost();
  std::cout << " msgs " << _statsTimeSeries[EVENT_SIM_FINISH].GetAdvertiseSent() + _statsTimeSeries[EVENT_SIM_FINISH].GetWithdrawSent();
  std::cout << " hellos " << _statsTimeSeries[EVENT_SIM_FINISH].GetHellosSent();
  std::cout << " pkts " << _statsTimeSeries[EVENT_SIM_FINISH].GetPayloadsSent();

  std::cout << std::endl;

  Simulator::Destroy ();
}

int
main (int argc, char *argv[])
{
  _testData.inputFileName = "topo.txt";
  _testData.anchorCount = 30;
  _testData.prefixCount = 210;
  _testData.replicaCount = 1;
  _testData.seed = time(NULL);
  _testData.detailed = false;
  _testData.testType = TEST_UNKNOWN;

  std::string testTypeString = "unknown";

  CommandLine cmd;
  cmd.AddValue ("input", "Name of the input file.", _testData.inputFileName);
  cmd.AddValue ("test", "prefix_delete | link_failure | link_recovery | add_replicas", testTypeString);
  cmd.AddValue ("anchors", "Anchor count", _testData.anchorCount);
  cmd.AddValue ("prefixes", "Prefix count", _testData.prefixCount);
  cmd.AddValue ("replicas", "Replica count", _testData.replicaCount);
  cmd.AddValue ("seed", "Random number seed.", _testData.seed);
  cmd.AddValue ("detailed", "Print detailed information", _testData.detailed);
  cmd.Parse (argc, argv);

  _testData.testType = TestStringToType(testTypeString);
  NS_ASSERT_MSG(_testData.testType != TEST_UNKNOWN, "Unknown test type " << testTypeString << ", see --help");

  _uniformRandomVariable.SetStream(_testData.seed);
  _testData.seed++;

  RunSimulation ();
  return 0;
}
