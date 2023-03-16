// // Header Files

// #include "ns3/core-module.h"
// #include "ns3/config-store.h"
// #include "ns3/network-module.h"
// #include "ns3/internet-module.h"
// #include "ns3/internet-apps-module.h"
// #include "ns3/applications-module.h"
// #include "ns3/mobility-module.h"
// #include "ns3/point-to-point-module.h"
// #include "ns3/flow-monitor-module.h"
// #include "ns3/nr-module.h"
// #include "ns3/config-store-module.h"
// #include "ns3/antenna-module.h"
// #include "ns3/netanim-module.h"
// #include "ns3/buildings-module.h"
// #include "ns3/ns3-ai-module.h"
// #include "tcp-rl.h"

// using namespace ns3;

// NS_LOG_COMPONENT_DEFINE ("NrTCP");

// int main(int argc, char *argv[])
// {
//     bool logging=true;
//     double simTime=10;//seconds
//     double centralFrequency = 6e9; //according to given topology configurations
//     double bandwidth = 30e6; //according to given topology configurations
//     double txPower = 23; //according to given topology configurations
//     double lambda = 1000;//by default set to non-full buffer case
//     uint16_t numerology = 1;
//     // Protocol to be used
//     std::string transport_prot = "ns3::TcpNewReno";
//     // bandwidth
//     std::string bottleneck_bandwidth = "2Mbps";
//     // delay
//     std::string bottleneck_delay = "0.01ms";
//     std::string recovery = "ns3::TcpClassicRecovery";
//     bool sack = true;   


//     double speed=10;
//     bool mobile=false;

//     CommandLine cmd;
//     cmd.AddValue ("logging",
//                 "Enable logging",
//                 logging);
//     cmd.AddValue ("txPower",
//                 "Tx power to be configured to gNB",
//                 txPower);
//     cmd.AddValue ("frequency",
//                 "The system frequency",
//                 centralFrequency);
//     cmd.AddValue ("bandwidth",
//                 "The system bandwidth",
//                 bandwidth);
//     cmd.AddValue ("lambda",
//                 "Number of UDP packets per second",
//                 lambda);
//     cmd.AddValue ("transport_prot",
//                 "Transport protocol to use: TcpNewReno, "
//                 "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
//                 "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, "
//                 "TcpLp, TcpRl, TcpRlTimeBased",
//                 transport_prot);
//     cmd.AddValue ("mobile","If true , then mobile case of UE,if false = then static case",mobile);
//     cmd.AddValue ("speed","Set speed for mobility case",speed);
//     cmd.Parse (argc, argv);

//     // 4 MB of TCP buffer
//     Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
//     Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
//     Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));
//     Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (2));

//     Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
//                       TypeIdValue (TypeId::LookupByName (recovery)));

//     TypeId tcpTid;
//     NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid),
//                         "TypeId " << transport_prot << " not found");
//     Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
//                           TypeIdValue (TypeId::LookupByName (transport_prot)));

//     // parse again so you can override default values from the command line
//     cmd.Parse(argc, argv);
//     // LogLevel logLevel2 = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME |
//     //                                   LOG_PREFIX_NODE | LOG_LEVEL_DEBUG);
 
//     LogLevel logLevel1 = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);
//     if (logging)
//     {
//         // LogComponentEnable ("NrTCPMain", LOG_LEVEL_INFO); //keep
//         LogComponentEnable ("NrTCP", logLevel1);  //keep
//         LogComponentEnable("ns3::TcpRlEnv", logLevel1);
//         LogComponentEnable("ns3::TcpRlTimeBased", logLevel1);
//         LogComponentEnable("TcpCongestionOps", logLevel1);
//     }


//     NS_LOG_UNCOND ("Tcp Congestion in NR");
//     NS_LOG_UNCOND ("--Tcp version: " << transport_prot);

//     // setup the nr simulation
//     Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();

