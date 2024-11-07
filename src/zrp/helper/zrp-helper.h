#ifndef ZRP_HELPER_H
#define ZRP_HELPER_H

#include "ns3/ipv4-routing-helper.h"
#include "ns3/zrp-routing-protocol.h"

namespace ns3 {
class ZrpHelper : public Ipv4RoutingHelper{

public:

  ZrpHelper ();
  virtual ~ZrpHelper ();

  void SetZoneRadius(uint32_t zoneRadius);

  ZrpHelper* Copy (void) const;
  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

private:
  uint32_t m_zoneRadius;
};

} // namespace ns3

#endif /* ZRP_HELPER_H */