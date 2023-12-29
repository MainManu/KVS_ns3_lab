#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/ipv4-flow-classifier.h"
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiExperiment");


int main(int argc, char *argv[])
{
    double disctance = 10.0;
    double simulationTime = 10.0;
    std::string propagationModel = "ns3::FriisPropagationLossModel";

    CommandLine cmd(__FILE__);
    cmd.AddValue("distance", "distance between nodes", disctance);
    cmd.AddValue("simulationTime", "simulation time in seconds", simulationTime);
    cmd.AddValue("propagationModel", "propagation model to use", propagationModel);
    cmd.Parse(argc, argv);

    // // Create nodes
    // NodeContainer wifiStaNode;
    // wifiStaNode.Create(1);
    // NodeContainer wifiApNode;
    // wifiApNode.Create(1);

    // Create channel helper
    Ptr<PropagationLossModel> propagationLossModel;
    if (propagationModel == "ns3::FriisPropagationLossModel") {
        propagationLossModel = CreateObject<FriisPropagationLossModel>();
    } else if (propagationModel == "ns3::TwoRayGroundPropagationLossModel") {
        propagationLossModel = CreateObject<TwoRayGroundPropagationLossModel>();
    } else if (propagationModel == "ns3::FixedRssLossModel") {
        propagationLossModel = CreateObject<FixedRssLossModel>();
    } else if (propagationModel == "ns3::ThreeLogDistancePropagationLossModel") {
        propagationLossModel = CreateObject<ThreeLogDistancePropagationLossModel>();
    } else if (propagationModel == "ns3::TwoRayGroundPropagationLossModel") {
        propagationLossModel = CreateObject<TwoRayGroundPropagationLossModel>();
    } else if (propagationModel == "ns3::NakagamiPropagationLossModel") {
        propagationLossModel = CreateObject<NakagamiPropagationLossModel>();
    }  else {
        NS_FATAL_ERROR("Invalid propagation model specified");
    }

    // Create channel
    Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel>();
    channel->SetPropagationLossModel(propagationLossModel);

    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    wifiPhy.SetChannel(channel);

    YansWifiHelper wifiHelper = YansWifiHelper::Default();
    wifiHelper.SetPhy(wifiPhy);
    
    // Create SSID
    Ssid ssid = Ssid("ns-3-ssid");

    // Create wifi mac adhoc helper
    WifiMacHelper wifiMacHelper;
    wifiMacHelper.SetType("ns3::AdhocWifiMac", "Ssid", SsidValue(ssid));

    // Set up WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211n_5GHZ);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue("HtMcs7"),
                                "ControlMode", StringValue("HtMcs0"));

    // Install WiFi on nodes
    NodeContainer nodes;
    nodes.Create(2);
    NetDeviceContainer devices = wifi.Install(wifiPhy, mac, nodes);

    // Set up mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0)); // Node 1 position
    positionAlloc->Add(Vector(distance, 0.0, 0.0)); // Node 2 position
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Set up Internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Set up IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Set up UDP traffic
    uint16_t port = 9; // Discard port (RFC 863)
    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), port));
    onoff.SetConstantRate(DataRate("75Mbps"), 1450);
    ApplicationContainer source_apps = onoff.Install(nodes.Get(0));
    source_apps.Start(Seconds(1.0));
    source_apps.Stop(Seconds(simulationTime));

    // Set up packet sink
    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    sink_apps = sink.Install(nodes.Get(1));
    sink_apps.Start(Seconds(1.0));
    sink_apps.Stop(Seconds(simulationTime));

    // Set up PHY RX callback to observe signal strength
    Config::Connect("/NodeList/*/DeviceList/*/Phy/RxBegin",
                    MakeCallback([](Ptr<const Packet> packet, double snr, WifiMode mode, enum WifiPreamble preamble) {
                        double signalStrengthDbm = RatioToDb(snr) + 30;
                        std::cout << "Signal strength: " << signalStrengthDbm << " dBm" << std::endl;
                    }));

    // Set up FlowMonitor to observe received data-rate
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Run simulation
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // Print FlowMonitor statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Received data-rate: " << i->second.rxBytes * 8.0 / simulationTime / 1024 / 1024  << " Mbps\n";
    }

    Simulator::Destroy();
}