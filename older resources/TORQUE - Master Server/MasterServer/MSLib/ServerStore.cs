using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;

namespace MSLib {
    /// <summary>
    /// This class stores the servers (in memory) that have registered with the master server
    /// </summary>
    public class ServerStore
    {
        #region Event and Handler delegates
        public delegate void ServerEntryHandler(Server oServer);

        public event ServerEntryHandler NewServerEntry;
        public event ServerEntryHandler UpdateServerEntry;
        public event ServerEntryHandler RemoveServerEntry;
        #endregion
               
        #region Fields
        public List<Server> Servers { get; set; }
        long heartbeat_to = MasterServer.ServerPreferences.Heatbeat_To * 1000;
        #endregion

        #region CTOR
        public ServerStore() {
            Servers = new List<Server>();
        }
        #endregion

        #region Properties
        public int Count {
            get {
                return Servers.Count;
            }
        }

        #endregion

        #region Methods
        /// <summary>
        /// Inserts the server into the list
        /// </summary>
        /// <param name="server"></param>
        public void InsertServer(Server server){
            if (!this.Servers.Contains(server)) {
                MasterServer.EventLog.LogEntry(3, "Server added to list");
                Servers.Add(server);
                this.FireNewServerEntry(server);
            }
            else {
                MasterServer.EventLog.LogEntry(3, "Could not insert server to list, it already exists");
            }
        }

        /// <summary>
        /// Removes the server from the list
        /// </summary>
        /// <param name="server"></param>
        public void RemoveServer(Server server){
            if (this.Count == 0)
                return;

            Server theServer = FindServer(server.RemoteAddress);

            if (theServer != null) {
                this.FireRemoveServerEntry(theServer);
                Servers.Remove(theServer);
            }
        }

        /// <summary>
        /// Finds the server in the list
        /// </summary>
        /// <param name="RemoteAddress"></param>
        /// <returns></returns>
        public Server FindServer(IPEndPoint RemoteAddress) {
            Server oReturn = null;

            try {
                oReturn = Servers.Find(t => t.RemoteAddress.Address.ToString() == RemoteAddress.Address.ToString() && t.RemoteAddress.Port == RemoteAddress.Port);
            }
            catch { }

            return oReturn;
        }

        /// <summary>
        ///Adds or updates the server in the list with the last heartbeat information 
        /// </summary>
        /// <param name="RemoteAddress"></param>
        public void HeartbeatServer(IPEndPoint RemoteAddress) {
            Server oServer = FindServer(RemoteAddress);
            if (oServer != null) {
                oServer.Last_Heart = DateTime.Now.Ticks;
                this.FireUpdateServerEntry(oServer);
                MasterServer.EventLog.LogEntry(1, "Updated server heartbeat...");
                return;
            }

            MasterServer.EventLog.LogEntry(1, string.Format("Creating new entry in server database for {0}:{1}...", RemoteAddress.ToString(), RemoteAddress.Port));

            // Ok, we didn't find a match, so add one.
            Server newServer = new Server();
            //newServer.RemoteInfo.Last_Heart = DateTime.Now.Ticks;
            newServer.Last_Heart = DateTime.Now.Ticks;
            newServer.RemoteAddress = RemoteAddress;
            InsertServer(newServer);
        }

        /// <summary>
        /// Updates the server in the list with the updated information
        /// </summary>
        /// <param name="RemoteAddress"></param>
        /// <param name="info"></param>
        public void UpdateServer(IPEndPoint RemoteAddress, ServerInfo info) {
            Server oServer = FindServer(RemoteAddress);
            if (oServer != null) {
                info.Last_Heart = DateTime.Now.Ticks;
                info.Last_Info = DateTime.Now.Ticks;

                oServer.UpdateServerInfo(info);
                FireUpdateServerEntry(oServer);
            }
        }

        /// <summary>
        /// Helper function for serverquery_callback
        /// </summary>
        /// <param name="r">Results set to add to.</param>
        /// <param name="d">Address to add.</param>
        public void AddServerData(ref ServerResults r, IPEndPoint d) {

            //Build up the server data into chuncks that can be sent
            foreach (ServerResult servRes in r.Results) {
                if (servRes.Count < PacketConfig.LIST_PACKET_MAX_SERVERS) {
                    //Do we need to allocate a new chunk?
                    if (servRes.Count == PacketConfig.LIST_PACKET_MAX_SERVERS) {
                        ServerResult theNewResult = new ServerResult();
                        theNewResult.Servers.Add(d);
                        r.Results.Add(theNewResult);

                        //No need to go any further
                        break;
                    }
                    else {
                        servRes.Servers.Add(d);

                        break;
                    }
                }
            }

        }

