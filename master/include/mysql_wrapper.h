#ifndef __MYSQLWRAPPER__
#define __MYSQLWRAPPER__

// MySQL libraries
#include <vector>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <WiFi.h>

#include "../secrets.h"

#define QUERY_LEN 256
#define QUERY_SELECT_DEVICES "SELECT * FROM `mvincenzi14`.`devices`"
#define QUERY_SELECT_CONFIGURATION "SELECT `configurations`.`parameters` FROM `mvincenzi14`.`configurations` WHERE `configurations`.`type` = '%s'"
#define QUERY_INSERT_DEVICES "INSERT INTO `mvincenzi14`.`devices` (`mac_address`, `type`, `status`) VALUES ('%s', '%s', '%d') ON DUPLICATE KEY UPDATE `status` = '%d'"
#define QUERY_UPDATE_DEVICES "UPDATE `mvincenzi14`.`devices` SET `status` = '%d' WHERE `mac_address` = '%s'"
#define QUERY_INSERT_EVENTS "INSERT INTO `mvincenzi14`.`events` (`category`, `value`, `description`, `device_id`) VALUES ('%s', '%d', '%s', '%s')"
#define QUERY_INSERT_EVENTS_NO_VALUE "INSERT INTO `mvincenzi14`.`events` (`category`, `description`, `device_id`) VALUES ('%s', '%s', '%s')"

class MySqlWrapper {
public:
  static MySqlWrapper& getInstance() {
    static MySqlWrapper _INSTANCE;
    return _INSTANCE;
  }

  int insertDevice(const String mac_address, const String type, int status);
  int updateDevice(const String mac_address, int status);
  int insertEvent(const String category, const String description, const String device_id, int value = -1);
  String getDeviceConfiguration(const String type);
  int getDevices(int &columns, std::vector<std::vector<String>> &rows);
  void setAutoclose(boolean enable = false);

private:
  MySqlWrapper();

  static MySqlWrapper _INSTANCE;
  WiFiClient _wifi_client;
  boolean _autoclose = true;
  int _mysql_port = 3306;
  char* _mysql_user = MYSQL_USERNAME;                 // MySQL user login username
  char* _mysql_password = MYSQL_PASSWORD;             // MySQL user login password
  IPAddress _server_addr;                             // IP of the MySQL *server* here
  MySQL_Connection _conn;
  char _query[QUERY_LEN];                             // Change if query length change

  bool checkConnection();
  int executeInsertQuery(const String query);
  String executeSelectQuery(const String query);
  void executeSelectAllQuery(int &columns, std::vector<std::vector<String>> &rows, const String query);
};

#endif
