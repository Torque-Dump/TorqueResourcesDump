using System;
using System.Collections.Generic;
using System.Net;

namespace MSLib.Messages {
    /// <summary>
    /// This class represents a ServerListRequest message. It is also used to process the request and return
    /// the results of the request.
    /// </summary>
    public class ServerListRequest : UDPMessage {

        #region CTOR
        /// <summary>
        /// This constructor is used to create an outbound message
        /// </summary>
        /// <param name="size">the size in bytes to allocate for the message</param>
        public ServerListRequest(int size): base(size){}

        /// <summary>
        /// This constructor is used to create an inbound message.
        /// </summary>
        /// <param name="ipRemoteAddress">the address the package came from</param>
        /// <param name="sMessage">the string representation of the packet (useful for debugging)</param>
        /// <param name="barrMessage">the byte array of the message</param>
        public ServerListRequest(IPEndPoint ipRemoteAddress, string sMessage, Byte[] barrMessage) : base(ipRemoteAddress, sMessage, barrMessage) {
            this.HasPacketIndex = true;
            this.InitializeReader();
        }
        #endregion

        #region Properties
        public string GameType { get; set; }
        public string MissionType { get; set; }
        public ushort MinPlayers { get; set; }
        public ushort MaxPlayers { get; set; }
        public uint Regions { get; set; }
        public uint Version { get; set; }
        public ushort InfoFlags { get; set; }
        public ushort NumberBots { get; set; }
        public uint CPUSpeed { get; set; }
        public ushort BuddyCount { get; set; }
        public List<uint> BuddyList { get; set; }
        #endregion

        #region Methods
        /// <summary>
        /// Initializes the message reader and populates the messages properties
        /// </summary>
        protected override void InitializeReader() {
            //Let the base at it first
            base.InitializeReader();

            //GameType
            this.GameType = this.readCString();
            MasterServer.EventLog.LogEntry(3, string.Format("ServerListRequest GameType: {0}", this.GameType));

            //Mission Type
            this.MissionType = this.readCString();
            MasterServer.EventLog.LogEntry(3, string.Format("ServerListRequest MissionType: {0}", this.MissionType));

            //Min Players
            this.MinPlayers = this.readU8();
            MasterServer.EventLog.LogEntry(3, string.Format("ServerListRequest MinPlayers: {0}", this.MinPlayers));

            //Max Players
            this.MaxPlayers = this.readU8();
            MasterServer.EventLog.LogEntry(3, string.Format("ServerListRequest MaxPlayers: {0}", this.MaxPlayers));

            //Regions            
            this.Regions = this.readU32();
            MasterServer.EventLog.LogEntry(3, string.Format("ServerListRequest Regions: {0}", this.Regions));

            //Version
            this.Version = this.readU32();
            MasterServer.EventLog.LogEntry(3, string.Format("ServerListRequest Version: {0}", this.Version));

            //InfoFlags
            this.InfoFlags = this.readU8();
            MasterServer.EventLog.LogEntry(3, string.Format("ServerListRequest InfoFlags: {0}", this.InfoFlags));

            //Num Bots
            this.NumberBots = this.readU8();
            MasterServer.EventLog.LogEntry(3, string.Format("ServerListRequest NumberBots: {0}", this.NumberBots));

            //CPU Speed
            this.CPUSpeed = this.readU16();
            MasterServer.EventLog.LogEntry(3, string.Format("ServerListRequest CPUSpeed: {0}", this.CPUSpeed));

            //Buddy Count
            this.BuddyCount = this.readU8();
            MasterServer.EventLog.LogEntry(3, string.Format("ServerListRequest BuddyCount: {0}", this.BuddyCount));

            this.BuddyList = new List<uint>();
            for(ushort i=0;i<this.BuddyCount;i++){
                uint iPlayer = this.readU32();
                this.BuddyList.Add(iPlayer);
            }

        }

