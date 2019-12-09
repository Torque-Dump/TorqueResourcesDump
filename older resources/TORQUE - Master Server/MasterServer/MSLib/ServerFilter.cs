using System.Collections.Generic;

namespace MSLib {
    /// <summary>
    /// Represents a filter to be used when quering the server list
    /// </summary>
    public class ServerFilter
    {
        #region CTOR
        public ServerFilter() {
            MissionType = null;
            GameType = null;
            PlayerList = null;
        }
        #endregion

        #region Properties
        public string MissionType{get; set;}
        public string GameType { get; set; }

        public int? MinPlayers{get; set;}
        public int? MaxPlayers{get; set;}
        public int? Regions { get; set; }
        public int? Version { get; set; }
        public int? FilterFlags { get; set; }
        public int? MaxBots{get; set;}
        public int? MinCPUSpeed{get; set;}

        public int? PlayerCount { get; set; }
        public List<uint> PlayerList { get; set; }
        #endregion

    }
}
