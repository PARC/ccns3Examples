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
 * # is up to the contributors to maintain their portion of this section and up to
 * # the user of the software to verify any claims herein.
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
 * Temporary FIB for initial testing.  Needs to be replaced.  Only does exact match.  Only single path routing.
 *    m_fib: CCNxName -> ConnId
 *

 */
#include "acme-flat-forwarder.h"

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/ccnx-l3-protocol.h"
#include "ns3/integer.h"

using namespace ns3;
using namespace ns3::acme;
using namespace ns3::ccnx;

NS_LOG_COMPONENT_DEFINE ("AcmeFlatForwarder");
NS_OBJECT_ENSURE_REGISTERED (AcmeFlatForwarder);

static const Time _defaultLayerDelayConstant = MicroSeconds (1);
static const Time _defaultLayerDelaySlope = Seconds (0);
static unsigned _defaultLayerDelayServers = 1;

TypeId
AcmeFlatForwarder::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ccnx::AcmeFlatForwarder")
    .SetParent<CCNxForwarder> ()
    .SetGroupName ("CCNx")
    .AddConstructor<AcmeFlatForwarder> ()
    .AddAttribute ("LayerDelayConstant", "The amount of constant layer delay",
                   TimeValue (_defaultLayerDelayConstant),
                   MakeTimeAccessor (&AcmeFlatForwarder::m_layerDelayConstant),
                   MakeTimeChecker ())
    .AddAttribute ("LayerDelaySlope", "The slope of the layer delay (in terms of packet bytes)",
                   TimeValue (_defaultLayerDelaySlope),
                   MakeTimeAccessor (&AcmeFlatForwarder::m_layerDelaySlope),
                   MakeTimeChecker ())
    .AddAttribute ("LayerDelayServers", "The number of servers for the layer delay input queue",
                   IntegerValue (_defaultLayerDelayServers),
                   MakeIntegerAccessor (&AcmeFlatForwarder::m_layerDelayServers),
                   MakeIntegerChecker<unsigned> ())
  ;
  return tid;
}

AcmeFlatForwarder::AcmeFlatForwarder ()
  :     m_layerDelayConstant (_defaultLayerDelayConstant), m_layerDelaySlope (_defaultLayerDelaySlope),
  m_layerDelayServers (_defaultLayerDelayServers)
{
  // empty
}

AcmeFlatForwarder::~AcmeFlatForwarder ()
{
  // empty (use DoDispose)
}

void
AcmeFlatForwarder::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  // empty
}

void
AcmeFlatForwarder::DoInitialize (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_inputQueue = Create<DelayQueueType> (m_layerDelayServers,
                                         MakeCallback (&AcmeFlatForwarder::GetServiceTime, this),
                                         MakeCallback (&AcmeFlatForwarder::ServiceInputQueue, this));
}

Time
AcmeFlatForwarder::GetServiceTime (Ptr<CCNxStandardForwarderWorkItem> item)
{
  Time delay = m_layerDelayConstant + m_layerDelaySlope * item->GetPacket ()->GetFixedHeader ()->GetPacketLength ();
  return delay;
}

void
AcmeFlatForwarder::RouteOutput (Ptr<CCNxPacket> packet,
                                Ptr<CCNxConnection> ingressConnection,
                                Ptr<CCNxConnection> egressConnection)
{
  NS_LOG_FUNCTION (this << packet << ingressConnection);
  Ptr<CCNxStandardForwarderWorkItem> item = Create<CCNxStandardForwarderWorkItem> (packet, ingressConnection, egressConnection);
  m_inputQueue->push_back (item);
}

void
AcmeFlatForwarder::RouteInput (Ptr<CCNxPacket> packet,
                               Ptr<CCNxConnection> ingressConnection)
{
  NS_LOG_FUNCTION (this << packet << ingressConnection);
  Ptr<CCNxStandardForwarderWorkItem> item = Create<CCNxStandardForwarderWorkItem> (packet, ingressConnection, Ptr<CCNxConnection> (0));
  m_inputQueue->push_back (item);
}

void
AcmeFlatForwarder::ServiceInputQueue (Ptr<CCNxStandardForwarderWorkItem> item)
{
  NS_LOG_FUNCTION (this << item->GetPacket () << item->GetIngressConnection () << item->GetEgressConnection ());

  Ptr<CCNxConnection> egress = InnerReceive (item);

  if (item->GetEgressConnection ())
    {
      // User specified an egressFromUser connection, so use that.
      item->SetRouteError (CCNxRoutingError::CCNxRoutingError_NoError);
      NS_LOG_DEBUG (": user has overridden fib lookup");
      egress = item->GetEgressConnection ();
    }

  Ptr<CCNxConnectionList> connections = Create<CCNxConnectionList> ();

  if (egress)
    {
      connections->push_back (egress);
    }

  item->SetConnectionsList (connections);

  NS_LOG_DEBUG ("RouteOutput(packet=" << *item->GetPacket () << ", from " << item->GetIngressConnection ()->GetConnectionId ()
                                      << ",  will be fwded to " << connections->size () << " destinations");

  if (connections->size () > 0)
    {
      NS_LOG_DEBUG ("first destination is " << connections->front ()->GetConnectionId () );
    }

  m_routeCallback (item->GetPacket (), item->GetIngressConnection (), item->GetRouteError (), item->GetConnectionsList ());
}

