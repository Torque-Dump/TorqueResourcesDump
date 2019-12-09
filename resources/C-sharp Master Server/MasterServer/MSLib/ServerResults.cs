using System.Collections;
using System.Collections.Generic;
using System.Net;

namespace MSLib {
    /// <summary>
    /// This class contains the results of a server list query
    /// </summary>
    public class ServerResults : IEnumerable<ServerResult>
    {
        #region Fields
        public List<ServerResult> Results { get; set; }
        #endregion
        
        #region CTOR
        public ServerResults() {
            this.Results = new List<ServerResult>();
        }
        #endregion

        #region Properties
        public int Count {
            get {
                return Results.Count;
            }
        }

        #endregion

        #region Methods
        public ServerResult this[int index] {
            get {
                ServerResult sRet = null;
                if (index < this.Results.Count) {
                    sRet = Results[index];
                }
                return sRet;
            }
        }

        public virtual IEnumerator<ServerResult> GetEnumerator() {
            foreach (ServerResult serv in this.Results) {
                yield return serv;
            }
        }

        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }
        #endregion

    }

    /// <summary>
    /// Represents a chunk of server query results
    /// </summary>
    public class ServerResult : IEnumerable<IPEndPoint>
    {

        #region CTOR
        public ServerResult() {
            this.Servers = new List<IPEndPoint>();
        }
        #endregion

        #region Properties
        public List<IPEndPoint> Servers { get; set; }

        public int Count {
            get {
                return Servers.Count;
            }
        }
        #endregion

        #region Methods
        public IPEndPoint this[int index] {
            get {
                IPEndPoint sRet = null;
                if (index < this.Servers.Count) {
                    sRet = Servers[index];
                }
                return sRet;
            }
        }
        
        public virtual IEnumerator<IPEndPoint> GetEnumerator() {
            foreach (IPEndPoint serv in this.Servers) {
                yield return serv;
            }
        }

        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }
        #endregion





    }
}
