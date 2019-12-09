using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace MSLib.Forms {
    public partial class frmServerStatus : Form {
        public frmServerStatus() {
            InitializeComponent();


        }

        private void frmServerStatus_Load(object sender, EventArgs e) {
            this.serverStoreBindingSource.DataSource = MasterServer.Server_Store;
            this.sessionHandlerBindingSource.DataSource = MasterServer.Sessions;
        }
    }
}
