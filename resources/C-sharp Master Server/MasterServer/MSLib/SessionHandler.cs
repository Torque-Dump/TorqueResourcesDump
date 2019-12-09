using System;
using System.Collections.Generic;
using System.Net;

namespace MSLib {
    /// <summary>
    /// This class stores and handles sessions
    /// </summary>
    public class SessionHandler
    {
        #region Event and handler delegates
        public delegate void SessionEntryHandler(Session oSession);

        public event SessionEntryHandler NewSessionEntry;
        public event SessionEntryHandler UpdateSessionEntry;
        public event SessionEntryHandler RemoveSessionEntry;
        #endregion

        #region Fields
        public List<Session> Sessions { get; set; }

        #endregion

        #region CTOR
        public SessionHandler() {
            this.Sessions = new List<Session>();
        }
        #endregion

        #region Properties
        public int Count {
            get {
                return Sessions.Count;
            }
        }
        #endregion

        #region Methods
        public Session this[int index]
        {
            get
            {
                Session sRet = null;
                if (index < this.Sessions.Count)
                {
                    sRet = Sessions[index];
                }
                return sRet;
            }
        }

        /// <summary>
        /// Start a client session.
        /// </summary>
        /// <param name="who">Address of session user.</param>
        /// <param name="session">A magic number identifying the user.</param>
        /// <param name="key">Key - all packets must be >= key.</param>
        /// <param name="res">ServerResults, if any, to store in the session.</param>
        public Session StartSession(IPEndPoint RemoteAddress, ushort session, ushort key, ServerResults res) {
            return StartSession(RemoteAddress, session, key, res, false);
        }

        /// <summary>
        /// Start a server or client session.
        /// </summary>
        /// <param name="who">Address of session user.</param>
        /// <param name="session">A magic number identifying the user.</param>
        /// <param name="key">Key - all packets must be >= key.</param>
        /// <param name="res">ServerResults, if any, to store in the session.</param>
        /// <param name="fromServer">If true, then _we_ started this, otherwise, they did.</param>
        public Session StartSession(IPEndPoint RemoteAddress, ushort session, ushort key, ServerResults res, bool fromServer) {
            // Add the session to the list.
            Session newS = new Session();

            // Fill in data
            newS.Created_Date = DateTime.Now;
            newS.SessionID = session;
            newS.Key = key;
            newS.Results = res;
            newS.RemoteAddress = RemoteAddress;
            newS.FromServer = fromServer;

            this.Sessions.Add(newS);
            this.FireNewSessionEntry(newS);

            MasterServer.EventLog.LogEntry(3, string.Format("Starting new session. Session: {0}, Key: {1}, Address: {2}", newS.SessionID, newS.Key, newS.RemoteAddress.Address.ToString()));

            return newS;
        }

        /// <summary>
        /// Helper function to remove old sessions.
        /// </summary>
        /// <param name="s">Session to consider for expiration.</param>
        private void Process(Session s) {
            // Expire things.
            if (s == null) return;

            if (DateTime.Now > (s.Created_Date.AddSeconds(30.0f))) {
                MasterServer.EventLog.LogEntry(2, String.Format("Expiring a session. Session: {0}, Key: {1}, Address: {2}", s.SessionID, s.Key, s.RemoteAddress.Address.ToString()));

                bool bSuccess = this.Sessions.Remove(s);
                if (!bSuccess)
                {
                    MasterServer.EventLog.LogEntry(3, "Could not remove session.");
                }
                else
                {
                    this.FireRemoveSessionEntry(s);
                }
            }
        }
        
        /// <summary>
        /// Process the next 5 sessions.
        /// </summary>
        public void DoProcessing() {
            this.DoProcessing(5);
        }

        /// <summary>
        /// Process the next n sessions.
        /// </summary>
        /// <param name="n">Number of sessions to process.</param>
        public void DoProcessing(int n){
            if (this.Count > 0) {
                for (int i = 0; i < n; i++)
                    Process(this[i]);
            }
        }