        /// <summary>
        /// Processes the message and returns the results
        /// </summary>
        /// <returns>Results of processing the message</returns>
        public override List<UDPMessage> ProcessRequest() {
            ServerFilter filter = new ServerFilter();
            /***********************************
            Read gameType/missionType
            ***********************************/
            filter.GameType = this.GameType;
            filter.MissionType = this.MissionType;

            /***********************************
             Read miscellaneous properties
            ***********************************/
            filter.MinPlayers = this.MinPlayers;
            filter.MaxPlayers = this.MaxPlayers;
            filter.Regions = (int)this.Regions;
            filter.Version = (int)this.Version;
            filter.FilterFlags = this.InfoFlags;
            filter.MaxBots = this.NumberBots;
            filter.MinCPUSpeed = (int)this.CPUSpeed;

            /************************************
             Read in the buddy list
            ************************************/
            filter.PlayerCount = this.BuddyCount;

            if (filter.MinPlayers < 0) filter.MinPlayers = 0;
            if (filter.MaxPlayers < 0) filter.MaxPlayers = 0;
            if (filter.MaxPlayers < filter.MinPlayers) filter.MaxPlayers = filter.MinPlayers;
            if (filter.Regions < 0) filter.Regions = 0;
            if (filter.Version < 0) filter.Version = 0;
            if (filter.MaxBots < 0) filter.MaxBots = 0;
            if (filter.MinCPUSpeed < 0) filter.MinCPUSpeed = 0;
            if (filter.PlayerCount < 0) filter.PlayerCount = 0;

            filter.PlayerList = this.BuddyList;

            // Ok, what to do?
            // First, get a server result set.
            ServerResults query_Results = MasterServer.Server_Store.QueryServers(filter);

            // Put the query result along with the session into the active-query
            // handler.
            MasterServer.Sessions.StartSession(this.RemoteAddress, this.Session, this.Key, query_Results);

            //Get the total packets to send
            
            //ServerResults cur = query_Results;
            ushort iTotal = (ushort)query_Results.Count;
            

            //Build a list of packets to return
            List<UDPMessage> theResults = new List<UDPMessage>();
            ushort iPos = 0;
            if (query_Results != null && query_Results.Results.Count > 0)
            {
                foreach (ServerResult servRes in query_Results.Results)
                {
                    theResults.Add(this.CreateListResponse(this.RemoteAddress, this.Session, this.Key, servRes, iPos++, iTotal));
                }
            }
            else
            {
                theResults.Add(this.CreateListResponse(this.RemoteAddress, this.Session, this.Key, null, iPos++, iTotal));
            }

            return theResults;
        }

        /// <summary>
        /// Creates outbound messages with the results of the server list query
        /// </summary>
        /// <param name="ipWhere"></param>
        /// <param name="session"></param>
        /// <param name="key"></param>
        /// <param name="res"></param>
        /// <param name="which"></param>
        /// <param name="total"></param>
        /// <returns></returns>
        private UDPMessage CreateListResponse(IPEndPoint ipWhere, ushort session, ushort key, ServerResult res, ushort which, ushort total) {
            UDPMessage theMessage = new UDPMessage(PacketConfig.LIST_PACKET_SIZE);
            //Set the header info
            theMessage.stuffHeader((ushort)MessageTypes.MasterServerListResponse, 0, session, key);
            //Packet Index
            theMessage.writeU8((byte)which);

            //Total packets to send
            theMessage.writeU8((byte)total);
            //count of servers in this packet
            theMessage.writeU16((res != null) ? (ushort)res.Count : (ushort)0);

            if (res != null)
            {
                //Now loop through the servers
                for (int i = 0; i < res.Count; i++)
                {
                    //Write the quads
                    string[] sarrParts = res.Servers[i].Address.ToString().Split('.');
                    theMessage.writeU8(byte.Parse(sarrParts[0]));
                    theMessage.writeU8(byte.Parse(sarrParts[1]));
                    theMessage.writeU8(byte.Parse(sarrParts[2]));
                    theMessage.writeU8(byte.Parse(sarrParts[3]));

                    //write the port
                    theMessage.writeU16((ushort)res.Servers[i].Port);
                }
            }
            theMessage.RemoteAddress = this.RemoteAddress;

            return theMessage;
        }
        #endregion

    }
}
