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
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/opengym-module.h"

using namespace ns3;

// 상태 공간 정의
Ptr<OpenGymSpace> GetObservationSpace() {
    std::vector<uint32_t> shape = {1}; // 상태 공간 크기
    Ptr<OpenGymBoxContainer<float>> space = CreateObject<OpenGymBoxContainer<float>>(shape);
    return DynamicCast<OpenGymSpace>(space); // 적절한 타입 변환
}

// 행동 공간 정의
Ptr<OpenGymSpace> GetActionSpace() {
    std::vector<uint32_t> shape = {1}; // 행동 공간 크기
    Ptr<OpenGymBoxContainer<float>> space = CreateObject<OpenGymBoxContainer<float>>(shape);
    return DynamicCast<OpenGymSpace>(space); // 적절한 타입 변환
}

// 관찰값 반환
Ptr<OpenGymDataContainer> GetObservation() {
    auto obs = CreateObject<OpenGymBoxContainer<float>>();
    obs->AddValue(42.0); // 예제 값
    return obs;
}

// 보상 계산
float GetReward() {
    return 1.0; // 예제 보상 값
}

// 게임 종료 조건
bool GetGameOver() {
    return false; // 게임이 계속 진행
}

// 추가 정보 전달
std::string GetExtraInfo() {
    return "Simulation running...";
}

// 행동 실행
bool ExecuteActions(Ptr<OpenGymDataContainer> action) {
    auto actionContainer = DynamicCast<OpenGymBoxContainer<float>>(action);
    float actionValue = actionContainer->GetValue(0); // 첫 번째 행동 값
    NS_LOG_UNCOND("Action received: " << actionValue);
    return true; // 성공적으로 처리
}

int main (int argc, char *argv[]){

  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (10);

  LogComponentEnable ("OlsrRoutingProtocol", LOG_LEVEL_INFO); // OLSR 로그 활성화
  LogComponentEnable ("AodvRoutingProtocol", LOG_LEVEL_INFO); // AODV 로그 활성화
  LogComponentEnable ("ZrpRoutingProtocol", LOG_LEVEL_INFO); // ZRP 로그 활성화

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

  zrp.SetZoneRadius(2);

  stack.SetRoutingHelper (zrp);
  stack.Install (nodes);

  // Assign IP addresses to the devices
  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // Set up applications (e.g., a UDP echo server and client)
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (9));

  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (9), 9);

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

  // Install FlowMonitor
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  // OpenGymInterface 설정
  Ptr<OpenGymInterface> gymInterface = CreateObject<OpenGymInterface>(5555);
  gymInterface->SetGetObservationSpaceCb(MakeCallback(&GetObservationSpace));
  gymInterface->SetGetActionSpaceCb(MakeCallback(&GetActionSpace));
  gymInterface->SetGetObservationCb(MakeCallback(&GetObservation));
  gymInterface->SetGetRewardCb(MakeCallback(&GetReward));
  gymInterface->SetGetGameOverCb(MakeCallback(&GetGameOver));
  gymInterface->SetGetExtraInfoCb(MakeCallback(&GetExtraInfo));
  gymInterface->SetExecuteActionsCb(MakeCallback(&ExecuteActions));

  // Run the simulation
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();

  // Print flow monitor statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
    std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
    std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
    std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
    std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
    std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
    std::cout << "  Lost Packets: " << i->second.lostPackets << "\n";
    std::cout << "  Delay: " << i->second.delaySum.GetSeconds () / i->second.rxPackets << " s\n";
  }

  Simulator::Destroy ();

  return 0;

}