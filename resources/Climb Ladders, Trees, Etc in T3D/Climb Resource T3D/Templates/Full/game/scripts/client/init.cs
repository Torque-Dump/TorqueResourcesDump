//-----------------------------------------------------------------------------

// Variables used by client scripts & code.  The ones marked with (c)
// are accessed from code.  Variables preceeded by Pref:: are client
// preferences and stored automatically in the ~/client/prefs.cs file
// in between sessions.
//
//    (c) Client::MissionFile             Mission file name
//    ( ) Client::Password                Password for server join

//    (?) Pref::Player::CurrentFOV
//    (?) Pref::Player::DefaultFov
//    ( ) Pref::Input::KeyboardTurnSpeed

//    (c) pref::Master[n]                 List of master servers
//    (c) pref::Net::RegionMask     
//    (c) pref::Client::ServerFavoriteCount
//    (c) pref::Client::ServerFavorite[FavoriteCount]
//    .. Many more prefs... need to finish this off

// Moves, not finished with this either...
//    (c) firstPerson
//    $mv*Action...

//-----------------------------------------------------------------------------
// These are variables used to control the shell scripts and
// can be overriden by mods:
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// loadMaterials - load all materials.cs files
//-----------------------------------------------------------------------------
function loadMaterials()
{
   // Load any materials files for which we only have DSOs.
   
   for( %file = findFirstFile( "*/materials.cs.dso" );
        %file !$= "";
        %file = findNextFile( "*/materials.cs.dso" ))
   {
      // Only execute, if we don't have the source file.
      %csFileName = getSubStr( %file, 0, strlen( %file ) - 4 );
      if( !isFile( %csFileName ) )
         exec( %csFileName );
   }
   
   // Load all source material files.
   
   for( %file = findFirstFile( "*/materials.cs" );
        %file !$= "";
        %file = findNextFile( "*/materials.cs" ))
   {
      exec( %file );
   }
   
   // Load all materials created by the material editor if
   // the folder exists
   if( IsDirectory( "materialEditor" ) )
   {
      for( %file = findFirstFile( "materialEditor/*.cs.dso" );
           %file !$= "";
           %file = findNextFile( "materialEditor/*.cs.dso" ))
      {
         // Only execute, if we don't have the source file.
         %csFileName = getSubStr( %file, 0, strlen( %file ) - 4 );
         if( !isFile( %csFileName ) )
            exec( %csFileName );
      }
      
      for( %file = findFirstFile( "materialEditor/*.cs" );
           %file !$= "";
           %file = findNextFile( "materialEditor/*.cs" ))
      {
         exec( %file );
      }
   }
}

function reloadMaterials()
{
   reloadTextures();
   loadMaterials();
   reInitMaterials();
}

