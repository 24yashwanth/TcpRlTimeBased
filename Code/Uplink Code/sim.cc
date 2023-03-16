// Header Files

#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store-module.h"
#include "ns3/netanim-module.h"
#include "ns3/buildings-module.h"
#include "ns3/ns3-ai-module.h"


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/eps-bearer-tag.h"
#include "ns3/grid-scenario-helper.h"
#include "ns3/log.h"
#include "ns3/antenna-module.h"
#include "ns3/core-module.h"
#include "ns3/config-store.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/log.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/nr-module.h"
#include "tcp-rl.h"
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NrTCP");

std::vector<std::pair <uint32_t,float>> numberOfPacketsDelayFromVehiclesAtRLC;
std::vector<uint32_t> numberOfPacketsSentFromVehicle;
static bool g_rxRxRlcPDUCallbackCalled = false;
static bool g_rxPdcpCallbackCalled = false;
Time delay;

/*
 * TraceSink, RxRlcPDU connects the trace sink with the trace source (RxPDU). It connects the UE with gNB and vice versa.
 */
void RxRlcPDU (std::string path, uint16_t rnti, uint8_t lcid, uint32_t bytes, uint64_t rlcDelay)
{
  g_rxRxRlcPDUCallbackCalled = true;
  delay = Time::FromInteger(rlcDelay,Time::NS);
  std::cout<<"\n rlcDelay in S (Time):"<< delay.GetSeconds ()<<std::endl;

  std::cout<<"\n\n Data received at RLC layer at: " << Simulator::Now() <<" RNTI: "<<rnti<<" "
  <<" Number of Bytes: "<<bytes << "\n delay :" << rlcDelay << std::endl;
  
  numberOfPacketsDelayFromVehiclesAtRLC[rnti].first=numberOfPacketsDelayFromVehiclesAtRLC[rnti].first+1;
  numberOfPacketsDelayFromVehiclesAtRLC[rnti].second=numberOfPacketsDelayFromVehiclesAtRLC[rnti].second+delay.GetSeconds();
  
}

void
RxPdcpPDU (std::string path, uint16_t rnti, uint8_t lcid, uint32_t bytes, uint64_t pdcpDelay)
{
  std::cout << "\n Packet PDCP delay:" << pdcpDelay << "\n";
  g_rxPdcpCallbackCalled = true;
}

void
ConnectUlPdcpRlcTraces ()
{
    Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/DataRadioBearerMap/*/LteRlc/RxPDU",
                   MakeCallback (&RxRlcPDU));
    Config::ConnectFailSafe ("/NodeList/*/DeviceList/*/LteUeRrc/DataRadioBearerMap/*/LtePdcp/RxPDU",
                    MakeCallback (&RxPdcpPDU));
    NS_LOG_INFO ("Received PDCP RLC UL");
}

void showPosition (Ptr<Node> node, double deltaTime)
{
  uint32_t nodeId = node->GetId ();
  Ptr<MobilityModel> mobModel = node->GetObject<MobilityModel> ();
  Vector3D pos = mobModel->GetPosition ();
  Vector3D speed = mobModel->GetVelocity ();
  NS_LOG_INFO("At" << Simulator::Now ().GetSeconds () << " node " << nodeId
            << ": Position(" << pos.x << ", " << pos.y << ", " << pos.z
            << ");   Speed(" << speed.x << ", " << speed.y << ", " << speed.z
            << ")");

  Simulator::Schedule (Seconds (deltaTime), &showPosition, node, deltaTime);
}

void getBytesReceived (Ptr<PacketSink> sink, double deltaTime)
{
    NS_LOG_DEBUG("Total Bytes Received At " << Simulator::Now ().GetMilliSeconds() << " By " << sink->GetNode()->GetId() << " Is " << sink->GetTotalRx ());

    Simulator::Schedule (Seconds (deltaTime), &getBytesReceived, sink, deltaTime);
}

