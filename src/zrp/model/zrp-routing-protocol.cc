#include "zrp-routing-protocol.h"
#include "ns3/log.h"
#include "ns3/ipv4-route.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/inet-socket-address.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/olsr-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ZrpRoutingProtocol");
NS_OBJECT_ENSURE_REGISTERED (ZrpRoutingProtocol);

TypeId ZrpRoutingProtocol::GetTypeId (void){

  static TypeId tid = TypeId ("ns3::ZrpRoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .SetGroupName ("Internet")
    .AddConstructor<ZrpRoutingProtocol> ();
  return tid;

}

ZrpRoutingProtocol::ZrpRoutingProtocol (){
  m_olsr = CreateObject<ns3::olsr::RoutingProtocol>();  // oslr 객체 초기화
  m_aodv = CreateObject<ns3::aodv::RoutingProtocol>();  // aodv 객체 초기화
}

ZrpRoutingProtocol::~ZrpRoutingProtocol (){

}

void ZrpRoutingProtocol::SetZoneRadius (uint32_t zoneRadius) {
  m_zoneRadius = zoneRadius;
}

void ZrpRoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4){

  m_ipv4 = ipv4;
  m_olsr->SetIpv4(ipv4);
  m_aodv->SetIpv4(ipv4);
  UpdateRoutingTable ();

}

void ZrpRoutingProtocol::Setup (Ptr<Ipv4> ipv4, NodeContainer nodes, uint32_t zoneRadius){

  m_ipv4 = ipv4;
  m_nodes = nodes;
  m_zoneRadius = zoneRadius;
  UpdateRoutingTable ();

}

void ZrpRoutingProtocol::NotifyInterfaceUp (uint32_t interface){

  UpdateRoutingTable ();

}

void ZrpRoutingProtocol::NotifyInterfaceDown (uint32_t interface){

  UpdateRoutingTable ();

}

void ZrpRoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address){

  UpdateRoutingTable ();

}

void ZrpRoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address){

  UpdateRoutingTable ();

}

Ptr<Ipv4Route> ZrpRoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno& sockerr){

  Ipv4Address dest = header.GetDestination ();

  uint32_t hopCount = CalculateHopDistance(dest);

  if (hopCount <= m_zoneRadius){
    NS_LOG_INFO("OLSR를 사용하겠습니다: " << dest);
    return m_olsr->RouteOutput(p, header, oif, sockerr); 
  }
  else{
    NS_LOG_INFO("AODV를 사용하겠습니다: " << dest);
    return m_aodv->RouteOutput(p, header, oif, sockerr); 
  }

  auto it = m_routingTable.find (GetNetworkAddress (dest));

  if (it != m_routingTable.end ()){
    NS_LOG_INFO ("Proactive route found for destination " << dest << "\n");
    return it->second;
  }

  sockerr = Socket::ERROR_NOROUTETOHOST;
  NS_LOG_INFO ("Reactive routing for destination " << dest << "\n");
  return nullptr;
}

bool ZrpRoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev, const UnicastForwardCallback& ucb, const MulticastForwardCallback& mcb, const LocalDeliverCallback& lcb, const ErrorCallback& ecb){

  Ipv4Address dest = header.GetDestination ();

  uint32_t hopCount = CalculateHopDistance(dest);

  if (hopCount <= m_zoneRadius){
    NS_LOG_INFO("OLSR를 사용하겠습니다: " << dest << " (홉 수: " << hopCount << ")");
    return m_olsr->RouteInput(p, header, idev, ucb, mcb, lcb, ecb);
  }
  else{
    NS_LOG_INFO("AODV를 사용하겠습니다: " << dest << " (홉 수: " << hopCount << ")");
    return m_aodv->RouteInput(p, header, idev, ucb, mcb, lcb, ecb);
  }

  auto it = m_routingTable.find (GetNetworkAddress (dest));

  if (it != m_routingTable.end ()){

    Ptr<Ipv4Route> route = it->second;

    if (route->GetOutputDevice () == idev){
      lcb (p, header, m_ipv4->GetInterfaceForDevice(idev));
      return true;
    }
    else{
      ucb (route, p, header);
      return true;
    }
  }

  ecb (p, header, Socket::ERROR_NOROUTETOHOST);

  return false;
}

