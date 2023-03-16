// Header Files

#include "ns3/core-module.h"
#include "ns3/config-store.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/nr-module.h"
#include "ns3/config-store-module.h"
#include "ns3/antenna-module.h"
#include "ns3/netanim-module.h"
#include "ns3/buildings-module.h"
#include "ns3/ns3-ai-module.h"
#include "tcp-rl.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NrTCP");

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
    // To Enable Logging
    bool logging = true;

    // Simulation Time in Seconds
    double simTime = 100; 
    
    // According to Given Topology
    double centralFrequency = 6e9;

    // According to Given Topology
    double bandwidth = 30e6;

    // According to Given Topology
    double txPower = 46; 

    // By default set to non-full buffer case
    double lambda = 1000;

    uint16_t numerology = 1;

    // Protocol to be used
    std::string transport_prot = "ns3::TcpCubic";

    // bandwidth
    std::string bottleneck_bandwidth = "2Mbps";

    // delay
    std::string bottleneck_delay = "0.01ms";

    // Recovery Protocol
    std::string recovery = "ns3::TcpClassicRecovery";

    bool sack = true;   

    // Number of UEs
    u_int16_t noOfUEs = 5;

    // Number of gNodeB
    u_int16_t noOfgNB = 1;

    double start_time = 0.0;
    double end_time = 99.9;



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
    cmd.Parse (argc, argv);

    // 4 MB of TCP buffer
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
    Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (2));

    // Recovery Protocol
    Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType", TypeIdValue (TypeId::LookupByName (recovery)));

    // For UE Random Walk Mobility Model
    Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Mode", StringValue ("Time"));
    Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Time", StringValue ("1s"));
    Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Speed", StringValue ("ns3::UniformRandomVariable[Min=5.0|Max=15.0]"));
    Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Bounds", RectangleValue (Rectangle (0.0, 1000.0, 0.0, 1000.0)));


    // Protocol to be Used
    TypeId tcpTid;
    NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid), "TypeId " << transport_prot << " not found");
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (transport_prot)));

    // Parse again so you can override default values from the command line
    cmd.Parse(argc, argv);
    
    /* For Logging */
    LogLevel logLevel1 = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);
    // LogLevel logLevel2 = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_DEBUG);
 
    if (logging)
    {
        // LogComponentEnable ("NrTCPMain", LOG_LEVEL_INFO); //keep
        LogComponentEnable ("NrTCP",  (LogLevel) (LOG_LEVEL_DEBUG | LOG_LEVEL_INFO));  //keep
        // LogComponentEnable("ns3::TcpRlEnv", logLevel1);
        // LogComponentEnable("ns3::TcpRlTimeBased", logLevel1);
        // LogComponentEnable("TcpCongestionOps", logLevel1);
        LogComponentEnable("TcpCubic", logLevel1);
    }


    NS_LOG_UNCOND ("Tcp Congestion in NR");
    NS_LOG_UNCOND ("--Tcp version: " << transport_prot);

    // Setup the nr simulation
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

    nrHelper->SetSchedulerTypeId (TypeId::LookupByName ("ns3::NrMacSchedulerTdmaMR"));

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

    const double gNbHeight = 10;
    const double ueHeight = 1.5;

    Ptr<ListPositionAllocator> bsPositionAlloc = CreateObject<ListPositionAllocator> ();
    Ptr<ListPositionAllocator> utPositionAlloc = CreateObject<ListPositionAllocator> ();

    gNbNodes.Create (noOfgNB);
    ueNodes.Create (noOfUEs);

    // gNodeB Mobility Constant Position Model


    for (uint32_t i=0; i<noOfgNB; i++) {
        bsPositionAlloc->Add (Vector (500.0, 500.0, gNbHeight));
    }
    gNBmobility.SetPositionAllocator (bsPositionAlloc);
    gNBmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    gNBmobility.Install (gNbNodes);

    // uE Mobility Random Walk Model

    utPositionAlloc->Add (Vector (1.0, 1.0, ueHeight));
    utPositionAlloc->Add (Vector (999.0, 1.0, ueHeight));
    utPositionAlloc->Add (Vector (1.0, 999.0, ueHeight));
    utPositionAlloc->Add (Vector (999.0, 999.0, ueHeight));
    utPositionAlloc->Add (Vector (501.0, 499.0, ueHeight));
    // double initialPosition = 10.0;
    // for (uint32_t i=0; i<noOfUEs; i++) 
    // {
    //     utPositionAlloc->Add (Vector (0.0, initialPosition, ueHeight));
    //     initialPosition += 10.0;
    // }
    uESMobility.SetPositionAllocator (utPositionAlloc);
    // uESMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

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
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
    internet.Install (ueNodes);


    Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDev));

    // Assign IP Address to UEs and Install Applications
    for (uint32_t i=0; i<noOfUEs; i++) 
    {
        // Set the default gateway for the UEs
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNodes.Get (i)->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }
    
    // attach UE to the closest eNB
    for (uint32_t i = 0; i < noOfUEs; i++) 
    {
        nrHelper->AttachToEnb (ueNetDev.Get (i), gNbNetDev.Get (0));
    }

    // Install Applications on UEs and RemoteHost

    ApplicationContainer sinkApps;
    ApplicationContainer sourceApps;

    for (uint32_t i = 0; i < noOfUEs; i++) 
    {
        PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
        sinkApps.Add (sink.Install (ueNodes.Get (i)));
        BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (ueIpIface.GetAddress (i), 9));
        source.SetAttribute ("SendSize", UintegerValue (1000));
        sourceApps.Add (source.Install (remoteHost));

    }

    // Start Applications

    sinkApps.Start (Seconds (start_time));
    sinkApps.Stop (Seconds (end_time));
    
    sourceApps.Start (Seconds (start_time + 0.1));
    sourceApps.Stop (Seconds (end_time - 0.1));

    // Enabling Traces
    nrHelper->EnableTraces();


    // Scheduling To Log Position of Each UE at every Second

    for (uint32_t i = 0; i < noOfUEs; i++) 
    {
        Simulator::Schedule (Seconds (0.0), &showPosition, ueNodes.Get (i), 1);
    }

    for (uint32_t i = 0; i < noOfUEs; i++) 
    {
        Simulator::Schedule (Seconds(0.0), &getBytesReceived, DynamicCast<PacketSink> (sinkApps.Get (i)), 0.001);
        // Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (i));
        // std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
    }

    Simulator::Stop (Seconds (simTime));
    Simulator::Run ();

    // Bytes Received by Sink
    u_int64_t total = 0;
    for (uint32_t i = 0; i < noOfUEs; i++) 
    {
        Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (i));
        uint128_t val = sink1->GetTotalRx ();
        total += val;
        std::cout << "Total Bytes Received: " << i << " " << sink1->GetTotalRx () << std::endl;
    }
    std::cerr << "Toal Bytes Received : " << total << std::endl;

    Simulator::Destroy ();
    return 0;
}

