#ifndef __INFLUXWRAPPER__
#define __INFLUXWRAPPER__

// InfluxDB library
#include <InfluxDbClient.h>
#include "../secrets.h"
#include "../config.h"

class InfluxWrapper {
public:
  static InfluxWrapper& getInstance() {
    static InfluxWrapper _INSTANCE;
    return _INSTANCE;
  }

  InfluxWrapper& checkConnection();
  InfluxWrapper& writeToInflux(const String field, float val);
  InfluxWrapper& writeToInflux(const String field, int val);
  InfluxWrapper& addTag(const String key, const String value);

private:
  InfluxWrapper();
  static InfluxWrapper _INSTANCE;
  InfluxDBClient _client_idb;
  Point _point_device;
};

#endif