uint32_t ZrpRoutingProtocol::CalculateHopDistance (Ipv4Address dest){

  std::vector<olsr::RoutingTableEntry> entries = m_olsr->GetRoutingTableEntries();
  
  for (const auto& entry : entries) {
    NS_LOG_INFO("OLSR 경로 발견: 목적지 = " << entry.destAddr << ", 거리 = " << entry.distance);
    if (entry.destAddr == dest) {
      return entry.distance;
    }
  }
  
  // 경로를 찾지 못한 경우
  NS_LOG_WARN("OLSR 라우팅 테이블에서 목적지 " << dest << "를 찾을 수 없습니다.");
  return m_zoneRadius + 1; // 최대 값 +1 반환하여 AODV로 처리
}

void ZrpRoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const{

  *stream->GetStream () << "ZRP Routing Table" << std::endl;

  for (auto it = m_routingTable.begin (); it != m_routingTable.end (); ++it){

    *stream->GetStream () << it->first << " -> " << it->second->GetGateway () << " via " << it->second->GetOutputDevice ()->GetIfIndex () << std::endl;

  }

}

Ipv4Address ZrpRoutingProtocol::GetNetworkAddress (Ipv4Address ip){

  uint32_t address = ip.Get ();

  if ((address & 0x80000000) == 0){

    return Ipv4Address (address & 0xff000000); // Class A

  }
  else if ((address & 0xc0000000) == 0x80000000){

    return Ipv4Address (address & 0xffff0000); // Class B

  }
  else{

    return Ipv4Address (address & 0xffffff00); // Class C

  }

}

void ZrpRoutingProtocol::UpdateRoutingTable (){

  m_routingTable.clear ();

  // m_olsr->SendHello();
  // m_olsr->SendTc();
  // m_olsr->SendMid();
  // m_olsr->SendHna();
  //m_olsr->RoutingTableComputation();
  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); ++i){

    for (uint32_t j = 0; j < m_ipv4->GetNAddresses (i); ++j){

      Ipv4InterfaceAddress ifAddr = m_ipv4->GetAddress (i, j);
      Ipv4Address netAddr = GetNetworkAddress (ifAddr.GetLocal ());
      Ptr<Ipv4Route> route = Create<Ipv4Route> ();

      route->SetDestination (Ipv4Address::GetBroadcast ());
      route->SetGateway (Ipv4Address ("0.0.0.0"));
      route->SetOutputDevice (m_ipv4->GetNetDevice (i));

      m_routingTable[netAddr] = route;

    }

  }

}


void ZrpRoutingProtocol::SendIarpMessages (){

  // Implement IARP message sending logic here
  NS_LOG_INFO("IARP 메시지 전송 시작");
  m_olsr->HelloTimerExpire();
  m_olsr->TcTimerExpire();

}

void ZrpRoutingProtocol::HandleIarpMessages (Ptr<Socket> socket){

  // Implement IARP message handling logic here
  Ptr<Packet> packet;
  Address from;
  packet = socket->RecvFrom(from);
  // OLSR에서 메시지 처리
  NS_LOG_INFO("IARP 메시지 수신: " << from);
  m_olsr->Receive(packet, from);

}

void ZrpRoutingProtocol::SendIerpMessages (Ipv4Address dest){

  // Implement IERP message sending logic here
  // RREQ 헤더 생성 및 초기화
  aodv::RreqHeader rreqHeader;
  rreqHeader.SetHopCount(1);
  rreqHeader.SetId(m_requestId++);   
  rreqHeader.SetDst(dest);           
  rreqHeader.SetDstSeqno(1);         
  rreqHeader.SetOrigin(m_ipv4->GetAddress(1, 0).GetLocal());
  rreqHeader.SetOriginSeqno(m_sequenceNumber++); 

  // RREQ 패킷 생성 및 RREQ 헤더 추가
  Ptr<Packet> packet = Create<Packet>();
  packet->AddHeader(rreqHeader);
  
  // Ipv4Header 객체 생성 및 목적지 설정
  Ipv4Header ipv4Header;
  ipv4Header.SetDestination(dest);

  // AODV 프로토콜에 전송 요청
  Socket::SocketErrno sockerr;
  m_aodv->RouteOutput(packet, ipv4Header, nullptr, sockerr);

}

void ZrpRoutingProtocol::HandleIerpMessages (Ptr<Socket> socket){

  // Implement IERP message handling logic here
  Ptr<Packet> packet;
  Address from;
  packet = socket->RecvFrom(from);
  m_aodv->RouteInput(packet, Ipv4Header(), socket->GetBoundNetDevice(), UnicastForwardCallback(), MulticastForwardCallback(), LocalDeliverCallback(), ErrorCallback());

}

} // namespace ns3