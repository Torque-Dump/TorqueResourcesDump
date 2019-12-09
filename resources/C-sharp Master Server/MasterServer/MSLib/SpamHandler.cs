using System.Collections.Generic;
using System.Net;

namespace MSLib {
    /// <summary>
    /// This class stores packet spamming
    /// </summary>
    public class SpamHandler
    {
        #region Fields
        public List<Spammer> SpamList = new List<Spammer>();
        #endregion

        #region CTOR
        public SpamHandler() { }
        #endregion

        #region Methods
        /// <summary>
        /// Clears the spamlist to recover the ram used
        /// </summary>
        public void FlushRam() {
            MasterServer.EventLog.LogEntry(3, " - Flushing RAM cache.");
            SpamList.Clear();
        }

        /// <summary>
        /// Gets the index of a spam entry from the passed in address and port
        /// </summary>
        /// <param name="Address"></param>
        /// <returns>-1 if the index cannot be found</returns>
        public int GetIDFromAddress(IPEndPoint Address) {
            int iIndex = -1;
            try {
                iIndex = SpamList.FindIndex(delegate(Spammer theSpammer) { return ((theSpammer.RemoteAddress.Address.ToString() == Address.ToString()) && (theSpammer.RemoteAddress.Port == Address.Port)); });
            }
            catch { }

            return iIndex;
        }
        #endregion

    }

    /// <summary>
    /// Spam entry used in the spamhandler
    /// </summary>
    public struct Spammer {
        public IPEndPoint RemoteAddress;
        public float SpamCount;
        public float BanTime;
        public uint DeltaTime;
        public uint Zeit;

        Spammer(IPEndPoint newAddress) {
            RemoteAddress = newAddress;
            SpamCount = 0;
            BanTime = 0;
            DeltaTime = 0;
            Zeit = 0;
        }
    }
}