int main(int argc, char *argv[])
{
    bool logging=true;
    double simTime=50;//seconds
    double centralFrequency = 6e9; //according to given topology configurations
    double bandwidth = 30e6; //according to given topology configurations
    double txPower = 46; //according to given topology configurations
    double lambda = 1000;//by default set to non-full buffer case


    uint16_t numerology = 1;
    uint32_t noOfUEs = 30;
    std::string protocolToBeUsed = "TcpRlTimeBased";
    std::string transport_prot = "ns3::"+protocolToBeUsed;
    // bandwidth
    std::string bottleneck_bandwidth = "2Mbps";
    // delay
    std::string bottleneck_delay = "0.01ms";
    std::string recovery = "ns3::TcpClassicRecovery";
    bool sack = true;   
    std::string simTag = "NR_Tcp_Uplink_UE_30"  + protocolToBeUsed;
    std::string outputDir = "./NrTcpUplinkLogs/Numerology1";
    numberOfPacketsDelayFromVehiclesAtRLC = std::vector<std::pair<uint32_t,float>> (noOfUEs+1, std::make_pair(0, 0));
    numberOfPacketsSentFromVehicle = std::vector<uint32_t> (noOfUEs,{0});

    double speed=10;
    bool mobile=false;

    CommandLine cmd;
    cmd.AddValue ("logging",
                "Enable logging",
                logging);
    cmd.AddValue ("txPower",
                "Tx power to be configured to gNB",
                txPower);
    cmd.AddValue ("frequency",
                "The system frequency",
                centralFrequency);
    cmd.AddValue ("bandwidth",
                "The system bandwidth",
                bandwidth);
    cmd.AddValue ("lambda",
                "Number of UDP packets per second",
                lambda);
    cmd.AddValue ("transport_prot",
                "Transport protocol to use: TcpNewReno, "
                "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, "
                "TcpLp, TcpRl, TcpRlTimeBased",
                transport_prot);
    cmd.AddValue ("mobile","If true , then mobile case of UE,if false = then static case",mobile);
    cmd.AddValue ("speed","Set speed for mobility case",speed);
    cmd.Parse (argc, argv);

    // 4 MB of TCP buffer
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
    Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (2));

    // For UE Random Walk Mobility Model
    Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Mode", StringValue ("Time"));
    Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Time", StringValue ("1s"));
    Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Speed", StringValue ("ns3::UniformRandomVariable[Min=5.0|Max=15.0]"));
    Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Bounds", RectangleValue (Rectangle (0.0, 1000.0, 0.0, 1000.0)));

    Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
                      TypeIdValue (TypeId::LookupByName (recovery)));

    TypeId tcpTid;
    NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid),
                        "TypeId " << transport_prot << " not found");
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
                          TypeIdValue (TypeId::LookupByName (transport_prot)));

    // parse again so you can override default values from the command line
    cmd.Parse(argc, argv);
    // LogLevel logLevel2 = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME |
    //                                   LOG_PREFIX_NODE | LOG_LEVEL_DEBUG);
 
    LogLevel logLevel1 = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);
    if (logging)
    {
        LogComponentEnable ("ns3::TcpRlTimeBased", logLevel1); //keep
        LogComponentEnable ("NrTCP", logLevel1);  //keep
        LogComponentEnable ("NrMacRxTrace", logLevel1);
        LogComponentEnable ("TcpCongestionOps", logLevel1);  //keep
        LogComponentEnable ("TcpIllinois", logLevel1);  //keep
        LogComponentEnable ("ns3::TcpRlEnv", logLevel1);
    }


    NS_LOG_UNCOND ("Tcp Congestion in NR");
    NS_LOG_UNCOND ("--Tcp version: " << transport_prot);

    // setup the nr simulation
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();

    /*
    * Spectrum division. We create one operation band with one component carrier
    * (CC) which occupies the whole operation band bandwidth. The CC contains a
    * single Bandwidth Part (BWP). This BWP occupies the whole CC band.
    * Both operational bands will use the StreetCanyon channel modeling.
    */
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1;  // in this example, both bands have a single CC
    BandwidthPartInfo::Scenario scenario = BandwidthPartInfo::UMa_LoS;



    // Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
    // a single BWP per CC
    CcBwpCreator::SimpleOperationBandConf bandConf (centralFrequency,
                                                    bandwidth,
                                                    numCcPerBand,
                                                    scenario);

    // By using the configuration created, it is time to make the operation bands
    OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc (bandConf);

    /*
    * Initialize channel and pathloss, plus other things inside band1. If needed,
    * the band configuration can be done manually, but we leave it for more
    * sophisticated examples. For the moment, this method will take care
    * of all the spectrum initialization needs.
    */
    nrHelper->InitializeOperationBand (&band);

    BandwidthPartInfoPtrVector allBwps = CcBwpCreator::GetAllBwps ({band});

    /*
    * Continue setting the parameters which are common to all the nodes, like the
    * gNB transmit power or numerology.
    */
    nrHelper->SetGnbPhyAttribute ("TxPower", DoubleValue (txPower));
    nrHelper->SetGnbPhyAttribute ("Numerology", UintegerValue (numerology));

    // Scheduler

    Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (999999999));

    nrHelper->SetSchedulerTypeId (TypeId::LookupByName ("ns3::NrMacSchedulerTdmaPF"));

    // Antennas for all the UEs
    nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (2));
    nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (4));
    nrHelper->SetUeAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

    // Antennas for all the gNbs
    nrHelper->SetGnbAntennaAttribute ("NumRows", UintegerValue (4));
    nrHelper->SetGnbAntennaAttribute ("NumColumns", UintegerValue (8));
    nrHelper->SetGnbAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

    // Beamforming method
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    idealBeamformingHelper->SetAttribute ("BeamformingMethod", TypeIdValue (DirectPathBeamforming::GetTypeId ()));
    nrHelper->SetBeamformingHelper (idealBeamformingHelper);

    Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod",TimeValue (MilliSeconds (0)));
    //  nrHelper->SetChannelConditionModelAttribute ("UpdatePeriod", TimeValue (MilliSeconds (0)));
    nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (false));

    // Error Model: UE and GNB with same spectrum error model.
    nrHelper->SetUlErrorModel ("ns3::NrEesmIrT1");
    nrHelper->SetDlErrorModel ("ns3::NrEesmIrT1");

    // Both DL and UL AMC will have the same model behind.

    nrHelper->SetGnbDlAmcAttribute ("AmcModel", EnumValue (NrAmc::ShannonModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
    nrHelper->SetGnbUlAmcAttribute ("AmcModel", EnumValue (NrAmc::ShannonModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel


    // Create EPC helper
    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
    nrHelper->SetEpcHelper (epcHelper);
    // Core latency
    epcHelper->SetAttribute ("S1uLinkDelay", TimeValue (MilliSeconds (2)));

    // gNb routing between Bearer anNrTCPd bandwidh part
    uint32_t bwpIdForBearer = 0;
    nrHelper->SetGnbBwpManagerAlgorithmAttribute ("GBR_CONV_VOICE", UintegerValue (bwpIdForBearer));

    // Initialize nrHelper
    nrHelper->Initialize ();


    /*
    *  Create the gNB and UE nodes according to the network topology
    */
    NodeContainer gNbNodes;
    NodeContainer ueNodes;
    MobilityHelper gNBmobility;
    MobilityHelper uESMobility;


    Ptr<ListPositionAllocator> bsPositionAlloc = CreateObject<ListPositionAllocator> ();
    Ptr<ListPositionAllocator> utPositionAlloc = CreateObject<ListPositionAllocator> ();

    const double gNbHeight = 10;
    const double ueHeight = 1.5;

    gNbNodes.Create (1);
    ueNodes.Create (noOfUEs);

    bsPositionAlloc->Add (Vector (500.0, 500.0, gNbHeight));
    float angle = 0.0;
    float theta = (360 / noOfUEs)*3.14159/180;
    int radius = 499;
    for(uint32_t i=0; i<noOfUEs; i++) {
        utPositionAlloc->Add (Vector (radius*(1 + sin(angle)), radius*(1 + cos(angle)), ueHeight));
        angle += theta;
    }
    gNBmobility.SetPositionAllocator (bsPositionAlloc);
    gNBmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    gNBmobility.Install (gNbNodes);

    uESMobility.SetPositionAllocator (utPositionAlloc);
    uESMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("1s"),
                             "Speed", StringValue ("ns3::UniformRandomVariable[Min=5.0|Max=15.0]"),
                             "Bounds", RectangleValue (Rectangle (0.0, 1000.0, 0.0, 1000.0)));
    uESMobility.Install (ueNodes);

    // Install nr net devices
    NetDeviceContainer gNbNetDev = nrHelper->InstallGnbDevice (gNbNodes,
                                                                allBwps);

    NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice (ueNodes,
                                                            allBwps);
    // Band40: CC0 - BWP0 & Band38: CC1 - BWP1
    nrHelper->GetGnbPhy (gNbNetDev.Get (0), 0)->SetAttribute ("Numerology", UintegerValue (numerology));
    nrHelper->GetGnbPhy (gNbNetDev.Get (0), 0)->SetAttribute ("TxPower",DoubleValue (txPower));
    nrHelper->GetGnbPhy (gNbNetDev.Get (0), 0)->SetAttribute ("RbOverhead", DoubleValue (0.1));

    int64_t randomStream = 1;
    randomStream += nrHelper->AssignStreams (gNbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams (ueNetDev, randomStream);


    // When all the configuration is done, explicitly call UpdateConfig ()

    for (auto it = gNbNetDev.Begin (); it != gNbNetDev.End (); ++it)
    {
        DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
    }

    for (auto it = ueNetDev.Begin (); it != ueNetDev.End (); ++it)
    {
        DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
    }
    // create the internet and install the IP stack on the UEs
    // get SGW/PGW and create a single RemoteHost
    Ptr<Node> pgw = epcHelper->GetPgwNode ();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create (1);
    Ptr<Node> remoteHost = remoteHostContainer.Get (0);
    InternetStackHelper internet;
    internet.Install (remoteHostContainer);

    // connect a remoteHost to pgw. Setup routing too

    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute ("DataRate", StringValue (bottleneck_bandwidth));
    p2ph.SetChannelAttribute ("Delay", StringValue (bottleneck_delay));

    NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
    internet.Install (ueNodes);



    Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDev));

    for(uint32_t i=0; i<noOfUEs; i++) {
        // Set the default gateway for the UEs
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNodes.Get (i)->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

    // attach UE to the closest eNB
    nrHelper->AttachToClosestEnb (ueNetDev, gNbNetDev);

    //--------------------------Install and start applications on UEs and remote host-------------------Starts Here----------------------//
    NS_LOG_UNCOND("Write Something");
    NS_LOG_UNCOND(remoteHostAddr);
    NS_LOG_UNCOND(ueNodes.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0));
    ApplicationContainer sinkApps;
    ApplicationContainer sourceApps;
    uint16_t port = 9;
    PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port++));
    sinkApps.Add (sink.Install(remoteHost));
    for(uint32_t i=0; i<noOfUEs; i++) {
        BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (remoteHostAddr, 9));
        source.SetAttribute ("SendSize", UintegerValue (1000));
        sourceApps.Add (source.Install(ueNodes.Get (i)));
    }

    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (49.9));

    sourceApps.Start (Seconds (0.1));
    sourceApps.Stop (Seconds (49.8));

    nrHelper->EnableTraces();

    Ptr<FlowMonitor> flowmon;
    FlowMonitorHelper flowmonHelper;
    NodeContainer endPoints;
    endPoints.Add(remoteHost);
    endPoints.Add(ueNodes);
    flowmon = flowmonHelper.Install (endPoints);

    // Scheduling To Log RLC-RLC Delay

    Simulator::Schedule (Seconds (2.0), &ConnectUlPdcpRlcTraces);

    // Scheduling To Log Position of Each UE at every Second

    for (uint32_t i = 0; i < noOfUEs; i++) 
    {
        Simulator::Schedule (Seconds (0.0), &showPosition, ueNodes.Get (i), 1);
    }

    for (uint32_t i = 0; i < sinkApps.GetN (); i++) 
    {
        Simulator::Schedule (Seconds(0.0), &getBytesReceived, DynamicCast<PacketSink> (sinkApps.Get (i)), 0.001);
    }

    Simulator::Stop (Seconds (simTime));
    Simulator::Run ();
    
    flowmon->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
    FlowMonitor::FlowStatsContainer stats = flowmon->GetFlowStats ();

    double averageFlowThroughput = 0.0;
    double averageFlowDelay = 0.0;
    double averagePacketLoss=0.0;


    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        std::stringstream protoStream;
        protoStream << (uint16_t) t.protocol;
        if (t.protocol == 6)
        {
            protoStream.str ("TCP");
        }
        if (t.protocol == 17)
        {
            protoStream.str ("UDP");
        }
        std::cerr << "\n========================================================================\n";
        std::cerr << "\nFlow " << i->first << " (" << t.sourceAddress << ":" << t.sourcePort << " -> " << t.destinationAddress << ":" << t.destinationPort << ") proto " << protoStream.str () << "\n";
        std::cerr << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cerr << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cerr << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cerr << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cerr << "  Delay Sum:  " << i->second.delaySum.GetSeconds () << "\n";
        std::cerr << "  TxOffered:  " << i->second.txBytes * 8.0 / (simTime) / 1000 / 1000  << " Mbps\n";
        std::cerr << "\n------------------------------------------------------------------------\n";
        if (i->second.rxPackets > 0)
        {
            // Measure the duration of the flow from receiver's perspective
            double rxDuration = i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ();

            averageFlowThroughput += i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000;
            averageFlowDelay += 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets;
            
            std::cerr << "  Throughput: " << i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000  << " Mbps\n";
            std::cerr << "  Mean delay:  " << 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets << " ms\n";
            //std::cerr << "  Mean upt:  " << i->second.uptSum / i->second.rxPackets / 1000/1000 << " Mbps \n";
            std::cerr << "  Mean jitter:  " << 1000 * i->second.jitterSum.GetSeconds () / i->second.rxPackets  << " ms\n";
        }
        else
        {
            std::cerr << "  Throughput:  0 Mbps\n";
            std::cerr << "  Mean delay:  0 ms\n";
            std::cerr << "  Mean upt:  0  Mbps \n";
            std::cerr << "  Mean jitter: 0 ms\n";
        }
        double packetLoss=i->second.txPackets-i->second.rxPackets ;
        double lossRate=(packetLoss/i->second.txPackets)*100;
        std::cerr << "  Loss Rate: " <<lossRate<<"\n";
        averagePacketLoss+= packetLoss/ i->second.txPackets;

    }
    std::cerr << "\n========================================================================\n";
    std::cerr << "\n\n  Mean flow throughput: " << averageFlowThroughput / stats.size () << "\n";
    std::cerr << "\n\n  Aggregrate throughput: " << averageFlowThroughput<< "\n";
    std::cerr << "\n\n  Mean flow delay: " << averageFlowDelay / stats.size () << "\n";
    std::cerr << "\n\n  Mean packet loss rate(%): " << (averagePacketLoss / stats.size ())*100 << "\n";
    std::cerr << "\n========================================================================\n";


    // Bytes Received by Sink
    u_int64_t total = 0;
    for (uint32_t i = 0; i < sinkApps.GetN(); i++) 
    {
        Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (i));
        uint128_t val = sink1->GetTotalRx ();
        total += val;
        std::cout << "Total Bytes Received: " << i << " " << sink1->GetTotalRx () << std::endl;
    }
    std::cerr << "Toal Bytes Received : " << total << std::endl;


    flowmon->SerializeToXmlFile (outputDir+ "/" +simTag+".xml", true, true);

    // To Log RLC to RLC Delay for Each UE

    for(long unsigned int i=1; i<numberOfPacketsDelayFromVehiclesAtRLC.size(); i++)
    {
	    double x = numberOfPacketsDelayFromVehiclesAtRLC[i].first;
        double y = numberOfPacketsSentFromVehicle[i-1];
        std::cerr <<"RNTI:\t"<<i<<"\tNumber of packet: \t"<< numberOfPacketsDelayFromVehiclesAtRLC[i].first<< "Second\t"<< numberOfPacketsDelayFromVehiclesAtRLC[i].second <<"\tRLC Average Delay in MS:\t" <<double(numberOfPacketsDelayFromVehiclesAtRLC[i].second/numberOfPacketsDelayFromVehiclesAtRLC[i].first)*1000<<"\tPDR:\t"<<(x/y)*100<<"\tNumber of packet sent from vehicle:\t"<<numberOfPacketsSentFromVehicle[i-1]<<std::endl;
  }
    Simulator::Destroy ();
    return 0;

}
