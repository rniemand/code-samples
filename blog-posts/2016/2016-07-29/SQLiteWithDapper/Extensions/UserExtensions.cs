using System.Data.SQLite;
using System.Linq;
using Dapper;
using SQLiteWithDapper.Models;

namespace SQLiteWithDapper.Extensions
{
  public static class UserExtensions
  {
    public static void SaveAsNewUser(this User user, SQLiteConnection connection)
    {
      connection.ExecuteNonQuery(@"
            INSERT INTO Users (Username, Email, Password)
            VALUES (@Username, @Email, @Password)",
        user);
    }

    public static bool ExistsInDb(this User user, SQLiteConnection connection)
    {
      var rows = connection.Query($"SELECT COUNT(1) as 'Count' FROM Users WHERE Username = '{user.Username}'");

      return (int)rows.First().Count > 0;
    }

    public static User GetUserByName(this SQLiteConnection connection, string username)
    {
      var userCollection = connection.Query<User>(
        "SELECT * FROM Users WHERE Username = @UserName",
        new { Username = username });

      return userCollection.FirstOrDefault();
    }

    public static void SaveChanges(this User user, SQLiteConnection connection)
    {
      connection.ExecuteNonQuery(@"
        UPDATE Users
        SET
        Email = @Email, Password = @Password
        WHERE Id = @Id", user);
    }

    public static User GetLastUser(this SQLiteConnection connection)
    {
      var userCollection = connection.Query<User>("SELECT * FROM Users ORDER BY Id DESC LIMIT 1");
      return userCollection.FirstOrDefault();
    }

    public static void RemoveFromDb(this User user, SQLiteConnection connection)
    {
      connection.ExecuteNonQuery(
        "DELETE FROM Users WHERE Id = @Id", user);
    }
  }
}
