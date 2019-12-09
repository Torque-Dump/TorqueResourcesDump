using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;

namespace MSLib {
    public class ServerResults : IEnumerable<ServerResult> {
        public List<ServerResult> Results { get; set; }
        public ServerResults() {
            this.Results = new List<ServerResult>();
        }

        public ServerResult this[int index] {
            get {
                ServerResult sRet = null;
                if (index < this.Results.Count) {
                    sRet = Results[index];
                }
                return sRet;
            }
        }

        public int Count {
            get {
                return Results.Count;
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
    }

    public class ServerResult : IEnumerable<IPEndPoint> {
        public ServerResult() {
            this.Servers = new List<IPEndPoint>();
        }
        public List<IPEndPoint> Servers { get; set; }

        public IPEndPoint this[int index] {
            get {
                IPEndPoint sRet = null;
                if (index < this.Servers.Count) {
                    sRet = Servers[index];
                }
                return sRet;
            }
        }

        public int Count {
            get {
                return Servers.Count;
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

    }
}
