/**
 * @file RobotMessageHandler.cpp
 * @author Andy Hobelsberger <mailto:anho1001@stud.hs-kl.de>
 * @brief 
 * @version 1.0
 * @date 2023-05-19
 * 
 * @copyright Copyright (c) 2023
 * 
 * based on SPLMessageHandler.cpp authored by
 * @author <a href="mailto:jesse@tzi.de">Jesse Richter-Klug</a>
 */

#include "RobotMessageHandler.h"
#include "Platform/BHAssert.h"
#include "Platform/SystemCall.h"
#include "Platform/Time.h"
#include "Tools/Settings.h"
#include "Tools/Debugging/DebugDrawings.h"



void RobotMessageHandler::startLocal(int port, unsigned localId)
{
  ASSERT(!this->port);
  this->port = port;
  this->localId = localId;

  socket.setBlocking(false);
  VERIFY(socket.setBroadcast(false));
  std::string group = SystemCall::getHostAddr();
  group = "239" + group.substr(group.find('.'));
  VERIFY(socket.bind("0.0.0.0", port));
  VERIFY(socket.setTTL(0)); //keep packets off the network. non-standard(?), may work.
  VERIFY(socket.joinMulticast(group.c_str()));
  VERIFY(socket.setTarget(group.c_str(), port));
  socket.setLoopback(true);
}

void RobotMessageHandler::start(int port, const char* subnet)
{
  ASSERT(!this->port);
  this->port = port;

  socket.setBlocking(false);
  VERIFY(socket.setBroadcast(true));
  VERIFY(socket.bind("0.0.0.0", port));
  socket.setTarget(subnet, port);
  socket.setLoopback(false);
}

void RobotMessageHandler::send()
{
  if(!port)
    return;

  RobotMessage msg = RobotMessage();
  msg.compile();
  size_t size = msg.compress(writeBuffer);

  socket.write((char*) writeBuffer.data(), SPL_MAX_MESSAGE_BYTES); // Guarantees 128 bytes are sent

  // Plot usage of data buffer in percent:
  const float usageInPercent = 100.f * size / SPL_MAX_MESSAGE_BYTES;
  PLOT("module:RobotMessageHandler:messageDataUsageInPercent", usageInPercent);
}

void RobotMessageHandler::receive()
{
  if(!port)
    return; // not started yet

  int size;
  unsigned remoteIp = 0;

  // Read all messages 
  do {
    // Read raw bytes into read buffer
    size = localId ? socket.readLocal((char*) readBuffer.data(), readBuffer.size())
                   : socket.read((char*) readBuffer.data(), readBuffer.size(), remoteIp);
                   
    // Error Check
    if (size == -1) {
      //TODO Error handling here
    }

    // A Message was read
    if (size > 0) {
      RobotMessage msg;
      if (msg.decompress(readBuffer) // Decompress Successful
        && msg.header.senderID != Global::getSettings().playerNumber) // Ignore Messages from Self
      {
        msg.doCallbacks();
      }
    }
  } while(size > 0);
}