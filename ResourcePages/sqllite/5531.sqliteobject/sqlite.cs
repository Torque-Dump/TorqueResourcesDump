// SQLite integration script

function sqliteTest(%dbname)
{
   %sqlite = new SQLiteObject(sqlite);
   if (%sqlite == 0)
   {
      echo("ERROR: Failed to create SQLiteObject. sqliteTest aborted.");
      return;
   }
   
   // open database
   if (sqlite.openDatabase(%dbname) == 0)
   {
      echo("ERROR: Failed to open database: " @ %dbname);
      echo("       Ensure that the disk is not full or write protected.  sqliteTest aborted.");
      sqlite.delete();
      return;
   }
   
   // create a new simple table for demonstration purposes
   %query = "CREATE TABLE users (username VARCHAR(20))";
   %result = sqlite.query(%query, 0);
   if (%result == 0)
   {
      // query failed
      echo("ERROR: Failed to create new table.  sqliteTest aborted.");
      sqlite.closeDatabase();
      sqlite.delete();
      return;
   }
   // result sets are persistant, and you can have more than one at any time.  This
   // allows for cross referencing data or what not.  However, each result set takes up
   // memory and large queries will take up a significant amount of memory.
   // If you no longer need a result set, you should clear it.
   sqlite.clearResult(%result);
   // insert some data into the table
   %query = "INSERT INTO users (username) VALUES ('john')";
   %result = sqlite.query(%query, 0);
   if (%result == 0)
   {
      // query failed
      echo("ERROR: Failed to INSERT test data into table.  Attempting to DROP table before aborting.");
      if (sqlite.query("DROP TABLE users", 0) == 0)
         echo("Failed to DROP table.  sqliteTest aborted.");
      else
         echo("Table DROP completed properly.  sqliteTest aborted.");
      sqlite.closeDatabase();
      sqlite.delete();
      return;
   }
   sqlite.clearResult(%result);
   // retrieve some data from the table
   %query = "SELECT * FROM users";
   %result = sqlite.query(%query, 0);
   if (%result == 0)
   {
      echo("ERROR: Failed to SELECT from users table.");
   }
   // attempt to retrieve result data
   while (!sqlite.endOfResult(%result))
   {
      %username = sqlite.getColumn(%result, "username");
      echo("Retrived username = " @ %username);
      sqlite.nextRow(%result);
   }
   
   sqlite.clearResult(%result);
   // delete the table
   %result = sqlite.query("DROP TABLE users", 0);
   if (%result == 0)
   {
      echo("ERROR: Failed to DROP table");
   }
   //sqlite.clearResult(%result);
   // close database
   sqlite.closeDatabase();
   
   // delete SQLite object.
   sqlite.delete();
}

function sqlite::onQueryFailed(%this, %error)
{
   echo ("SQLite Query Error: " @ %error);
}