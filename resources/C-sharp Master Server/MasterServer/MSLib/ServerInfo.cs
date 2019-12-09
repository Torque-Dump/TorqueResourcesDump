using System.Collections.Generic;
using System.Net;

namespace MSLib {
    /// <summary>
    /// This class represents information of a server that is in the server list
    /// </summary>
    public class ServerInfo
    {
        #region CTOR
        public ServerInfo() {
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
        public IPEndPoint RemoteAddress { get; set; }

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

    }
}