Ptr<CCNxConnection>
AcmeFlatForwarder::InnerReceive (Ptr<CCNxStandardForwarderWorkItem> item)
{
  NS_LOG_FUNCTION (this << item->GetPacket () << item->GetIngressConnection ());

  Ptr<CCNxConnection> egress;
  switch (item->GetPacket ()->GetFixedHeader ()->GetPacketType ())
    {
    case CCNxFixedHeaderType_Interest:
      egress = ForwardInterest (item->GetPacket (), item->GetIngressConnection ());
      break;

    case CCNxFixedHeaderType_Object:
      egress = ForwardContentObject (item->GetPacket (), item->GetIngressConnection ());
      break;

    default:
      NS_ASSERT_MSG (false, "Unsupported packetType");
    }
  NS_LOG_INFO ("Route " << item->GetPacket ()->GetMessage ()->GetName () << " egress " << egress);
  return egress;
}

Ptr<CCNxConnection>
AcmeFlatForwarder::ForwardInterest (Ptr<CCNxPacket> packet, Ptr<CCNxConnection> ingress)
{
  NS_LOG_FUNCTION (this << packet << ingress);
  NS_LOG_INFO ("Forwarding " << *packet);

  Ptr<CCNxConnection> result = Ptr<CCNxConnection> (0);

  FibMapType::const_iterator i = m_fib.find (packet->GetMessage ()->GetName ());
  if (i == m_fib.end ())
    {
      NS_LOG_INFO ("No route in FIB : " << *packet->GetMessage ()->GetName ());
      // DROP
    }
  else
    {
      if (i->second != ingress->GetConnectionId ())
        {
          Ptr<CCNxConnection> connection = m_ccnx->GetConnection (i->second);
          if (connection)
            {
              NS_LOG_INFO ("Route found, packet forward to connid " << connection->GetConnectionId ());
            }
          else
            {
              NS_LOG_INFO ("Could not resolve CCNxL3Protocol connection for connid " << i->second);
            }
          result = connection;
        }
      else
        {
          NS_LOG_INFO ("Egress is same as ingress connid " << i->second << " : no route");
        }
    }
  return result;
}

Ptr<CCNxConnection>
AcmeFlatForwarder::ForwardContentObject (Ptr<CCNxPacket> packet, Ptr<CCNxConnection> ingress)
{
  NS_LOG_FUNCTION (this << packet << ingress);

  NS_LOG_INFO ("Forwarding " << *packet);

  // TODO: The CS/PIT/FIB lookups
  return (Ptr<CCNxConnection>) 0;
}

// =========
// Fake FIB section

bool
AcmeFlatForwarder::AddRoute (CCNxConnection::ConnIdType connId, Ptr<const CCNxName> name)
{
  NS_LOG_FUNCTION (this << connId << name);

  if (connId != CCNxConnection::ConnIdLocalHost)
    {
      FibMapType::const_iterator j = m_fib.find (name);
      NS_ASSERT_MSG (j == m_fib.end (), "Name already exits in FIB " << *name);

      m_fib[name] = connId;

      NS_LOG_INFO ("AddRoute connId " << connId << " name " << *name);
      return true;
    }
  else
    {
      return false;
    }

}

bool
AcmeFlatForwarder::AddRoute (Ptr<CCNxConnection> connection, Ptr<const CCNxName> name)
{
  NS_LOG_FUNCTION (this << connection << name);
  return AddRoute (connection->GetConnectionId (), name);
}

bool
AcmeFlatForwarder::AddRoute (Ptr<const CCNxRoute> route)
{
  NS_LOG_FUNCTION (this << route);
  bool added = false;
  for (CCNxRoute::const_iterator i = route->begin (); i != route->end (); ++i)
    {
      added |= AddRoute (i->GetConnection ()->GetConnectionId (), i->GetPrefix ());
    }
  return added;
}


bool
AcmeFlatForwarder::RemoveRoute (CCNxConnection::ConnIdType connId, Ptr<const CCNxName> name)
{
  NS_LOG_FUNCTION (this << connId << name);
  FibMapType::iterator j = m_fib.find (name);
  if (j != m_fib.end () && (j->second == connId))
    {
      m_fib.erase (j);
      NS_LOG_INFO ("RemoveRoute connection " << connId << " name " << *name);
      return true;
    }
  return false;
}

bool
AcmeFlatForwarder::RemoveRoute (Ptr<CCNxConnection> connection, Ptr<const CCNxName> name)
{
  NS_LOG_FUNCTION (this << connection << name);
  return RemoveRoute (connection->GetConnectionId (), name);
}

bool
AcmeFlatForwarder::RemoveRoute (Ptr<const CCNxRoute> route)
{
  NS_LOG_FUNCTION (this << route);
  bool removed = false;
  for (CCNxRoute::const_iterator i = route->begin (); i != route->end (); ++i)
    {
      removed |= RemoveRoute (i->GetConnection (), i->GetPrefix ());
    }
  return removed;
}

void
AcmeFlatForwarder::PrintForwardingStatistics (Ptr<OutputStreamWrapper> streamWrapper) const
{
  std::ostream *stream = streamWrapper->GetStream ();
  *stream << "AcmeFlatForwarder does not support printing statistics" << std::endl;
}

void
AcmeFlatForwarder::PrintForwardingTable (Ptr<OutputStreamWrapper> streamWrapper) const
{
  std::ostream *stream = streamWrapper->GetStream ();
  *stream << "AcmeFlatForwarder not implemented" << std::endl;
}
