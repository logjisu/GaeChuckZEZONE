#ifndef ZRP_ROUTING_PROTOCOL_H
#define ZRP_ROUTING_PROTOCOL_H

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/aodv-routing-protocol.h"
#include "ns3/ipv4.h"
#include "ns3/timer.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/node-container.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/aodv-routing-protocol.h"

namespace ns3 {

class ZrpRoutingProtocol : public Ipv4RoutingProtocol{

public:
  Ptr<olsr::RoutingProtocol> m_olsr;  // OLSR 라우팅 프로토콜 객체
  Ptr<aodv::RoutingProtocol> m_aodv; // AODV 라우팅 프로토콜 객체

  static TypeId GetTypeId (void);

  ZrpRoutingProtocol ();
  virtual ~ZrpRoutingProtocol ();
  void Setup (Ptr<Ipv4> ipv4, NodeContainer nodes, uint32_t zoneRadius);

  void SetZoneRadius (uint32_t zoneRadius);

  /* Inherited from Ipv4RoutingProtocol */
  virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p,
                                      const Ipv4Header &header,
                                      Ptr<NetDevice> oif,
                                      Socket::SocketErrno& sockerr);
  virtual bool RouteInput (Ptr<const Packet> p,
                           const Ipv4Header &header,
                           Ptr<const NetDevice> idev,
                           const UnicastForwardCallback& ucb,
                           const MulticastForwardCallback& mcb,
                           const LocalDeliverCallback& lcb,
                           const ErrorCallback& ecb);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;
  void PrintOlsrRoutingTable(); // OLSR 라우팅 테이블 덤프 함수
  void SetOlsrRoutingProtocol(Ptr<olsr::RoutingProtocol> olsrRouting);
  uint32_t CalculateHopDistance (Ipv4Address dest);  // 목적지까지의 홉 수 계산 함수 추가
  
private:

  Ptr<Ipv4> m_ipv4;
  NodeContainer m_nodes;
  uint32_t m_zoneRadius;

  std::map<Ipv4Address, Ptr<Ipv4Route>> m_routingTable;

  void UpdateRoutingTable ();

  Ipv4Address GetNetworkAddress (Ipv4Address ip);

  void SendIarpMessages ();
  void HandleIarpMessages (Ptr<Socket> socket);
  void SendIerpMessages (Ipv4Address dest);
  void HandleIerpMessages (Ptr<Socket> socket);

  uint32_t m_requestId = 0;        // RREQ 메시지의 고유 요청 ID
  uint32_t m_sequenceNumber = 0;   // 출발지 시퀀스 번호
  //Timer m_helloTimer; // HELLO 메시지 타이머
  // olsr::RoutingProtocol
};

} // namespace ns3

#endif /* ZRP_ROUTING_PROTOCOL_H */