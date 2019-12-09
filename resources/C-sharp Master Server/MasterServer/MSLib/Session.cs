using System;
using System.Net;

namespace MSLib {
    /// <summary>
    /// This class represents a cominication session between the server and the master server
    /// </summary>
    public class Session
    {
        #region CTOR
        public Session() {
            this.RemoteAddress = null;
            this.Results = null;
        }
        #endregion

        #region Properties
        public IPEndPoint RemoteAddress { get; set; }

        public ushort SessionID { get; set; }

        public ushort Key { get; set; }

        public DateTime Created_Date { get; set; }

        public bool FromServer { get; set; }

        public ServerResults Results { get; set; }
        #endregion
    }
}
