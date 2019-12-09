using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace MSLib.Messages {
    public class ServerListRequest : UDPMessage {

        public ServerListRequest(int size): base(size){

        }
        public ServerListRequest(IPEndPoint ipRemoteAddress, string sMessage, Byte[] barrMessage) : base(ipRemoteAddress, sMessage, barrMessage) {
            this.HasPacketIndex = true;
            this.InitializeReader();
        }

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
            ServerResults query_Results = MasterServer.Server_Store.queryServers(filter);

            // Put the query result along with the session into the active-query
            // handler.
            MasterServer.Sessions.startSession(this.RemoteAddress, this.Session, this.Key, query_Results);

            //Get the total packets to send
            
            //ServerResults cur = query_Results;
            ushort iTotal = (ushort)query_Results.Count;
            //while (cur != null) {
            //    iTotal++;
            //    cur = cur.next;
            //}
            

            //Build a list of packets to return
            List<UDPMessage> theResults = new List<UDPMessage>();
            ushort iPos = 0;
            foreach (ServerResult servRes in query_Results.Results) {
                theResults.Add(this.CreateListResponse(this.RemoteAddress, this.Session, this.Key, servRes, iPos++, iTotal));
            }

            //cur = query_Results;
            //while (cur != null) {
            //    theResults.Add(this.CreateListResponse(this.RemoteAddress, this.Session, this.Key, cur, iPos++, iTotal));
            //    cur = cur.next;
            //}

            return theResults;
        }

        private UDPMessage CreateListResponse(IPEndPoint ipWhere, ushort session, ushort key, ServerResult res, ushort which, ushort total) {
            UDPMessage theMessage = new UDPMessage(PacketConfig.LIST_PACKET_SIZE);
            //Set the header info
            theMessage.stuffHeader(8, 0, session, key);
            //Packet Index
            theMessage.writeU8((byte)which);

            //Total packets to send
            theMessage.writeU8((byte)total);
            //count of servers in this packet
            theMessage.writeU16((ushort)res.Count);

            //Now loop through the servers
            for (int i = 0; i < res.Count; i++) {
                //Write the quads
                string[] sarrParts = res.Servers[i].Address.ToString().Split('.');
                theMessage.writeU8(byte.Parse(sarrParts[0]));
                theMessage.writeU8(byte.Parse(sarrParts[1]));
                theMessage.writeU8(byte.Parse(sarrParts[2]));
                theMessage.writeU8(byte.Parse(sarrParts[3]));

                //write the port
                theMessage.writeU16((ushort)res.Servers[i].Port);
            }
            theMessage.RemoteAddress = this.RemoteAddress;

            return theMessage;
        }
    }
}
