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

#ifndef CCNS3SIM_ACMEFLATFORWARDER_H
#define CCNS3SIM_ACMEFLATFORWARDER_H

#include <map>
#include "ns3/ccnx-forwarder.h"
#include "ns3/ccnx-delay-queue.h"
#include "ns3/ccnx-standard-forwarder-work-item.h"
#include "ns3/nstime.h"

namespace ns3 {
namespace acme {
/**
 * @defgroup flat-forwarder Flat Forwarder: single path exact name routing
 * @ingroup ccnx-forwarder
 * */

/**
 * @ingroup flat-forwarder
 *
* The flat forwarder does not do longest matching prefix.  It is only
* exact match, so in it the same as having a flat global namespace.
 *
 * It is provided as a simple example of adding a different forwarder to the
 * CCNx layer 3 module.
*/
class AcmeFlatForwarder : public ccnx::CCNxForwarder
{
public:
  static TypeId GetTypeId (void);

  AcmeFlatForwarder ();
  virtual ~AcmeFlatForwarder ();

  virtual void RouteOutput (Ptr<ccnx::CCNxPacket> packet,
                            Ptr<ccnx::CCNxConnection> ingressConnection,
                            Ptr<ccnx::CCNxConnection> egressConnection);

  virtual void RouteInput (Ptr<ccnx::CCNxPacket> packet,
                           Ptr<ccnx::CCNxConnection> ingressConnection);

  virtual bool AddRoute (Ptr<ccnx::CCNxConnection> connection, Ptr<const ccnx::CCNxName> name);

  virtual bool RemoveRoute (Ptr<ccnx::CCNxConnection> connection, Ptr<const ccnx::CCNxName> name);

  virtual bool AddRoute (Ptr<const ccnx::CCNxRoute> route);

  virtual bool RemoveRoute (Ptr<const ccnx::CCNxRoute> route);

  virtual void PrintForwardingTable (Ptr<OutputStreamWrapper> streamWrapper) const;

  virtual void PrintForwardingStatistics (Ptr<OutputStreamWrapper> streamWrapper) const;

protected:
  /**
   * Called when object life starts in the simulator
   */
  virtual void DoInitialize (void);

  /**
   * Called at end of object life
   */
  virtual void DoDispose (void);

private:
  /**
   * Add a route by connection ID
   * @param [in] connId
   * @param [in] name
   * @return
   */
  bool AddRoute (ccnx::CCNxConnection::ConnIdType connId, Ptr<const ccnx::CCNxName> name);

  /**
   * Remove a route by connection ID
   * @param [in] connId
   * @param [in] name
   * @return
   */
  bool RemoveRoute (ccnx::CCNxConnection::ConnIdType connId, Ptr<const ccnx::CCNxName> name);

  /**
   * The common routing function called by RouteIn and RouteOut.
   */
  virtual Ptr<ccnx::CCNxConnection> InnerReceive (Ptr<ccnx::CCNxStandardForwarderWorkItem> item);

  /**
   * Once receiving from a net device or local L4 protocol is resolved to
   * an ingress connection and the packet is decoded, call this function
   * for CCNx L3-level forwarding.
   */
  Ptr<ccnx::CCNxConnection> ForwardInterest (Ptr<ccnx::CCNxPacket> packet, Ptr<ccnx::CCNxConnection> ingress);

  /**
   * Once receiving from a net device or local L4 protocol is resolved to
   * an ingress connection and the packet is decoded, call this function
   * for CCNx L3-level forwarding.
   */
  Ptr<ccnx::CCNxConnection> ForwardContentObject (Ptr<ccnx::CCNxPacket> packet, Ptr<ccnx::CCNxConnection> ingress);

  // a temporary fib for early testing.  Only has one mapping from a name
  // to a connection id.  Only does exact match.
  typedef std::map<Ptr<const ccnx::CCNxName>, ccnx::CCNxConnection::ConnIdType, ccnx::CCNxName::isLessPtrCCNxName> FibMapType;

  FibMapType m_fib;

  /**
   * The storage type of the CCNxDelayQueue
   */
  typedef ccnx::CCNxDelayQueue<ccnx::CCNxStandardForwarderWorkItem> DelayQueueType;

  /**
   * Input queue used to simulate processing delay
   */
  Ptr<DelayQueueType> m_inputQueue;

  /**
   * Callback from delay queue to compute the service time of a work item
   *
   * @param item [in] The work item being serviced
   * @return The service time of the work item
   */
  Time GetServiceTime (Ptr<ccnx::CCNxStandardForwarderWorkItem> item);

  /**
   * Callback from delay queue after a work item has waited its service time
   *
   * @param item [in] The work item to service
   */
  void ServiceInputQueue (Ptr<ccnx::CCNxStandardForwarderWorkItem> item);

  /**
   * The layer delay is:
   *
   * time = m_layerDelayConstant + m_layerDelaySlope * packetBytes
   *
   * It is set via the attribute "LayerDelayConstant".  The default is 1 usec.
   */
  Time m_layerDelayConstant;

  /**
   * The layer delay is:
   *
   * time = m_layerDelayConstant + m_layerDelaySlope * packetBytes
   *
   * It is set via the attribute "LayerDelaySlope".  The default is 0.
   */
  Time m_layerDelaySlope;

  /**
   * The number of parallel servers processing the input delay queue.
   * The more servers there are, the lower the amortized delay is.
   *
   * This value is set via the attribute "LayerDelayServers".  The default is 1.
   */
  unsigned m_layerDelayServers;

};
}
}

#endif //CCNS3SIM_ACMEFLATFORWARDER_H