//-----------------------------------------------------------------------------
function initClient()
{
   echo("\n--------- Initializing " @ $appName @ ": Client Scripts ---------");

   // Make sure this variable reflects the correct state.
   $Server::Dedicated = false;

   // Game information used to query the master server
   $Client::GameTypeQuery = $appName;
   $Client::MissionTypeQuery = "Any";

   exec("art/gui/customProfiles.cs"); // override the base profiles if necessary

   // The common module provides basic client functionality
   initBaseClient();

   // Use our prefs to configure our Canvas/Window
   configureCanvas();
      
   /// Load client-side Audio Profiles/Descriptions
   exec("./audioProfiles.cs");

   // Load up the Game GUIs
   exec("art/gui/defaultGameProfiles.cs");
   exec("art/gui/PlayGui.gui");
   //exec("art/gui/ChatHud.gui");
   //exec("art/gui/playerList.gui");

   // Load up the shell GUIs
   exec("art/gui/mainMenuGui.gui");
   exec("art/gui/aboutDlg.gui");
   exec("art/gui/chooseLevelDlg.gui");
   exec("art/gui/joinServerDlg.gui");
   exec("art/gui/endGameGui.gui");
   exec("art/gui/remapDlg.gui");
   exec("art/gui/StartupGui.gui");

   // load the unified shell overrides
   exec("~/gui/ShellGui.cs");

   // Client scripts
   exec("./client.cs");
   exec("./game.cs");
   exec("./missionDownload.cs");
   exec("./serverConnection.cs");
   //exec("./playerList.cs");
   //exec("./chatHud.cs");
   //exec("./messageHud.cs");
   exec("~/gui/playGui.cs");
   //exec("./centerPrint.cs");
   exec("./message.cs");

   // Load useful Materials
   exec("./glowBuffer.cs");
   exec("./shaders.cs");
   exec("./commonMaterialData.cs" );

   // Default player key bindings
   exec("./default.bind.cs");

   if (isFile("./config.cs"))
      exec("./config.cs");

   // Load Example objectView 
   exec("art/gui/objectViewExample.gui");
   exec("./objectViewExample.cs");

   loadMaterials();
   
     // NOTE: We could easily destroy and reload these for mission
   // specific terrain material scripts.
   $Client::ManagedTerrainMaterials = "art/terrains/managedTerrainMaterials.cs";
   exec( $Client::ManagedTerrainMaterials );

   
   // Really shouldn't be starting the networking unless we are
   // going to connect to a remote server, or host a multi-player
   // game.
   setNetPort(0);

   // Copy saved script prefs into C++ code.
   setDefaultFov( $pref::Player::defaultFov );
   setZoomSpeed( $pref::Player::zoomSpeed );

   // Start up the main menu... this is separated out into a 
   // method for easier mod override.

   if ($startWorldEditor || $startGUIEditor) {
      // Editor GUI's will start up in the primary main.cs once
      // engine is initialized.
      return;
   }

   // Connect to server if requested.
   if ($JoinGameAddress !$= "") {
      // If we are instantly connecting to an address, load the
      // main menu then attempt the connect.
      loadMainMenu();
      connect($JoinGameAddress, "", $Pref::Player::Name);
   }
   else {
      // Otherwise go to the splash screen.
      Canvas.setCursor("DefaultCursor");
      loadStartup();
   }
}
//-----------------------------------------------------------------------------
function loadPlayerPickerData()
{
   // Load the datablocks we need for picking a player.
   // This function just needs to populate PlayerDatasGroup
   // with a valid set of objects (could be ScriptObjects,
   // PlayerDatas, or StaticShapeDatas for example). The
   // objects just need to have a name and a "shapeFile"
   // field. For now we are just pulling this directly from
   // the same set of PlayerDatas that we load on the server
   // since it will be much simpler for a new user to figure
   // out what to change to add their own Player's to the
   // picker but it would be a good optimization to use a
   // lighter-weight object.
   
   // Create our SimSet to hold the player datablocks for the picker
   if (!isObject(PlayerDatasGroup))
      new SimSet(PlayerDatasGroup);

   // We have to load these audio profiles to avoid a set of warnings
   exec("art/datablocks/audioProfiles.cs");

   // Load our default player script
   exec("art/datablocks/players/player.cs");

   // Load our other player scripts
   exec("art/datablocks/players/BoomBot.cs");
}

//-----------------------------------------------------------------------------
function loadDefaultMission()
{
   Canvas.setCursor("DefaultCursor");
   createServer( "SinglePlayer", "levels/ladder.mis" );

   %conn = new GameConnection(ServerConnection);
   RootGroup.add(ServerConnection);
   %conn.setConnectArgs($pref::Player::Name);
   %conn.setJoinPassword($Client::Password);
   %conn.connectLocal();
}

function loadMainMenu()
{
   // Startup the client with the Main menu...
   if (isObject( MainMenuGui ))
      Canvas.setContent( MainMenuGui );
   else if (isObject( UnifiedMainMenuGui ))
      Canvas.setContent( UnifiedMainMenuGui );
   Canvas.setCursor("DefaultCursor");
   
   // Attempt to load a set of datablocks for use in a player picker
   if (isFunction("loadPlayerPickerData"))
      loadPlayerPickerData();
}
