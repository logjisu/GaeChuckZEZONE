#include <iostream>
#include <cmath>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/zrp-helper.h"
#include "ns3/v4ping-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/aodv-helper.h"
#include "ns3/olsr-helper.h"

using namespace ns3;

int main (int argc, char *argv[]){

  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);                                          // 노드 수

  LogComponentEnable ("OlsrRoutingProtocol", LOG_LEVEL_INFO);   // OLSR 로그 활성화
  LogComponentEnable ("AodvRoutingProtocol", LOG_LEVEL_INFO);   // AODV 로그 활성화
  LogComponentEnable ("ZrpRoutingProtocol", LOG_LEVEL_INFO);    // ZRP 로그 활성화

  // Set up WiFi
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();

  wifiPhy.SetChannel (wifiChannel.Create ());

  WifiMacHelper wifiMac;

  wifiMac.SetType ("ns3::AdhocWifiMac");

  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);

  // Set up mobility model
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (5.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  // Install the internet stack on nodes
  InternetStackHelper stack;
  ZrpHelper zrp;

  zrp.SetZoneRadius(3);                         // zone 수 변경

  stack.SetRoutingHelper (zrp);
  stack.Install (nodes);

  // Assign IP addresses to the devices
  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // Set up applications (e.g., a UDP echo server and client)
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));

  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);

  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));

  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // Enable tracing
  AsciiTraceHelper ascii;

  wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("zrp-simulation.tr"));
  wifiPhy.EnablePcapAll ("zrp-simulation");

  // Run the simulation
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;

}