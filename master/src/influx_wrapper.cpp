#include "../include/influx_wrapper.h"

InfluxWrapper::InfluxWrapper() : _client_idb(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN),
                                 _point_device(INFLUX_DATA_POINT_LABEL)
{
  //pointDevice.addTag("device", "ESP32");
}

InfluxWrapper& InfluxWrapper::checkConnection()
{
  // Check server connection
  if (_client_idb.validateConnection()) {
    Serial.print(F("[InfluxWrapper] Connected to InfluxDB: "));
    Serial.println(_client_idb.getServerUrl());
  } else {
    Serial.print(F("[InfluxWrapper] InfluxDB connection failed: "));
    Serial.println(_client_idb.getLastErrorMessage());
  }

  return *this;
}

InfluxWrapper& InfluxWrapper::addTag(const String key, const String value)
{
  _point_device.addTag(key, value);
  return *this;
}

InfluxWrapper& InfluxWrapper::writeToInflux(const String field, float val)
{
  checkConnection();
  _point_device.clearFields();

  _point_device.addField(field, val);

  Serial.print(F("[InfluxWrapper] Writing: "));
  Serial.println(_client_idb.pointToLineProtocol(_point_device));

  if (!_client_idb.writePoint(_point_device)) {
    Serial.print(F("[InfluxWrapper] InfluxDB write failed: "));
    Serial.println(_client_idb.getLastErrorMessage());
  } else {
    Serial.println("[InfluxWrapper] Data recorded on InfluxDB");
  }

  _point_device.clearTags();

  return *this;
}

InfluxWrapper& InfluxWrapper::writeToInflux(const String field, int val)
{
  checkConnection();
  _point_device.clearFields();

  _point_device.addField(field, val);

  Serial.print(F("[InfluxWrapper] Writing: "));
  Serial.println(_client_idb.pointToLineProtocol(_point_device));

  if (!_client_idb.writePoint(_point_device)) {
    Serial.print(F("[InfluxWrapper] InfluxDB write failed: "));
    Serial.println(_client_idb.getLastErrorMessage());
  } else {
    Serial.println("[InfluxWrapper] Data recorded on InfluxDB");
  }
  _point_device.clearTags();

  return *this;
}
