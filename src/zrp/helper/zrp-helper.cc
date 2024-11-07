#include "zrp-helper.h"
#include "ns3/zrp-routing-protocol.h"
#include "ns3/node.h"
#include "ns3/ipv4.h"

namespace ns3 {

ZrpHelper::ZrpHelper () : m_zoneRadius (2) {

}

ZrpHelper::~ZrpHelper (){

}

void ZrpHelper::SetZoneRadius (uint32_t zoneRadius) {
  m_zoneRadius = zoneRadius;
}

ZrpHelper* ZrpHelper::Copy (void) const{

  return new ZrpHelper (*this);

}

Ptr<Ipv4RoutingProtocol> ZrpHelper::Create (Ptr<Node> node) const{

  Ptr<ZrpRoutingProtocol> zrpRouting = CreateObject<ZrpRoutingProtocol> ();
  zrpRouting -> SetZoneRadius(m_zoneRadius);
  node->AggregateObject (zrpRouting);

  return zrpRouting;

}

} // namespace ns3