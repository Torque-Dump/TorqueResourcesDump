namespace MasterServer {
    partial class MSMonitor {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing) {
            if (disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent() {
            this.components = new System.ComponentModel.Container();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.tcStatuses = new System.Windows.Forms.TabControl();
            this.tpServers = new System.Windows.Forms.TabPage();
            this.dgvServers = new System.Windows.Forms.DataGridView();
            this.txcRemoteAddress = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.txcGameType = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.txcMissionType = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.txcMaxPlayers = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.txcRegions = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.txcVersion = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.txcNumBots = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.txcCPUSpeed = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.txcPlayerCount = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.tpSessions = new System.Windows.Forms.TabPage();
            this.dgvSessions = new System.Windows.Forms.DataGridView();
            this.txcSessRemoteAddress = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.txcSessionID = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.txcKey = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.txcCreated_Date = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.cbkFromServer = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            this.rtfMessages = new System.Windows.Forms.RichTextBox();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.startServerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.stopServerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.tmrStartup = new System.Windows.Forms.Timer(this.components);
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.tcStatuses.SuspendLayout();
            this.tpServers.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dgvServers)).BeginInit();
            this.tpSessions.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dgvSessions)).BeginInit();
            this.menuStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.tcStatuses);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.rtfMessages);
            this.splitContainer1.Size = new System.Drawing.Size(1225, 630);
            this.splitContainer1.SplitterDistance = 342;
            this.splitContainer1.TabIndex = 0;
            // 
            // tcStatuses
            // 
            this.tcStatuses.Controls.Add(this.tpServers);
            this.tcStatuses.Controls.Add(this.tpSessions);
            this.tcStatuses.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tcStatuses.Location = new System.Drawing.Point(0, 0);
            this.tcStatuses.Name = "tcStatuses";
            this.tcStatuses.SelectedIndex = 0;
            this.tcStatuses.Size = new System.Drawing.Size(1225, 342);
            this.tcStatuses.TabIndex = 0;
            // 
            // tpServers
            // 
            this.tpServers.Controls.Add(this.dgvServers);
            this.tpServers.Location = new System.Drawing.Point(4, 22);
            this.tpServers.Name = "tpServers";
            this.tpServers.Padding = new System.Windows.Forms.Padding(3);
            this.tpServers.Size = new System.Drawing.Size(1217, 316);
            this.tpServers.TabIndex = 0;
            this.tpServers.Text = "Servers";
            this.tpServers.UseVisualStyleBackColor = true;
            // 
            // dgvServers
            // 
            this.dgvServers.AllowUserToAddRows = false;
            this.dgvServers.AllowUserToDeleteRows = false;
            this.dgvServers.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dgvServers.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.txcRemoteAddress,
            this.txcGameType,
            this.txcMissionType,
            this.txcMaxPlayers,
            this.txcRegions,
            this.txcVersion,
            this.txcNumBots,
            this.txcCPUSpeed,
            this.txcPlayerCount});
            this.dgvServers.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dgvServers.Location = new System.Drawing.Point(3, 3);
            this.dgvServers.MultiSelect = false;
            this.dgvServers.Name = "dgvServers";
            this.dgvServers.ReadOnly = true;
            this.dgvServers.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.CellSelect;
            this.dgvServers.Size = new System.Drawing.Size(1211, 310);
            this.dgvServers.TabIndex = 0;
            this.dgvServers.Enter += new System.EventHandler(this.dataGridView1_Enter);
            this.dgvServers.RowEnter += new System.Windows.Forms.DataGridViewCellEventHandler(this.dataGridView1_RowEnter);
            this.dgvServers.CellStateChanged += new System.Windows.Forms.DataGridViewCellStateChangedEventHandler(this.dataGridView1_CellStateChanged);
            // 
            // txcRemoteAddress
            // 
            this.txcRemoteAddress.DataPropertyName = "RemoteAddress";
            this.txcRemoteAddress.FillWeight = 200F;
            this.txcRemoteAddress.HeaderText = "Remote Address";
            this.txcRemoteAddress.MinimumWidth = 100;
            this.txcRemoteAddress.Name = "txcRemoteAddress";
            this.txcRemoteAddress.ReadOnly = true;
            this.txcRemoteAddress.Width = 200;
            // 
            // txcGameType
            // 
            this.txcGameType.DataPropertyName = "GameType";
            this.txcGameType.HeaderText = "Game Type";
            this.txcGameType.Name = "txcGameType";
            this.txcGameType.ReadOnly = true;
            // 
            // txcMissionType
            // 
            this.txcMissionType.DataPropertyName = "MissionType";
            this.txcMissionType.HeaderText = "Mission Type";
            this.txcMissionType.Name = "txcMissionType";
            this.txcMissionType.ReadOnly = true;
            // 
            // txcMaxPlayers
            // 
            this.txcMaxPlayers.DataPropertyName = "MaxPlayers";
            this.txcMaxPlayers.HeaderText = "Max Players";
            this.txcMaxPlayers.Name = "txcMaxPlayers";
            this.txcMaxPlayers.ReadOnly = true;
            // 
            // txcRegions
            // 
            this.txcRegions.DataPropertyName = "Regions";
            this.txcRegions.HeaderText = "Region";
            this.txcRegions.Name = "txcRegions";
            this.txcRegions.ReadOnly = true;
            // 
            // txcVersion
            // 
            this.txcVersion.DataPropertyName = "Version";
            this.txcVersion.HeaderText = "Version";
            this.txcVersion.Name = "txcVersion";
            this.txcVersion.ReadOnly = true;
            // 
            // txcNumBots
            // 
            this.txcNumBots.DataPropertyName = "NumBots";
            this.txcNumBots.HeaderText = "Num Bots";
            this.txcNumBots.Name = "txcNumBots";
            this.txcNumBots.ReadOnly = true;
            // 
            // txcCPUSpeed
            // 
            this.txcCPUSpeed.DataPropertyName = "CPUSpeed";
            this.txcCPUSpeed.HeaderText = "CPU Speed";
            this.txcCPUSpeed.Name = "txcCPUSpeed";
            this.txcCPUSpeed.ReadOnly = true;
            // 
            // txcPlayerCount
            // 
            this.txcPlayerCount.DataPropertyName = "PlayerCount";
            this.txcPlayerCount.HeaderText = "Player Count";
            this.txcPlayerCount.Name = "txcPlayerCount";
            this.txcPlayerCount.ReadOnly = true;
            // 
            // tpSessions
            // 
            this.tpSessions.Controls.Add(this.dgvSessions);
            this.tpSessions.Location = new System.Drawing.Point(4, 22);
            this.tpSessions.Name = "tpSessions";
            this.tpSessions.Padding = new System.Windows.Forms.Padding(3);
            this.tpSessions.Size = new System.Drawing.Size(1217, 303);
            this.tpSessions.TabIndex = 1;
            this.tpSessions.Text = "Sessions";
            this.tpSessions.UseVisualStyleBackColor = true;
            // 
            // dgvSessions
            // 
            this.dgvSessions.AllowUserToAddRows = false;
            this.dgvSessions.AllowUserToDeleteRows = false;
            this.dgvSessions.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dgvSessions.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.txcSessRemoteAddress,
            this.txcSessionID,
            this.txcKey,
            this.txcCreated_Date,
            this.cbkFromServer});
            this.dgvSessions.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dgvSessions.Location = new System.Drawing.Point(3, 3);
            this.dgvSessions.MultiSelect = false;
            this.dgvSessions.Name = "dgvSessions";
            this.dgvSessions.ReadOnly = true;
            this.dgvSessions.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.CellSelect;
            this.dgvSessions.Size = new System.Drawing.Size(1211, 297);
            this.dgvSessions.TabIndex = 1;
            this.dgvSessions.Enter += new System.EventHandler(this.dataGridView1_Enter);
            this.dgvSessions.RowEnter += new System.Windows.Forms.DataGridViewCellEventHandler(this.dataGridView1_RowEnter);
            this.dgvSessions.CellStateChanged += new System.Windows.Forms.DataGridViewCellStateChangedEventHandler(this.dataGridView1_CellStateChanged);
            this.dgvSessions.DataError += new System.Windows.Forms.DataGridViewDataErrorEventHandler(this.dgvSessions_DataError);
            // 
            // txcSessRemoteAddress
            // 
            this.txcSessRemoteAddress.DataPropertyName = "RemoteAddress";
            this.txcSessRemoteAddress.FillWeight = 200F;
            this.txcSessRemoteAddress.HeaderText = "Remote Address";
            this.txcSessRemoteAddress.MinimumWidth = 100;
            this.txcSessRemoteAddress.Name = "txcSessRemoteAddress";
            this.txcSessRemoteAddress.ReadOnly = true;
            this.txcSessRemoteAddress.Width = 200;
            // 
            // txcSessionID
            // 
            this.txcSessionID.DataPropertyName = "SessionID";
            this.txcSessionID.HeaderText = "Session ID";
            this.txcSessionID.Name = "txcSessionID";
            this.txcSessionID.ReadOnly = true;
            // 
            // txcKey
            // 
            this.txcKey.DataPropertyName = "Key";
            this.txcKey.HeaderText = "Key";
            this.txcKey.Name = "txcKey";
            this.txcKey.ReadOnly = true;
            // 
            // txcCreated_Date
            // 
            this.txcCreated_Date.DataPropertyName = "Created_Date";
            this.txcCreated_Date.HeaderText = "Created";
            this.txcCreated_Date.Name = "txcCreated_Date";
            this.txcCreated_Date.ReadOnly = true;
            // 
            // cbkFromServer
            // 
            this.cbkFromServer.DataPropertyName = "FromServer";
            this.cbkFromServer.HeaderText = "From Server";
            this.cbkFromServer.Name = "cbkFromServer";
            this.cbkFromServer.ReadOnly = true;
            // 
            // rtfMessages
            // 
            this.rtfMessages.BackColor = System.Drawing.Color.Black;
            this.rtfMessages.Dock = System.Windows.Forms.DockStyle.Fill;
            this.rtfMessages.ForeColor = System.Drawing.Color.Yellow;
            this.rtfMessages.Location = new System.Drawing.Point(0, 0);
            this.rtfMessages.Name = "rtfMessages";
            this.rtfMessages.ReadOnly = true;
            this.rtfMessages.Size = new System.Drawing.Size(1225, 284);
            this.rtfMessages.TabIndex = 0;
            this.rtfMessages.Text = "";
            // 
            // menuStrip1
            // 
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.startServerToolStripMenuItem,
            this.stopServerToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(1225, 24);
            this.menuStrip1.TabIndex = 1;
            this.menuStrip1.Text = "menuStrip1";
            this.menuStrip1.Visible = false;
            // 
            // startServerToolStripMenuItem
            // 
            this.startServerToolStripMenuItem.Name = "startServerToolStripMenuItem";
            this.startServerToolStripMenuItem.Size = new System.Drawing.Size(71, 20);
            this.startServerToolStripMenuItem.Text = "Start Server";
            this.startServerToolStripMenuItem.Click += new System.EventHandler(this.startServerToolStripMenuItem_Click);
            // 
            // stopServerToolStripMenuItem
            // 
            this.stopServerToolStripMenuItem.Name = "stopServerToolStripMenuItem";
            this.stopServerToolStripMenuItem.Size = new System.Drawing.Size(71, 20);
            this.stopServerToolStripMenuItem.Text = "Stop Server";
            this.stopServerToolStripMenuItem.Click += new System.EventHandler(this.stopServerToolStripMenuItem_Click);
            // 
            // tmrStartup
            // 
            this.tmrStartup.Enabled = true;
            this.tmrStartup.Interval = 1000;
            this.tmrStartup.Tick += new System.EventHandler(this.tmrStartup_Tick);
            // 
            // MSMonitor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1225, 630);
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.menuStrip1);
            this.Name = "MSMonitor";
            this.Text = "MS Monitor";
            this.Load += new System.EventHandler(this.MSMonitor_Load);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.tcStatuses.ResumeLayout(false);
            this.tpServers.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.dgvServers)).EndInit();
            this.tpSessions.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.dgvSessions)).EndInit();
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.RichTextBox rtfMessages;
        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem startServerToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem stopServerToolStripMenuItem;
        private System.Windows.Forms.TabControl tcStatuses;
        private System.Windows.Forms.TabPage tpServers;
        private System.Windows.Forms.DataGridView dgvServers;
        private System.Windows.Forms.TabPage tpSessions;
        private System.Windows.Forms.DataGridView dgvSessions;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcRemoteAddress;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcGameType;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcMissionType;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcMaxPlayers;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcRegions;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcVersion;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcNumBots;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcCPUSpeed;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcPlayerCount;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcSessRemoteAddress;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcSessionID;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcKey;
        private System.Windows.Forms.DataGridViewTextBoxColumn txcCreated_Date;
        private System.Windows.Forms.DataGridViewCheckBoxColumn cbkFromServer;
        private System.Windows.Forms.Timer tmrStartup;
    }
}

