namespace MSLib.Forms {
    partial class frmServerStatus {
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
            this.tcServer = new System.Windows.Forms.TabControl();
            this.tpServers = new System.Windows.Forms.TabPage();
            this.dataGridView1 = new System.Windows.Forms.DataGridView();
            this.tpSessions = new System.Windows.Forms.TabPage();
            this.dataGridView2 = new System.Windows.Forms.DataGridView();
            this.vAddressDataGridViewTextBoxColumn = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.vInfoDataGridViewTextBoxColumn = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.serverStoreBindingSource = new System.Windows.Forms.BindingSource(this.components);
            this.whoDataGridViewTextBoxColumn = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.sessionDataGridViewTextBoxColumn = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.keyDataGridViewTextBoxColumn = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.createddateDataGridViewTextBoxColumn = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.fromServerDataGridViewCheckBoxColumn = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            this.resDataGridViewTextBoxColumn = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.nextDataGridViewTextBoxColumn = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.sessionHandlerBindingSource = new System.Windows.Forms.BindingSource(this.components);
            this.tcServer.SuspendLayout();
            this.tpServers.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView1)).BeginInit();
            this.tpSessions.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView2)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.serverStoreBindingSource)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.sessionHandlerBindingSource)).BeginInit();
            this.SuspendLayout();
            // 
            // tcServer
            // 
            this.tcServer.Controls.Add(this.tpServers);
            this.tcServer.Controls.Add(this.tpSessions);
            this.tcServer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tcServer.Location = new System.Drawing.Point(0, 0);
            this.tcServer.Name = "tcServer";
            this.tcServer.SelectedIndex = 0;
            this.tcServer.Size = new System.Drawing.Size(889, 446);
            this.tcServer.TabIndex = 0;
            // 
            // tpServers
            // 
            this.tpServers.Controls.Add(this.dataGridView1);
            this.tpServers.Location = new System.Drawing.Point(4, 22);
            this.tpServers.Name = "tpServers";
            this.tpServers.Padding = new System.Windows.Forms.Padding(3);
            this.tpServers.Size = new System.Drawing.Size(881, 420);
            this.tpServers.TabIndex = 0;
            this.tpServers.Text = "Servers";
            this.tpServers.UseVisualStyleBackColor = true;
            // 
            // dataGridView1
            // 
            this.dataGridView1.AllowUserToAddRows = false;
            this.dataGridView1.AllowUserToDeleteRows = false;
            this.dataGridView1.AutoGenerateColumns = false;
            this.dataGridView1.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dataGridView1.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.vAddressDataGridViewTextBoxColumn,
            this.vInfoDataGridViewTextBoxColumn});
            this.dataGridView1.DataSource = this.serverStoreBindingSource;
            this.dataGridView1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dataGridView1.Location = new System.Drawing.Point(3, 3);
            this.dataGridView1.Name = "dataGridView1";
            this.dataGridView1.ReadOnly = true;
            this.dataGridView1.Size = new System.Drawing.Size(875, 414);
            this.dataGridView1.TabIndex = 0;
            // 
            // tpSessions
            // 
            this.tpSessions.Controls.Add(this.dataGridView2);
            this.tpSessions.Location = new System.Drawing.Point(4, 22);
            this.tpSessions.Name = "tpSessions";
            this.tpSessions.Padding = new System.Windows.Forms.Padding(3);
            this.tpSessions.Size = new System.Drawing.Size(881, 420);
            this.tpSessions.TabIndex = 1;
            this.tpSessions.Text = "Sessions";
            this.tpSessions.UseVisualStyleBackColor = true;
            // 
            // dataGridView2
            // 
            this.dataGridView2.AllowUserToAddRows = false;
            this.dataGridView2.AllowUserToDeleteRows = false;
            this.dataGridView2.AutoGenerateColumns = false;
            this.dataGridView2.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dataGridView2.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.whoDataGridViewTextBoxColumn,
            this.sessionDataGridViewTextBoxColumn,
            this.keyDataGridViewTextBoxColumn,
            this.createddateDataGridViewTextBoxColumn,
            this.fromServerDataGridViewCheckBoxColumn,
            this.resDataGridViewTextBoxColumn,
            this.nextDataGridViewTextBoxColumn});
            this.dataGridView2.DataSource = this.sessionHandlerBindingSource;
            this.dataGridView2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dataGridView2.Location = new System.Drawing.Point(3, 3);
            this.dataGridView2.Name = "dataGridView2";
            this.dataGridView2.ReadOnly = true;
            this.dataGridView2.Size = new System.Drawing.Size(875, 414);
            this.dataGridView2.TabIndex = 0;
            // 
            // vAddressDataGridViewTextBoxColumn
            // 
            this.vAddressDataGridViewTextBoxColumn.DataPropertyName = "vAddress";
            this.vAddressDataGridViewTextBoxColumn.HeaderText = "vAddress";
            this.vAddressDataGridViewTextBoxColumn.Name = "vAddressDataGridViewTextBoxColumn";
            this.vAddressDataGridViewTextBoxColumn.ReadOnly = true;
            // 
            // vInfoDataGridViewTextBoxColumn
            // 
            this.vInfoDataGridViewTextBoxColumn.DataPropertyName = "vInfo";
            this.vInfoDataGridViewTextBoxColumn.HeaderText = "vInfo";
            this.vInfoDataGridViewTextBoxColumn.Name = "vInfoDataGridViewTextBoxColumn";
            this.vInfoDataGridViewTextBoxColumn.ReadOnly = true;
            // 
            // serverStoreBindingSource
            // 
            this.serverStoreBindingSource.DataSource = typeof(MSLib.ServerStore);
            // 
            // whoDataGridViewTextBoxColumn
            // 
            this.whoDataGridViewTextBoxColumn.DataPropertyName = "Who";
            this.whoDataGridViewTextBoxColumn.HeaderText = "Who";
            this.whoDataGridViewTextBoxColumn.Name = "whoDataGridViewTextBoxColumn";
            this.whoDataGridViewTextBoxColumn.ReadOnly = true;
            // 
            // sessionDataGridViewTextBoxColumn
            // 
            this.sessionDataGridViewTextBoxColumn.DataPropertyName = "session";
            this.sessionDataGridViewTextBoxColumn.HeaderText = "session";
            this.sessionDataGridViewTextBoxColumn.Name = "sessionDataGridViewTextBoxColumn";
            this.sessionDataGridViewTextBoxColumn.ReadOnly = true;
            // 
            // keyDataGridViewTextBoxColumn
            // 
            this.keyDataGridViewTextBoxColumn.DataPropertyName = "key";
            this.keyDataGridViewTextBoxColumn.HeaderText = "key";
            this.keyDataGridViewTextBoxColumn.Name = "keyDataGridViewTextBoxColumn";
            this.keyDataGridViewTextBoxColumn.ReadOnly = true;
            // 
            // createddateDataGridViewTextBoxColumn
            // 
            this.createddateDataGridViewTextBoxColumn.DataPropertyName = "created_date";
            this.createddateDataGridViewTextBoxColumn.HeaderText = "created_date";
            this.createddateDataGridViewTextBoxColumn.Name = "createddateDataGridViewTextBoxColumn";
            this.createddateDataGridViewTextBoxColumn.ReadOnly = true;
            // 
            // fromServerDataGridViewCheckBoxColumn
            // 
            this.fromServerDataGridViewCheckBoxColumn.DataPropertyName = "fromServer";
            this.fromServerDataGridViewCheckBoxColumn.HeaderText = "fromServer";
            this.fromServerDataGridViewCheckBoxColumn.Name = "fromServerDataGridViewCheckBoxColumn";
            this.fromServerDataGridViewCheckBoxColumn.ReadOnly = true;
            // 
            // resDataGridViewTextBoxColumn
            // 
            this.resDataGridViewTextBoxColumn.DataPropertyName = "res";
            this.resDataGridViewTextBoxColumn.HeaderText = "res";
            this.resDataGridViewTextBoxColumn.Name = "resDataGridViewTextBoxColumn";
            this.resDataGridViewTextBoxColumn.ReadOnly = true;
            // 
            // nextDataGridViewTextBoxColumn
            // 
            this.nextDataGridViewTextBoxColumn.DataPropertyName = "next";
            this.nextDataGridViewTextBoxColumn.HeaderText = "next";
            this.nextDataGridViewTextBoxColumn.Name = "nextDataGridViewTextBoxColumn";
            this.nextDataGridViewTextBoxColumn.ReadOnly = true;
            // 
            // sessionHandlerBindingSource
            // 
            this.sessionHandlerBindingSource.DataSource = typeof(MSLib.SessionHandler);
            // 
            // frmServerStatus
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(889, 446);
            this.Controls.Add(this.tcServer);
            this.Name = "frmServerStatus";
            this.Text = "Server Status";
            this.Load += new System.EventHandler(this.frmServerStatus_Load);
            this.tcServer.ResumeLayout(false);
            this.tpServers.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView1)).EndInit();
            this.tpSessions.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView2)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.serverStoreBindingSource)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.sessionHandlerBindingSource)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl tcServer;
        private System.Windows.Forms.TabPage tpServers;
        private System.Windows.Forms.TabPage tpSessions;
        private System.Windows.Forms.DataGridView dataGridView1;
        private System.Windows.Forms.DataGridViewTextBoxColumn vAddressDataGridViewTextBoxColumn;
        private System.Windows.Forms.DataGridViewTextBoxColumn vInfoDataGridViewTextBoxColumn;
        private System.Windows.Forms.BindingSource serverStoreBindingSource;
        private System.Windows.Forms.DataGridView dataGridView2;
        private System.Windows.Forms.DataGridViewTextBoxColumn whoDataGridViewTextBoxColumn;
        private System.Windows.Forms.DataGridViewTextBoxColumn sessionDataGridViewTextBoxColumn;
        private System.Windows.Forms.DataGridViewTextBoxColumn keyDataGridViewTextBoxColumn;
        private System.Windows.Forms.DataGridViewTextBoxColumn createddateDataGridViewTextBoxColumn;
        private System.Windows.Forms.DataGridViewCheckBoxColumn fromServerDataGridViewCheckBoxColumn;
        private System.Windows.Forms.DataGridViewTextBoxColumn resDataGridViewTextBoxColumn;
        private System.Windows.Forms.DataGridViewTextBoxColumn nextDataGridViewTextBoxColumn;
        private System.Windows.Forms.BindingSource sessionHandlerBindingSource;
    }
}