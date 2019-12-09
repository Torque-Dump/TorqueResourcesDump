using System.Collections.Generic;
using System.Net;

namespace MSLib {
    /// <summary>
    /// This represents a server that has been registered with the master server
    /// </summary>
    public class Server
    {

        #region CTOR
        public Server() {
            this.GameKey = new char[4];
            PlayerList = null;
            MissionType = null;
            GameType = null;
            MaxPlayers = 0;
            Regions = 0;
            Version = 0;
            InfoFlags = 0;
            NumBots = 0;
            CPUSpeed = 0;
            PlayerCount = 0;
            Last_Heart = 0;
            Last_Info = 0;
        }
        #endregion

        #region Properties
        /// <summary>
        /// The address of the server
        /// </summary>
        public IPEndPoint RemoteAddress { get; set; }

        //Used to determine if the server should be removed from the list
        public bool RemoveServer { get; set; }
        
        public char[] GameKey { get; set; } // What game is this? For now, TGE.
        public string GameType { get; set; }
        public string MissionType { get; set; }

        public int MaxPlayers { get; set; }
        public int Regions { get; set; }
        public int Version { get; set; }
        public int InfoFlags { get; set; }
        public int NumBots { get; set; }
        public int CPUSpeed { get; set; }

        public int PlayerCount { get; set; }
        public List<uint> PlayerList { get; set; }

        // Bookkeeping information
        public long Last_Heart { get; set; }	// Last time we got a heart beat
        public long Last_Info { get; set; }	// Last time we got info from them
        #endregion

        #region Methods
        /// <summary>
        /// Updates the properties of this server with the data from the ServerInfo object
        /// </summary>
        /// <param name="Info"></param>
        public void UpdateServerInfo(ServerInfo Info) {
            this.GameKey = Info.GameKey;
            PlayerList = Info.PlayerList;
            MissionType = Info.MissionType;
            GameType = Info.GameType;
            MaxPlayers = Info.MaxPlayers;
            Regions = Info.Regions;
            Version = Info.Version;
            InfoFlags = Info.InfoFlags;
            NumBots = Info.NumBots;
            CPUSpeed = Info.CPUSpeed;
            PlayerCount = Info.PlayerCount;
            Last_Heart = Info.Last_Heart;
            Last_Info = Info.Last_Info;
        }
        #endregion
    }

    public enum flags {
        running_linux = 1,
        dedicated = 2,
        passworded = 4
    }

}
