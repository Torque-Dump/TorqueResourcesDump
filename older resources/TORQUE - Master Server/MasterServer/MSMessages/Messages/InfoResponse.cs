using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;
namespace MSLib.Messages {
    public class InfoResponse : UDPMessage {

        public InfoResponse(int size): base(size){

        }
        public InfoResponse(IPEndPoint ipRemoteAddress, string sMessage, Byte[] barrMessage)
            : base(ipRemoteAddress, sMessage, barrMessage) {

            this.InitializeReader();
        }

        protected override void InitializeReader() {
            //Let the base at it first
            base.InitializeReader();

            //GameType
            this.GameType = this.readCString();
            MasterServer.EventLog.LogEntry(3, string.Format("InfoResponse GameType: {0}", this.GameType));
            
            //Mission Type
            this.MissionType = this.readCString();
            MasterServer.EventLog.LogEntry(3, string.Format("InfoResponse MissionType: {0}", this.MissionType));
            
            //Max Players
            this.MaxPlayers = this.readU8();
            MasterServer.EventLog.LogEntry(3, string.Format("InfoResponse MaxPlayers: {0}", this.MaxPlayers));
            
            //Regions            
            this.Regions = this.readU32();
            MasterServer.EventLog.LogEntry(3, string.Format("InfoResponse Regions: {0}", this.Regions));
            
            //Version
            this.Version = this.readU32();
            MasterServer.EventLog.LogEntry(3, string.Format("InfoResponse Version: {0}", this.Version));
            
            //InfoFlags
            this.InfoFlags = this.readU8();
            MasterServer.EventLog.LogEntry(3, string.Format("InfoResponse InfoFlags: {0}", this.InfoFlags));
            
            //Num Bots
            this.NumberBots = this.readU8();
            MasterServer.EventLog.LogEntry(3, string.Format("InfoResponse NumberBots: {0}", this.NumberBots));
            
            //CPU Speed
            this.CPUSpeed = this.readU16();
            MasterServer.EventLog.LogEntry(3, string.Format("InfoResponse CPUSpeed: {0}", this.CPUSpeed));
            
            //Buddy Count
            this.PlayerCount = this.readU8();
            MasterServer.EventLog.LogEntry(3, string.Format("InfoResponse BuddyCount: {0}", this.PlayerCount));

            this.PlayerList = new List<uint>();
            for (ushort i = 0; i < this.PlayerCount; i++) {
                uint iPlayer = this.readU32();
                this.PlayerList.Add(iPlayer);
            }

        }

        public string GameType { get; set; }
        public string MissionType { get; set; }
        public ushort MaxPlayers { get; set; }
        public uint Regions { get; set; }
        public uint Version { get; set; }
        public ushort InfoFlags { get; set; }
        public ushort NumberBots { get; set; }
        public uint CPUSpeed { get; set; }
        public ushort PlayerCount { get; set; }
        public List<uint> PlayerList { get; set; }

        public override List<UDPMessage> ProcessRequest(){
            ServerInfo info = new ServerInfo();
            info.RemoteAddress = this.RemoteAddress;
            info.GameKey[0] = 'T';
            info.GameKey[1] = 'G';
            info.GameKey[2] = 'E';
            info.GameKey[3] = ' ';
            info.GameType = this.GameType;
            info.MissionType = this.MissionType;
            info.MaxPlayers = this.MaxPlayers;
            info.Regions = (int)this.Regions;
            info.Version = (int)this.Version;
            info.InfoFlags = this.InfoFlags;
            info.NumBots = this.NumberBots;
            info.CPUSpeed = (int)this.CPUSpeed;
            info.PlayerCount = this.PlayerCount;

            if (info.MaxPlayers < 0) info.MaxPlayers = 0;
            if (info.Regions < 0) info.Regions = 0;
            if (info.Version < 0) info.Version = 0;
            if (info.NumBots < 0) info.NumBots = 0;
            if (info.CPUSpeed < 0) info.CPUSpeed = 0;
            if (info.PlayerCount < 0) info.PlayerCount = 0;

            info.PlayerList = this.PlayerList;

            //Done, store it.
            MasterServer.Server_Store.updateServer(this.RemoteAddress, info);

            return base.ProcessRequest();
        }
    }
}