//     /*
//     * Spectrum division. We create one operation band with one component carrier
//     * (CC) which occupies the whole operation band bandwidth. The CC contains a
//     * single Bandwidth Part (BWP). This BWP occupies the whole CC band.
//     * Both operational bands will use the StreetCanyon channel modeling.
//     */
//     CcBwpCreator ccBwpCreator;
//     const uint8_t numCcPerBand = 1;  // in this example, both bands have a single CC
//     BandwidthPartInfo::Scenario scenario = BandwidthPartInfo::UMa_LoS;



//     // Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
//     // a single BWP per CC
//     CcBwpCreator::SimpleOperationBandConf bandConf (centralFrequency,
//                                                     bandwidth,
//                                                     numCcPerBand,
//                                                     scenario);

//     // By using the configuration created, it is time to make the operation bands
//     OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc (bandConf);

//     /*
//     * Initialize channel and pathloss, plus other things inside band1. If needed,
//     * the band configuration can be done manually, but we leave it for more
//     * sophisticated examples. For the moment, this method will take care
//     * of all the spectrum initialization needs.
//     */
//     nrHelper->InitializeOperationBand (&band);

//     BandwidthPartInfoPtrVector allBwps = CcBwpCreator::GetAllBwps ({band});

//     /*
//     * Continue setting the parameters which are common to all the nodes, like the
//     * gNB transmit power or numerology.
//     */
//     nrHelper->SetGnbPhyAttribute ("TxPower", DoubleValue (txPower));
//     nrHelper->SetGnbPhyAttribute ("Numerology", UintegerValue (numerology));

//     // Scheduler

//     Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (999999999));

//     nrHelper->SetSchedulerTypeId (TypeId::LookupByName ("ns3::NrMacSchedulerTdmaMR"));

//     // Antennas for all the UEs
//     nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (2));
//     nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (4));
//     nrHelper->SetUeAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

//     // Antennas for all the gNbs
//     nrHelper->SetGnbAntennaAttribute ("NumRows", UintegerValue (4));
//     nrHelper->SetGnbAntennaAttribute ("NumColumns", UintegerValue (8));
//     nrHelper->SetGnbAntennaAttribute ("AntennaElement", PointerValue (CreateObject<ThreeGppAntennaModel> ()));

//     // Beamforming method
//     Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
//     idealBeamformingHelper->SetAttribute ("BeamformingMethod", TypeIdValue (DirectPathBeamforming::GetTypeId ()));
//     nrHelper->SetBeamformingHelper (idealBeamformingHelper);

//     Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod",TimeValue (MilliSeconds (0)));
//     //  nrHelper->SetChannelConditionModelAttribute ("UpdatePeriod", TimeValue (MilliSeconds (0)));
//     nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (false));

//     // Error Model: UE and GNB with same spectrum error model.
//     nrHelper->SetUlErrorModel ("ns3::NrEesmIrT1");
//     nrHelper->SetDlErrorModel ("ns3::NrEesmIrT1");

//     // Both DL and UL AMC will have the same model behind.

//     nrHelper->SetGnbDlAmcAttribute ("AmcModel", EnumValue (NrAmc::ShannonModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
//     nrHelper->SetGnbUlAmcAttribute ("AmcModel", EnumValue (NrAmc::ShannonModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel


//     // Create EPC helper
//     Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
//     nrHelper->SetEpcHelper (epcHelper);
//     // Core latency
//     epcHelper->SetAttribute ("S1uLinkDelay", TimeValue (MilliSeconds (2)));

//     // gNb routing between Bearer anNrTCPd bandwidh part
//     uint32_t bwpIdForBearer = 0;
//     nrHelper->SetGnbBwpManagerAlgorithmAttribute ("GBR_CONV_VOICE", UintegerValue (bwpIdForBearer));

//     // Initialize nrHelper
//     nrHelper->Initialize ();


//     /*
//     *  Create the gNB and UE nodes according to the network topology
//     */
//     NodeContainer gNbNodes;
//     NodeContainer ueNodes;
//     MobilityHelper mobility;

//     mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

//     Ptr<ListPositionAllocator> bsPositionAlloc = CreateObject<ListPositionAllocator> ();
//     Ptr<ListPositionAllocator> utPositionAlloc = CreateObject<ListPositionAllocator> ();

