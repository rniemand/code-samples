using System;
using System.Data.SQLite;
using System.IO;
using System.Linq;
using System.Text;
using Dapper;
using SQLiteWithDapper.Extensions;
using SQLiteWithDapper.Models;

namespace SQLiteWithDapper
{
  class Program
  {
    private static SQLiteConnection _dbConnection;

    static void Main(string[] args)
    {
      // Sample Dapper and SQLit application as part of my blog post
      //    >> https://richardn.ca/2016/07/29/sqlite-and-dapper-in-c/
      //    Revised: 2018-06-07

      // If the DB file exists - let's remove it
      if (File.Exists("./TestDb.sqlite"))
        File.Delete("./TestDb.sqlite");

      CreateAndOpenDb();

      // Create "User" table and seed the admin user account
      SeedDatabase();

      // Fetch the admin user
      var adminUser = _dbConnection
        .Query<User>("SELECT * FROM Users WHERE Username = 'admin'")
        .First();

      Console.WriteLine($"The admin was created on: {adminUser.DateCreated}");

      CreateSecondUser();
      ModifyAdminUser();

      // Create "x" amount of random users
      AddMoreUsers(15);

      // Remove the last created user from the DB
      RemoveLastNonAdminUser();

      
      // All done
      Console.WriteLine();
      Console.WriteLine(">> Done - press ENTER to quit");
      Console.ReadLine();
    }

    // Supporting methods
    private static void CreateAndOpenDb()
    {
      const string dbFilePath = "./TestDb.sqlite";

      if (!File.Exists(dbFilePath))
      {
        SQLiteConnection.CreateFile(dbFilePath);
      }

      _dbConnection = new SQLiteConnection($"Data Source={dbFilePath};Version=3;");
      _dbConnection.Open();
    }

    private static void SeedDatabase()
    {
      // Create a Users table
      _dbConnection.ExecuteNonQuery(@"
        CREATE TABLE IF NOT EXISTS [Users] (
            [Id] INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
            [Username] NVARCHAR(64) NOT NULL,
            [Email] NVARCHAR(128) NOT NULL,
            [Password] NVARCHAR(128) NOT NULL,
            [DateCreated] TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )");

      // Insert an ADMIN user
      _dbConnection.ExecuteNonQuery(@"
        INSERT INTO Users
            (Username, Email, Password)
        VALUES
            ('admin', 'niemand.richard@gmail.com', 'test')");
    }

    private static void CreateSecondUser()
    {
      var secondUser = new User
      {
        Username = "rachel",
        Email = "1@2.com",
        Password = "password"
      };

      if (!secondUser.ExistsInDb(_dbConnection))
      {
        secondUser.SaveAsNewUser(_dbConnection);
      }
    }

    private static void ModifyAdminUser()
    {
      var adminUser = _dbConnection.GetUserByName("admin");
      adminUser.Password = $"pass_{DateTime.Now.Millisecond}";
      adminUser.SaveChanges(_dbConnection);
    }

    private static void AddMoreUsers(int amount)
    {
      var baseUsername = string.Format(
        "{0}{1}{2}{3}{4}{5}",
        DateTime.Now.Year,
        DateTime.Now.Month.ToString().PadLeft(2, '0'),
        DateTime.Now.Day.ToString().PadLeft(2, '0'),
        DateTime.Now.Hour.ToString().PadLeft(2, '0'),
        DateTime.Now.Minute.ToString().PadLeft(2, '0'),
        DateTime.Now.Second.ToString().PadLeft(2, '0'));

      for (var i = 0; i < amount; i++)
      {
        var tempUser = new User
        {
          Username = $"{baseUsername}{RandomString(4)}",
          Password = RandomString(10),
          Email = $"{RandomString(12)}@{RandomString(5)}.com"
        };

        tempUser.SaveAsNewUser(_dbConnection);
      }
    }

    private static string RandomString(int size)
    {
      var builder = new StringBuilder();
      var random = new Random();

      char ch;
      for (var i = 0; i < size; i++)
      {
        ch = Convert.ToChar(Convert.ToInt32(Math.Floor(26 * random.NextDouble() + 65)));
        builder.Append(ch);
      }

      return builder.ToString();
    }

    private static void RemoveLastNonAdminUser()
    {
      var user = _dbConnection.GetLastUser();
      if (user.Username == "admin") return;
      user.RemoveFromDb(_dbConnection);
    }
  }
}
