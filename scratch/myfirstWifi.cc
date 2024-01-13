/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-utils.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/multi-model-spectrum-channel.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point
// n0 is the client and n1 is the server
//

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstScriptExample");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);                                  // can be changed exactly once
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO); // defined in echo client
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO); // defined in echo server

    NodeContainer nodes;
    nodes.Create(2);

    // // CHANNEL LOSS MODEL IS THE ISSUE CONFIRMED
    // double txPower = 10.0;
    // std::string propagationModel = "ns3::FixedRssLossModel";
    // //std::string propagationModel = "ns3::FriisPropagationLossModel";
    // YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default();
    // channelHelper.AddPropagationLoss(propagationModel);
    // YansWifiPhyHelper wifiPhy;
    // wifiPhy.SetChannel(channelHelper.Create());
    // StringValue wifi_channel_settings("{38, 40, BAND_5GHZ, 0}");
    // wifiPhy.Set("ChannelSettings", wifi_channel_settings);
    // wifiPhy.Set("TxPowerStart", ns3::DoubleValue(txPower));
    // wifiPhy.Set("TxPowerEnd", ns3::DoubleValue(txPower));
    // wifiPhy.Set("TxGain", ns3::DoubleValue(1.0));
    // wifiPhy.Set("RxGain", ns3::DoubleValue(1.0));

    // Create channel
    double txPower = 10.0;
    SpectrumWifiPhyHelper wifiPhy;
    Ptr<MultiModelSpectrumChannel> channel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<PropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    channel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->SetPropagationDelayModel(delayModel);
    wifiPhy.SetChannel(channel);
    wifiPhy.Set("TxPowerStart", DoubleValue(txPower));
    wifiPhy.Set("TxPowerEnd", DoubleValue(txPower));
    wifiPhy.Set("TxGain", DoubleValue(1.0));
    wifiPhy.Set("RxGain", DoubleValue(1.0));
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n); //causes "Can't find response rate for OfdmRate6Mbps
    WifiMacHelper wifiMacHelper;
    wifiMacHelper.SetType("ns3::AdhocWifiMac", "Ssid", SsidValue(Ssid("ns-3-ssid")));
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager");
    NetDeviceContainer wifiDevices = wifi.Install(wifiPhy, wifiMacHelper, nodes);

    // mobility working confirmed
    double distance = 10.0;
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));      // Node 1 position
    positionAlloc->Add(Vector(distance, 0.0, 0.0)); // Node 2 position
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    InternetStackHelper internet;
    internet.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces;
    interfaces = address.Assign(wifiDevices);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    // Set up UDP traffic
    // double simulationTime = 10.0;
    // uint16_t port = 9; // Discard port (RFC 863)
    // OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), port));
    // onoff.SetConstantRate(DataRate("75Mbps"), 1450U);
    // ApplicationContainer source_apps = onoff.Install(nodes.Get(0));
    // source_apps.Start(Seconds(2.0));
    // source_apps.Stop(Seconds(simulationTime));

    // // Set up packet sink
    // PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    // ApplicationContainer sink_apps = sink.Install(nodes.Get(1));
    // sink_apps.Start(Seconds(1.0));
    // sink_apps.Stop(Seconds(simulationTime));

    // Set up FlowMonitor to observe received data-rate
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
 

    // Set up UDP traffic 
    double SimulationTime = 600;
    // create server application
    UdpEchoServerHelper echoServer(9); // port 9

    ApplicationContainer serverApps =
        echoServer.Install(nodes.Get(1)); // nodes.Get(1) returns smart pointer to node 1 and uses
                                          // it in an unnamed constructor to create a NoceContainer
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(SimulationTime));

    // create client application
    UdpEchoClientHelper echoClient(interfaces.GetAddress(1),
                                   9); // interfaces.GetAddress(1) returns
                                       //  the address of node 1, 9 is the port
    // echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    // echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(19.33))); // 75 Mbps with 1450 byte
                                                                        // packets at app layer => 1 Packet per 19.33ms
    echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(20.0))); // 75 Mbps with 1450 byte
    echoClient.SetAttribute("PacketSize", UintegerValue(1450));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(SimulationTime));

    Simulator::Stop(Seconds(SimulationTime));
    Simulator::Run();
     // Print FlowMonitor statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << ")\n";
        std::cout << "  Received data-rate: "
                  << i->second.rxBytes * 8.0 / (SimulationTime-2.0) / 1024 / 1024 << " Mbps\n";
    }

    Simulator::Destroy();
    return 0;
}