        /// <summary>
        /// Mark us as having started a session with someone.
        /// 
        /// We return session and key to be transmitted.
        /// 
        /// This is called when we do game info queries (in response to heartbeats).
        /// Originally (see game/net/serverQuery.cc:808), this would probably have
        /// been the ping session again, but the code just treats session and key
        /// as a 32 bit number and blasts them back to us. So this way we save
        /// some effort, and it also makes us a little less easy to spoof (as
        /// they have to guess our number as well as the client's.)
        /// </summary>
        /// <param name="RemoteAddress">Address of session person.</param>
        public Session MakeSession(IPEndPoint RemoteAddress) {
            // Check for an existing session from this person
            Session cur = this.FindSession(RemoteAddress);

            if (cur != null) {
                // On match, return results, update TTL
                 cur.Created_Date = DateTime.Now;
                this.FireUpdateSessionEntry(cur);
            }
            else {
                // No match! How'd'ya like that?
                Random autoRand = new Random();
                ushort session = (ushort)autoRand.Next(ushort.MaxValue);
                ushort key = (ushort)autoRand.Next(ushort.MaxValue);
                cur = StartSession(RemoteAddress, session, key, null, true);
            }

            return cur;
        }

        /// <summary>
        /// Finds a session from the ip address and port
        /// </summary>
        /// <param name="RemoteAddress"></param>
        /// <returns></returns>
        public Session FindSession(IPEndPoint RemoteAddress) {
            Session oReturn = null;

            try {
                oReturn = this.Sessions.Find(t => 
                    (t.RemoteAddress.Address.ToString() == RemoteAddress.Address.ToString()) && 
                    (t.RemoteAddress.Port == RemoteAddress.Port)
                    );
                MasterServer.EventLog.LogEntry(3, string.Format("Found Session. Session: {0}, Key: {1}, Address: {2}", oReturn.SessionID, oReturn.Key, oReturn.RemoteAddress.Address.ToString()));
            }
            catch {
                MasterServer.EventLog.LogEntry(3, string.Format("Could not find session for address: {0}:{1}", RemoteAddress.Address.ToString(), RemoteAddress.Port));
            }

            return oReturn;
        }

        /// <summary>
        /// Wrapper function to check on a valid session.
        /// </summary>
        /// <param name="who">Address we received packet from.</param>
        /// <param name="session">Session from packet</param>
        /// <param name="key">Key from the packet.</param>
        /// <returns>true if the session is valid.</returns>
        public bool ValidSession(IPEndPoint RemoteAddress, ushort session, ushort key) {
            // Simple wrapper for external use.
            Session g = null;
            return ValidSession(RemoteAddress, session, key, g);
        }

        /// <summary>
        /// Check for a valid session.
        /// 
        /// The logic:
	    ///	- we haven't seen anything from who before, so...
	    ///		o we note down their session and key
	    ///		o return true
	    ///	- we saw something from who before, with this session,
	    ///				and their key is greater than before.
	    ///		o we update last-seen
	    ///		o return true
	    ///	- else
        ///		o return false
        /// </summary>
        /// <param name="who">Address we received packet from.</param>
        /// <param name="session">Session from packet</param>
        /// <param name="key">Key from the packet.</param>
        /// <param name="which">Used to (optionally) return a handle to a valid session. NULL causes nothin to be returned.</param>
        /// <returns>true if the session is valid.</returns>
        public bool ValidSession(IPEndPoint RemoteAddress, ushort session, ushort key, Session which) {
            Session theSession = null;
            try {
                theSession = this.Sessions.Find(t => (t.RemoteAddress.Address.ToString() == RemoteAddress.Address.ToString()) && (t.RemoteAddress.Port == RemoteAddress.Port) && (t.SessionID == session) && (key > t.Key));
            }
            catch { }

            return (theSession != null);

        }

        /// <summary>
        /// Deal with a packet resend request from a user.
        /// </summary>
        /// <param name="who"></param>
        /// <param name="session"></param>
        /// <param name="key"></param>
        /// <param name="which"></param>
        public void FesendSessionPacket(IPEndPoint RemoteAddress, ushort session, ushort key, int which) {
            // Validate session
            Session g = null;

            if (!ValidSession(RemoteAddress, session, key, g)) {
                MasterServer.EventLog.LogEntry(1, "Got a bad session from someone.\n");
                return; // No good!
            }

            // Now do the sending (if we got valid)
            //TODO:
            //sendListResponse(who, session, key, g.res, which);
        }


        public void FireNewSessionEntry(Session oSession)
        {
            if (NewSessionEntry != null)
            {
                NewSessionEntry(oSession);
            }
        }
        public void FireUpdateSessionEntry(Session oSession)
        {
            if (UpdateSessionEntry != null)
            {
                UpdateSessionEntry(oSession);
            }
        }
        public void FireRemoveSessionEntry(Session oSession)
        {
            if (RemoveSessionEntry != null)
            {
                RemoveSessionEntry(oSession);
            }
        }        
        
        #endregion

    }
}
