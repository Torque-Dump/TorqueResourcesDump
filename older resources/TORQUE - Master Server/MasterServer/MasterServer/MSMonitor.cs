using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

using MSLib;

namespace MasterServer {
    public partial class MSMonitor : Form
    {
        #region Fields
        MSLib.MasterServer theMS = new MSLib.MasterServer();
        List<Server> _lstServers;
        List<Session> _lstSessions;
        #endregion

        #region CTOR
        public MSMonitor() {
            InitializeComponent();

            
        }
        #endregion

        #region Methods
        private void BindServerList() {
            if (_lstServers == null) {
                _lstServers = new List<Server>();
                _lstServers.AddRange(MSLib.MasterServer.Server_Store.Servers);
            }

            if (this.dgvServers.InvokeRequired) {
                this.dgvServers.Invoke(new MethodInvoker(delegate { this.dgvServers.AutoGenerateColumns = false; }));
                this.dgvServers.Invoke(new MethodInvoker(delegate{this.dgvServers.DataSource = null;}));
                this.dgvServers.Invoke(new MethodInvoker(delegate { this.dgvServers.DataSource = _lstServers; }));
            }
            else {
                this.dgvServers.AutoGenerateColumns = false;
                this.dgvServers.DataSource = null;
                this.dgvServers.DataSource = _lstServers;
            }
            
        }
        
        private void BindSessionList()
        {
            if (_lstSessions == null)
            {
                _lstSessions = new List<Session>();
                _lstSessions.AddRange(MSLib.MasterServer.Sessions.Sessions);
            }

            if (this.dgvSessions.InvokeRequired)
            {
                this.dgvSessions.Invoke(new MethodInvoker(delegate { this.dgvSessions.AutoGenerateColumns = false; }));
                this.dgvSessions.Invoke(new MethodInvoker(delegate { this.dgvSessions.DataSource = null; }));
                this.dgvSessions.Invoke(new MethodInvoker(delegate { this.dgvSessions.DataSource = _lstSessions; }));
            }
            else
            {
                this.dgvSessions.AutoGenerateColumns = false;
                this.dgvSessions.DataSource = null;
                this.dgvSessions.DataSource = _lstSessions;
            }
        }

        private void StartMasterServer()
        {
            this.stopServerToolStripMenuItem.Enabled = true;
            this.startServerToolStripMenuItem.Enabled = false;

            theMS.StartMasterServer();
            this.BindServerList();
            this.BindSessionList();
        }

        private void StopMasterServer()
        {
            this.stopServerToolStripMenuItem.Enabled = false;
            this.startServerToolStripMenuItem.Enabled = true;

            theMS.StopMasterServer();

            this._lstServers = null;
            this._lstSessions = null;
        }
        #endregion

        #region Events
        private void MSMonitor_Load(object sender, EventArgs e) {
            MSLib.MasterServer.EventLog.NewLogEntry += new MSLib.Logging.Logger.NewLogEntryHandler(EventLog_NewLogEntry);

            this.dgvServers.AutoGenerateColumns = false;
            this.dgvSessions.AutoGenerateColumns = false;

            this.stopServerToolStripMenuItem.Enabled = false;
            this.startServerToolStripMenuItem.Enabled = true;

            MSLib.MasterServer.Server_Store.NewServerEntry += new ServerStore.ServerEntryHandler(Server_Store_NewServerEntry);
            MSLib.MasterServer.Server_Store.UpdateServerEntry += new ServerStore.ServerEntryHandler(Server_Store_UpdateServerEntry);
            MSLib.MasterServer.Server_Store.RemoveServerEntry += new ServerStore.ServerEntryHandler(Server_Store_RemoveServerEntry);

            MSLib.MasterServer.Sessions.NewSessionEntry += new SessionHandler.SessionEntryHandler(Sessions_NewSessionEntry);
            MSLib.MasterServer.Sessions.UpdateSessionEntry += new SessionHandler.SessionEntryHandler(Sessions_UpdateSessionEntry);
            MSLib.MasterServer.Sessions.RemoveSessionEntry += new SessionHandler.SessionEntryHandler(Sessions_RemoveSessionEntry);

        }

        void Sessions_RemoveSessionEntry(Session oSession)
        {
            Session sessionToFind = null;
            try
            {
                sessionToFind = _lstSessions.Find(t =>
                    ((t.RemoteAddress.Address.ToString() == oSession.RemoteAddress.Address.ToString()) &&
                    (t.RemoteAddress.Port == oSession.RemoteAddress.Port)) && (t.SessionID == oSession.SessionID && t.Key == oSession.Key));
            }
            catch { }

            this.BindingContext[dgvSessions.DataSource].SuspendBinding();            
            if (sessionToFind != null)
            {
                _lstSessions.Remove(sessionToFind);
            }
            this.BindSessionList();
            this.BindingContext[dgvSessions.DataSource].ResumeBinding();           
        }

        void Sessions_UpdateSessionEntry(Session oSession)
        {
            Session sessionToFind = null;
            try
            {
                sessionToFind = _lstSessions.Find(t => (t.RemoteAddress.Address.ToString() == oSession.RemoteAddress.Address.ToString()));
            }
            catch { }

            this.BindingContext[dgvSessions.DataSource].SuspendBinding();
            if (sessionToFind != null)
            {                
                sessionToFind.FromServer = oSession.FromServer;
                sessionToFind.Created_Date = oSession.Created_Date;
                sessionToFind.Key = oSession.Key;
                sessionToFind.Results = oSession.Results;
                sessionToFind.SessionID = oSession.Key;
            }
            this.BindSessionList();
            this.BindingContext[dgvSessions.DataSource].ResumeBinding();
        }

