#include "../include/mysql_wrapper.h"

MySqlWrapper::MySqlWrapper() :  _server_addr(IPAddress(MYSQL_HOST)),
                _conn((Client *)&_wifi_client)
{
  //
}

bool MySqlWrapper::checkConnection()
{
  if (_conn.connect(_server_addr, _mysql_port, _mysql_user, _mysql_password)) {
    Serial.println(F("[MySqlWrapper] MySQL connection established."));
    return true;
  }

  Serial.println(F("[MySqlWrapper] MySQL connection failed."));
  return false;
}

int MySqlWrapper::executeInsertQuery(const String query)
{
  int error = -1;
  if (checkConnection()) {
    MySQL_Cursor *cur_mem = new MySQL_Cursor(&_conn);

    cur_mem->execute(query.c_str());
    delete cur_mem;

    error = 1;

    Serial.println(F("[MySqlWrapper] Data recorded on MySQL"));

    if (_autoclose) _conn.close();
  }

  return error;
}

String MySqlWrapper::executeSelectQuery(const String query)
{
  row_values *row = NULL;
  String configuration = "";

  if (checkConnection()) {
    MySQL_Cursor *cur_mem = new MySQL_Cursor(&_conn);

    cur_mem->execute(query.c_str());

    column_names *columns = cur_mem->get_columns();

    row = cur_mem->get_next_row();
    if (row != NULL) {
      configuration = row->values[0];
    }
    delete cur_mem;

    Serial.println(F("[MySqlWrapper] Data read by MySQL"));

    if (_autoclose) _conn.close();
  }

  return configuration;
}

void MySqlWrapper::executeSelectAllQuery(int &num_columns, std::vector<std::vector<String>> &rows, const String query)
{
  if (checkConnection()) {
    Serial.println(F("[MySqlWrapper] MySQL connection established."));

    MySQL_Cursor *cur_mem = new MySQL_Cursor(&_conn);
    cur_mem->execute(query.c_str());
    column_names *columns = cur_mem->get_columns();

    row_values *row = NULL;
    do {
      row = cur_mem->get_next_row();
      std::vector<String> r;
      if (row != NULL) {
        for (int f = 0; f < columns->num_fields; f++) {
          r.push_back(String(row->values[f]));
        }
        rows.push_back(r);
      }
    } while (row != NULL);

    delete cur_mem;

    Serial.println(F("[MySqlWrapper] Data read by MySQL"));
    if (_autoclose) _conn.close();
  }
}

void MySqlWrapper::getCards(int &columns, std::vector<std::vector<String>> &rows)
{
  executeSelectAllQuery(columns, rows, QUERY_SELECT_CARDS);
}

void MySqlWrapper::validateCard(const String &card, bool &is_master, bool &is_authorized)
{
  sprintf(_query, QUERY_VALIDATE_CARD, card.c_str());

  int num_columns;
  std::vector<std::vector<String>> rows;
  executeSelectAllQuery(num_columns, rows, _query);

  is_master = false;
  is_authorized = false;
  if (rows.size() < 1) {
    return;
  }

  is_master = rows[0][1] == "1";
  is_authorized = true;
}

void MySqlWrapper::authorizeCard(const String &card)
{
  sprintf(_query, QUERY_AUTHORIZE_CARD, card.c_str());
  executeInsertQuery(_query);
}

void MySqlWrapper::revokeCard(const String &card)
{
  sprintf(_query, QUERY_REVOKE_CARD, card.c_str());
  executeInsertQuery(_query);
}

int MySqlWrapper::insertDevice(const String mac_address, const String type, int status)
{
  sprintf(_query, QUERY_INSERT_DEVICES, mac_address.c_str(), type.c_str(), status, status);
  return executeInsertQuery(_query);
}

int MySqlWrapper::updateDevice(const String mac_address, int status)
{
  sprintf(_query, QUERY_UPDATE_DEVICES, status, mac_address.c_str());
  return executeInsertQuery(_query);
}

int MySqlWrapper::insertEvent(const String category, const String description, const String device_id, int value)
{
  if (value >= 0) {
    sprintf(_query, QUERY_INSERT_EVENTS, category.c_str(), value, description.c_str(), device_id.c_str());
  } else {
    sprintf(_query, QUERY_INSERT_EVENTS_NO_VALUE, category.c_str(), description.c_str(), device_id.c_str());
  }

  return executeInsertQuery(_query);
}

int MySqlWrapper::getDevices(int &columns, std::vector<std::vector<String>> &rows)
{
  executeSelectAllQuery(columns, rows, QUERY_SELECT_DEVICES);
}

String MySqlWrapper::getDeviceConfiguration(const String type)
{
  sprintf(_query, QUERY_SELECT_CONFIGURATION, type.c_str());
  return executeSelectQuery(_query);
}

void MySqlWrapper::setAutoclose(boolean enable)
{
  this->_autoclose = enable;
}
