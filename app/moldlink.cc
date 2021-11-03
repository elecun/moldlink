/**
 * @file    main.cc
 * @brief   Moldlink Application (route from RS232 to MQTT for data logger)
 * @author  bh.hwang@iae.re.kr
 */

#include "include/util/cxxopts.hpp"
#include "include/spdlog/spdlog.h"
#include "include/spdlog/sinks/stdout_color_sinks.h"
#include "include/spdlog/fmt/bin_to_hex.h"
#include <csignal>
#include <array>
#include <deque>
#include <sys/types.h>
#include <unistd.h>
#include "include/util/json.hpp"
#include <map>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "mosquitto/mosquitto.h"
#include "core/serialbus.hpp"
#include "core/sensor.hpp"

using namespace std;
using namespace nlohmann; 

//global variables
moldlink::serialbus* _serialbus = nullptr;
struct mosquitto* _mqtt = nullptr;
int _pub_inteval_sec = 1;



static void postprocess(json& msg){
    if(!msg.empty()){
        string strdata = msg.dump();
        spdlog::info("publishing : {}", strdata);

        if(_mqtt){
            int ret = mosquitto_publish(_mqtt, nullptr, "moldlink/sensors", strdata.size(), strdata.c_str(), 2, false);
            mosquitto_loop(_mqtt, 3, 1);
            if(ret){
                spdlog::error("Broker connection error");
            }
        }
    }
}

void subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
    spdlog::info("subscribed");
}

/* MQTT Connection callback */
void connect_callback(struct mosquitto* mosq, void *obj, int result)
{
    spdlog::info("Connected to broker = {}", result);
}

/* MQTT message subscription callback */
void message_callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message)
{

}

void terminate() {

    if(_serialbus){
        _serialbus->stop();

        delete _serialbus;
        _serialbus = nullptr;
    }

    mosquitto_loop_stop(_mqtt, true);
    mosquitto_destroy(_mqtt);
    mosquitto_lib_cleanup();

    spdlog::info("Successfully terminated");
    exit(EXIT_SUCCESS);
}

void cleanup(int sig) {
  switch(sig){
    case SIGSEGV: { spdlog::warn("Segmentation violation"); } break;
    case SIGABRT: { spdlog::warn("Abnormal termination"); } break;
    case SIGKILL: { spdlog::warn("Process killed"); } break;
    case SIGBUS: { spdlog::warn("Bus Error"); } break;
    case SIGTERM: { spdlog::warn("Termination requested"); } break;
    case SIGINT: { spdlog::warn("interrupted"); } break;
    default:
      spdlog::info("Cleaning up the program");
  }
  ::terminate(); 
}


/* main */
int main(int argc, char* argv[])
{  
    const int signals[] = { SIGINT, SIGTERM, SIGBUS, SIGKILL, SIGABRT, SIGSEGV };
    for(const int& s:signals)
        signal(s, cleanup);

    //signal masking
    sigset_t sigmask;
    if(!sigfillset(&sigmask)){
        for(int signal:signals)
            sigdelset(&sigmask, signal); //delete signal from mask
    }
    else {
        spdlog::error("Signal Handling Error");
        ::terminate(); //if failed, do termination
    }

    if(pthread_sigmask(SIG_SETMASK, &sigmask, nullptr)!=0){ // signal masking for this thread(main)
        spdlog::error("Signal Masking Error");
        ::terminate();
    }

    spdlog::stdout_color_st("console");

    int optc = 0;
    string _device_port = "/dev/ttyUSB0";
    int _baudrate = 9600;

    string _mqtt_broker = "0.0.0.0";
    int _mqtt_rc = 0;

    while((optc=getopt(argc, argv, "p:b:t:i:h"))!=-1)
    {
        switch(optc){
            case 'p': { /* device port */
                _device_port = optarg;
            }
            break;

            case 'b': { /* baudrate */
                _baudrate = atoi(optarg);
            }
            break;

            case 't': { /* target ip to pub */
                _mqtt_broker = optarg;
            }
            break;
            
            case 'h':
            default:
            cout << fmt::format("Moldlink Middleware (built {}/{})", __DATE__, __TIME__) << endl;
            cout << "Usage: moldlink [-p port] [-b baudrate] [-t broker ip]" << endl;
            exit(EXIT_FAILURE);
            break;
        }
    }

    // show arguments
    spdlog::info("> set device port : {}", _device_port);
    spdlog::info("> set port baudrate : {}", _baudrate);
    spdlog::info("> set broker IP : {}", _mqtt_broker);

    try {

        mosquitto_lib_init();
        _mqtt = mosquitto_new("moldlink", true, 0);

        if(_mqtt){
            spdlog::info("connecting to broker...");
		    mosquitto_connect_callback_set(_mqtt, connect_callback);
		    mosquitto_message_callback_set(_mqtt, message_callback);
            mosquitto_subscribe_callback_set(_mqtt, subscribe_callback);
            _mqtt_rc = mosquitto_connect(_mqtt, _mqtt_broker.c_str(), 1883, 60);
            spdlog::info("mqtt connection : {}", _mqtt_rc);
            mosquitto_loop_start(_mqtt);
        }
        
        if(!_serialbus){
            _serialbus = new moldlink::serialbus(_device_port.c_str(), _baudrate);
            _serialbus->set_postprocess(postprocess);
            _serialbus->add_subport("sensor", new moldlink::sensor());
            _serialbus->start();
        }

        ::pause();

    }
    catch(const std::exception& e){
        spdlog::error("Exception : {}", e.what());
    }

    ::terminate();
    return EXIT_SUCCESS;
}