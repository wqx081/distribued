#ifndef PUBLIC_SESSION_OPTIONS_H_
#define PUBLIC_SESSION_OPTIONS_H_

#include <string>
#include "protobuf/config.pb.h"

using std::string;

namespace mr {

class Env;

struct SessionOptions {
  Env* env;
  string target;
  ConfigProto config;
  SessionOptions();
};
} // mr
#endif // PUBLIC_SESSION_OPTIONS_H_