//     const double gNbHeight = 10;
//     const double ueHeight = 1.5;

//     gNbNodes.Create (1);
//     ueNodes.Create (1);

//     mobility.Install (gNbNodes);
//     mobility.Install (ueNodes);
//     bsPositionAlloc->Add (Vector (0.0, 0.0, gNbHeight));
//     utPositionAlloc->Add (Vector (0.0, 30.0, ueHeight));
//     bsPositionAlloc->Add (Vector (0.0, 0, gNbHeight));

//     mobility.SetPositionAllocator (bsPositionAlloc);
//     mobility.Install (gNbNodes);

//     mobility.SetPositionAllocator (utPositionAlloc);
//     mobility.Install (ueNodes);


//     // Install nr net devices
//     NetDeviceContainer gNbNetDev = nrHelper->InstallGnbDevice (gNbNodes,
//                                                                 allBwps);

//     NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice (ueNodes,
//                                                             allBwps);
//     // Band40: CC0 - BWP0 & Band38: CC1 - BWP1
//     nrHelper->GetGnbPhy (gNbNetDev.Get (0), 0)->SetAttribute ("Numerology", UintegerValue (numerology));
//     nrHelper->GetGnbPhy (gNbNetDev.Get (0), 0)->SetAttribute ("TxPower",DoubleValue (txPower));
//     nrHelper->GetGnbPhy (gNbNetDev.Get (0), 0)->SetAttribute ("RbOverhead", DoubleValue (0.1));

//     int64_t randomStream = 1;
//     randomStream += nrHelper->AssignStreams (gNbNetDev, randomStream);
//     randomStream += nrHelper->AssignStreams (ueNetDev, randomStream);


//     // When all the configuration is done, explicitly call UpdateConfig ()

//     for (auto it = gNbNetDev.Begin (); it != gNbNetDev.End (); ++it)
//     {
//         DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
//     }

//     for (auto it = ueNetDev.Begin (); it != ueNetDev.End (); ++it)
//     {
//         DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
//     }
//     // create the internet and install the IP stack on the UEs
//     // get SGW/PGW and create a single RemoteHost
//     Ptr<Node> pgw = epcHelper->GetPgwNode ();
//     NodeContainer remoteHostContainer;
//     remoteHostContainer.Create (1);
//     Ptr<Node> remoteHost = remoteHostContainer.Get (0);
//     InternetStackHelper internet;
//     internet.Install (remoteHostContainer);

//     // connect a remoteHost to pgw. Setup routing too
//     PointToPointHelper p2ph;
//     p2ph.SetDeviceAttribute ("DataRate", StringValue (bottleneck_bandwidth));
//     // p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
//     p2ph.SetChannelAttribute ("Delay", StringValue (bottleneck_delay));

//     NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
//     Ipv4AddressHelper ipv4h;
//     ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
//     Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
//     // Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
//     Ipv4StaticRoutingHelper ipv4RoutingHelper;
//     Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
//     remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
//     internet.Install (ueNodes);


//     Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDev));

//     // Set the default gateway for the UEs
//     Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNodes.Get (0)->GetObject<Ipv4> ());
//     ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

//     // attach UE to the closest eNB
//     nrHelper->AttachToClosestEnb (ueNetDev, gNbNetDev);

//     //--------------------------Install and start applications on UEs and remote host-------------------Starts Here----------------------//
//     NS_LOG_DEBUG("Write Something");

//     PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
//     ApplicationContainer sinkApps = sink.Install (ueNodes.Get (0));
//     sinkApps.Start (Seconds (0.0));
//     sinkApps.Stop (Seconds (9.0));

//     BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address ("7.0.0.2"), 9));
//     source.SetAttribute ("SendSize", UintegerValue (1000));
//     ApplicationContainer sourceApps = source.Install (remoteHost);
//     sourceApps.Start (Seconds (1.0));
//     sourceApps.Stop (Seconds (8.0));
//     nrHelper->EnableTraces();

//     Simulator::Stop (Seconds (simTime));
//     Simulator::Run ();

//     Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
//     std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
//     Simulator::Destroy ();
//     return 0;

// }