        /// <summary>
        /// Queries the server list and returns the results of the query
        /// </summary>
        /// <param name="filter"></param>
        /// <returns></returns>
        public ServerResults QueryServers(ServerFilter filter) {
            ServerResults oReturn = new ServerResults();

            List<Server> tmpServers = new List<Server>();
            tmpServers.AddRange(Servers);
            foreach(Server svr in tmpServers){
                bool bRemoveServer = false;

                //Heartbeat
                if ((DateTime.Now.Ticks - svr.Last_Heart) > heartbeat_to) {
                    bRemoveServer = true;
                    //Server is passed its heartbeat so remove it from the master list
                    RemoveServer(svr);
                    continue;
                }

                if ((bRemoveServer == false) && (filter.MaxBots != null)) {
                    if (svr.NumBots > filter.MaxBots)
                        bRemoveServer = true;
                }

                if ((bRemoveServer == false) && (filter.GameType != null)) {
                    if (svr.GameType != filter.GameType)
                        bRemoveServer = true;
                }

                if ((bRemoveServer == false) && (filter.MaxPlayers != 100)) {
                    if (svr.PlayerCount > filter.MaxPlayers)
                        bRemoveServer = true;
                }

                if ((bRemoveServer == false) && (filter.MinPlayers != null)) {
                    if (svr.PlayerCount < filter.MinPlayers)
                        bRemoveServer = true;
                }

                if ((bRemoveServer == false) && (filter.MinCPUSpeed > 100)) {
                    if (svr.CPUSpeed < filter.MinCPUSpeed)
                        bRemoveServer = true;
                }

                if ((bRemoveServer == false) && (filter.MissionType != null && filter.MissionType != "Any")) {
                    if (svr.MissionType != filter.MissionType)
                    bRemoveServer = true;
                }

                if ((bRemoveServer == false) && (filter.FilterFlags != null)) {
                    // We have to deal with each one individually.

                    // According to serverQuery.cc:
                    /*      Dedicated         = BIT(0),
                            NotPassworded     = BIT(1),
                            Linux	          = BIT(2),
                            CurrentVersion    = BIT(7) */
                    // Now, I don't know what CurrentVersion means,
                    // but the rest are pretty easy.

                    if ((svr.InfoFlags & filter.FilterFlags) != filter.FilterFlags)
                        bRemoveServer = true;
                }

                //Do we need to remove this server from the list
                if (bRemoveServer) {
                    svr.RemoveServer = true;
                }
            }


            

            //Prepare the results
            foreach (Server svr in tmpServers) {
                if (!svr.RemoveServer) {
                    AddServerData(ref oReturn, svr.RemoteAddress);
                }
            }

            return oReturn;
        }

        /// <summary>
        /// Normally the server list gets culled when a server list is requested.
        /// This can be called to cull servers without having to rely on a list request.
        /// </summary>
        public void ProcessExpiredServers()
        {
            try
            {
                List<Server> theExpiredServers = this.Servers.FindAll(t => (DateTime.Now.Ticks - t.Last_Heart) > heartbeat_to);
                foreach (Server theServer in theExpiredServers)
                {
                    this.RemoveServer(theServer);
                }
            }
            catch { }
        }

        /// <summary>
        /// Returns a unique list of game types from the active servers
        /// </summary>
        /// <returns>IList of game types</returns>
        public IList<string> GetGameTypes() {
            SortedList<string, string> slGameTypes = new SortedList<string, string>();
            foreach (Server serv in this.Servers) {
                try {
                    if (!slGameTypes.ContainsKey(serv.GameType)) {
                        slGameTypes.Add(serv.GameType, serv.GameType);
                    }
                }
                catch { }
            }

            return slGameTypes.Keys;
        }

        /// <summary>
        /// Returns a unique list of mission types from the active servers
        /// </summary>
        /// <returns>IList of mission types</returns>
        public IList<string> GetMissionTypes() {
            SortedList<string, string> slMissionTypes = new SortedList<string, string>();
            foreach (Server serv in this.Servers) {
                try {
                    if (!slMissionTypes.ContainsKey(serv.MissionType)) {
                        slMissionTypes.Add(serv.MissionType, serv.MissionType);
                    }
                }
                catch { }
            }

            return slMissionTypes.Keys;
        }

        public void FireNewServerEntry(Server oServer) {
            if (NewServerEntry != null) {
                NewServerEntry(oServer);
            }
        }
        public void FireUpdateServerEntry(Server oServer) {
            if (UpdateServerEntry != null) {
                UpdateServerEntry(oServer);
            }
        }
        public void FireRemoveServerEntry(Server oServer) {
            if (RemoveServerEntry != null) {
                RemoveServerEntry(oServer);
            }
        }

        #endregion
    }
}