        void Sessions_NewSessionEntry(Session oSession)
        {
            //Lets make sure its not in the list already
            Session sessionToFind = null;
            try
            {
                sessionToFind = _lstSessions.Find(t => 
                    ((t.RemoteAddress.Address.ToString() == oSession.RemoteAddress.Address.ToString()) && 
                    (t.RemoteAddress.Port == oSession.RemoteAddress.Port)) && (t.SessionID == oSession.SessionID && t.Key == oSession.Key));
            }
            catch { }

            this.BindingContext[dgvSessions.DataSource].SuspendBinding();            
            if (sessionToFind == null)
            {
                _lstSessions.Add(oSession);
            }
            this.BindSessionList();
            this.BindingContext[dgvSessions.DataSource].ResumeBinding();
        }

        void Server_Store_RemoveServerEntry(Server oServer) {
            Server serverToFind = null;
            try {
                serverToFind = _lstServers.Find(t => (t.RemoteAddress.Address.ToString() == oServer.RemoteAddress.Address.ToString()) && (t.RemoteAddress.Port == oServer.RemoteAddress.Port));
            }
            catch { }

            this.BindingContext[dgvServers.DataSource].SuspendBinding();
            if (serverToFind != null) {
                _lstServers.Remove(serverToFind);
            }
            this.BindServerList();
            this.BindingContext[dgvServers.DataSource].ResumeBinding();
        }

        void Server_Store_UpdateServerEntry(Server oServer) {
            Server serverToFind = null;
            try {
                serverToFind = _lstServers.Find(t => (t.RemoteAddress.Address.ToString() == oServer.RemoteAddress.Address.ToString()) && (t.RemoteAddress.Port == oServer.RemoteAddress.Port));
            }
            catch { }

            this.BindingContext[dgvServers.DataSource].SuspendBinding();
            if (serverToFind != null) {
                serverToFind.CPUSpeed = oServer.CPUSpeed;
                serverToFind.GameType = oServer.GameType;
                serverToFind.InfoFlags = oServer.InfoFlags;
                serverToFind.Last_Heart = oServer.Last_Heart;
                serverToFind.Last_Info = oServer.Last_Info;
                serverToFind.MaxPlayers = oServer.MaxPlayers;
                serverToFind.MissionType = oServer.MissionType;
                serverToFind.NumBots = oServer.NumBots;
                serverToFind.PlayerCount = oServer.PlayerCount;
                serverToFind.PlayerList = oServer.PlayerList;
                serverToFind.Regions = oServer.Regions;
                serverToFind.Version = oServer.Version;                
            }

            this.BindServerList();
            this.BindingContext[dgvServers.DataSource].ResumeBinding();
        }

        void Server_Store_NewServerEntry(Server oServer) {
            //Lets make sure its not in the list already
            Server serverToFind = null;
            try {
                serverToFind = _lstServers.Find(t => (t.RemoteAddress.Address.ToString() == oServer.RemoteAddress.Address.ToString()) && (t.RemoteAddress.Port == oServer.RemoteAddress.Port));
            }
            catch { }

            this.BindingContext[dgvServers.DataSource].SuspendBinding();
            if (serverToFind == null) {
                _lstServers.Add(oServer);
            }
            this.BindServerList();
            this.BindingContext[dgvServers.DataSource].ResumeBinding();
        }
        
        void EventLog_NewLogEntry(string sMessage) {
            if (this.rtfMessages.InvokeRequired) {
                this.rtfMessages.Invoke(new MethodInvoker(delegate { this.rtfMessages.AppendText(sMessage + Environment.NewLine); }));
                this.rtfMessages.Invoke(new MethodInvoker(delegate { this.rtfMessages.ScrollToCaret(); }));
            }
            else {
                this.rtfMessages.AppendText(sMessage + Environment.NewLine);
                this.rtfMessages.ScrollToCaret();
            }
        }
        
        private void startServerToolStripMenuItem_Click(object sender, EventArgs e) {
            this.StartMasterServer();
        }

        private void stopServerToolStripMenuItem_Click(object sender, EventArgs e) {
            this.StopMasterServer();
        }
        
        private void dataGridView1_Enter(object sender, EventArgs e) {
            ((DataGridView)sender).ClearSelection();
        }

        private void dataGridView1_RowEnter(object sender, DataGridViewCellEventArgs e) {
            ((DataGridView)sender).ClearSelection();
        }

        private void dataGridView1_CellStateChanged(object sender, DataGridViewCellStateChangedEventArgs e) {
            if (e.StateChanged == DataGridViewElementStates.Selected)
                e.Cell.Selected = false;
        }
        #endregion

        private void dgvSessions_DataError(object sender, DataGridViewDataErrorEventArgs e)
        {
            e.Cancel = true;
        }

        private void tmrStartup_Tick(object sender, EventArgs e)
        {
            tmrStartup.Enabled = false;
            this.StartMasterServer();
            
        }



        



    }
}
