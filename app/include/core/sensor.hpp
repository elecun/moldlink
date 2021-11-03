/**
 * @file    sensor.hpp
 * @brief   read sensor data
 * @author Byunghun<bh.hwang@iae.re.kr>
 */

#ifndef _MOLDLINK_SENSOR_HPP_
#define _MOLDLINK_SENSOR_HPP_

#include <string>
#include <spdlog/spdlog.h>
#include <core/subport.hpp>
#include <spdlog/fmt/bin_to_hex.h>
#include <util/json.hpp>
#include <deque>


using namespace std;
using namespace nlohmann;

#define STX 0x02
#define ETX 0x03
#define ACK 0x06

namespace moldlink {
    class sensor : public subport {

        public:
            sensor(){
            }
            virtual ~sensor() { }

            virtual int read(unsigned char* buffer){

            }

            virtual bool write(unsigned char* buffer, int size){
                return false;
            }

            virtual void readsome(boost::asio::serial_port* bus, json& data){

                const int max_len = 1024;
                unsigned char rbuffer[max_len] = {0, };
                int read_len = bus->read_some(boost::asio::buffer(rbuffer, max_len));

                //insert to queue
                for(int i=0;i<read_len;i++){
                    _qbuffer.push_back(rbuffer[i]);
                }

                union {
                    unsigned long value;
                    float f_value;
                } u;

                const int pack_size = 36;

                //process with queue
                while(1){
                    if(_qbuffer.size()>=pack_size){
                        if(_qbuffer[0]==0x02 && _qbuffer[1]==0x55 && _qbuffer[34]==0x03 && _qbuffer[35]==0xaa){
                            u.value = ((_qbuffer[5]&0xff)<<24)|((_qbuffer[4]&0xff)<<16)|((_qbuffer[3]&0xff)<<8)|(_qbuffer[2]&0xff);
                            data["Ch1"] = u.f_value;
                            
                            u.value = ((_qbuffer[9]&0xff)<<24)|((_qbuffer[8]&0xff)<<16)|((_qbuffer[7]&0xff)<<8)|(_qbuffer[6]&0xff);
                            data["Ch2"] = u.f_value;

                            u.value = ((_qbuffer[13]&0xff)<<24)|((_qbuffer[12]&0xff)<<16)|((_qbuffer[11]&0xff)<<8)|(_qbuffer[10]&0xff);
                            data["Ch3"] = u.f_value;

                            u.value = ((_qbuffer[17]&0xff)<<24)|((_qbuffer[16]&0xff)<<16)|((_qbuffer[15]&0xff)<<8)|(_qbuffer[14]&0xff);
                            data["Ch4"] = u.f_value;

                            u.value = ((_qbuffer[21]&0xff)<<24)|((_qbuffer[20]&0xff)<<16)|((_qbuffer[19]&0xff)<<8)|(_qbuffer[18]&0xff);
                            data["Ch5"] = u.f_value;

                            u.value = ((_qbuffer[25]&0xff)<<24)|((_qbuffer[24]&0xff)<<16)|((_qbuffer[23]&0xff)<<8)|(_qbuffer[22]&0xff);
                            data["Ch6"] = u.f_value;

                            u.value = ((_qbuffer[29]&0xff)<<24)|((_qbuffer[28]&0xff)<<16)|((_qbuffer[27]&0xff)<<8)|(_qbuffer[26]&0xff);
                            data["Ch7"] = u.f_value;

                            u.value = ((_qbuffer[33]&0xff)<<24)|((_qbuffer[32]&0xff)<<16)|((_qbuffer[31]&0xff)<<8)|(_qbuffer[30]&0xff);
                            data["Ch8"] = u.f_value;

                            //remove
                            for(int i=0;i<pack_size;i++)
                                _qbuffer.pop_front();
                        }
                        else {
                            _qbuffer.pop_front();
                        }
                    }
                    else
                        break;
                }


                vector<char> rpacket(rbuffer, rbuffer+read_len);

                //process received data

                boost::this_thread::sleep_for(boost::chrono::milliseconds(100));

            }

            virtual void request(boost::asio::serial_port* bus, json& response){
                //read request
                unsigned char frame[] = { STX, 0x30, 0x30+(unsigned char)_id, 0x52, 0x58, 0x50, 0x30, ETX, 0x00};
                frame[8] = _checksum(frame, 8);
                int write_len = bus->write_some(boost::asio::buffer(frame, 9));
                spdlog::info("requested read sensor {}", _id);

                vector<char> wpacket(frame, frame+write_len);
                //spdlog::info("write data : {:x}", spdlog::to_hex(wpacket));

                #define MAX_READ_BUFFER 1024
                unsigned char rbuffer[MAX_READ_BUFFER] = {0, };
                int read_len = bus->read_some(boost::asio::buffer(rbuffer, MAX_READ_BUFFER));
                //spdlog::info("{}bytes read", read_len);

                vector<char> rpacket(rbuffer, rbuffer+read_len);
                //spdlog::info("read data : {:x}", spdlog::to_hex(rpacket));

                //parse data
                float value = parse_value(rbuffer, read_len);
                //spdlog::info("sensor {} : {}", _id, value);

                boost::this_thread::sleep_for(boost::chrono::milliseconds(250));    //must sleep

                response["sensor"]["id"] = _id;
                response["sensor"]["value"] = value;
            }

        private:



            json parse(unsigned char* data, int len){

            }

            float parse_value(unsigned char* data, int size){

                unsigned char* frame = new unsigned char[size];
                memcpy(frame, data, sizeof(unsigned char)*size);

                //checksum validation
                unsigned char chksum = _checksum(frame, size-1);
                if(chksum!=frame[size-1]){
                    spdlog::error("BCC is invalid");
                }

                //parse data
                float value = 0.0;
                if(frame[0]==ACK && frame[1]==STX && frame[14]==ETX && frame[3]==(0x30+(unsigned char)_id)){
                    char vt[4] = {0, };
                    memcpy(vt, &frame[9], 4);
                    value = atof(vt);
                    if(frame[13]!=0x30){
                        value = value*pow(0.1,(frame[13]-0x30));
                    }
                    if(frame[8]==0x2d)
                        value *= -1;
                }
                else
                    spdlog::error("failed parse {:x}, {:x}", frame[3], (0x30+(unsigned char)_id));
                delete []frame;

                return value;
            }

        private:
            unsigned char _checksum(unsigned char* buffer, int size){
                unsigned char chk = 0x00;
                for(int i=0;i<size;i++){
                    chk ^= buffer[i];
                }
                return chk;
            }



        private:
            int _id = 0;
            std::deque<unsigned char> _qbuffer;
    };
} /* end of namespace */

#endif